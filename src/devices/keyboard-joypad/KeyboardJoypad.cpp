/*
 * Copyright (C) 2021 Istituto Italiano di Tecnologia (IIT)
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms of the
 * BSD-2-Clause license. See the accompanying LICENSE file for details.
 */

#define GL_GLEXT_PROTOTYPES
#define GL3_PROTOTYPES
#define GL_SILENCE_DEPRECATION

#include <GL/glew.h>
#include <GL/gl.h>

#include <GLFW/glfw3.h>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <stdio.h>

#include <mutex>
#include <atomic>
#include <unordered_map>
#include <string>
#include <vector>
#include <algorithm>
#include <cmath>

#include <yarp/os/LogStream.h>

#include <KeyboardJoypad.h>
#include <KeyboardJoypadLogComponent.h>


struct ButtonState {
    ImGuiKey key = ImGuiKey_COUNT;
    int col = 0;
    int sign = 1;
    size_t index = 0;
    bool active = false;
    bool buttonPressed = false;
};

using ButtonsMap = std::unordered_map<std::string, ButtonState>;
using ButtonsTable = std::pair<std::vector<ButtonsMap>, int>; //The second element is the number of columns

static bool parseFloat(yarp::os::Searchable& cfg, const std::string& key, float min_value, float max_value, float& value)
{
    if (!cfg.check(key))
    {
        yCInfo(KEYBOARDJOYPAD) << "The key" << key << "is not present in the configuration file."
                               << "Using the default value:" << value;
        return true;
    }

    if (!cfg.find(key).isFloat64() && !cfg.find(key).isInt64())
    {
        yCError(KEYBOARDJOYPAD) << "The value of " << key << " is not a float";
        return false;
    }
    float input = static_cast<float>(cfg.find(key).asFloat64());
    if (input < min_value || input > max_value)
    {
        yCError(KEYBOARDJOYPAD) << "The value of " << key << " is out of range. It should be between" << min_value << "and" << max_value;
        return false;
    }
    value = input;
    return true;
}

static bool parseInt(yarp::os::Searchable& cfg, const std::string& key, int min_value, int max_value, int& value)
{
    if (!cfg.check(key))
    {
        yCInfo(KEYBOARDJOYPAD) << "The key" << key << "is not present in the configuration file."
                               << "Using the default value:" << value;
        return true;
    }

    if (!cfg.find(key).isInt64())
    {
        yCError(KEYBOARDJOYPAD) << "The value of " << key << " is not an integer";
        return false;
    }
    int input = static_cast<int>(cfg.find(key).asInt64());
    if (input < min_value || input > max_value)
    {
        yCError(KEYBOARDJOYPAD) << "The value of " << key << " is out of range. It should be between" << min_value << "and" << max_value;
        return false;
    }
    value = input;
    return true;
}

struct Settings {
    float button_size = 100;
    float min_button_size = 50;
    float max_button_size = 200;
    float font_multiplier = 1.0;
    float min_font_multiplier = 0.5;
    float max_font_multiplier = 4.0;
    float gui_period = 0.033f;
    int window_width = 1280;
    int window_height = 720;

    bool parseFromConfigFile(yarp::os::Searchable& cfg)
    {
        if (!parseFloat(cfg, "button_size", static_cast<float>(1), static_cast<float>(1e5), button_size))
        {
            return false;
        }

        if (!parseFloat(cfg, "min_button_size", static_cast<float>(1), static_cast<float>(1e5), min_button_size))
        {
            return false;
        }

        if (!parseFloat(cfg, "max_button_size", static_cast<float>(1), static_cast<float>(1e5), max_button_size))
        {
            return false;
        }

        if (!parseFloat(cfg, "font_multiplier", static_cast<float>(0.01), static_cast<float>(1e5), font_multiplier))
        {
            return false;
        }

        if (!parseFloat(cfg, "min_font_multiplier", static_cast<float>(0.01), static_cast<float>(1e5), min_font_multiplier))
        {
            return false;
        }

        if (!parseFloat(cfg, "max_font_multiplier", static_cast<float>(0.01), static_cast<float>(1e5), max_font_multiplier))
        {
            return false;
        }

        if (!parseInt(cfg, "window_width", 1, static_cast<int>(1e4), window_width))
        {
            return false;
        }

        if (!parseInt(cfg, "window_height", 1, static_cast<int>(1e4), window_height))
        {
            return false;
        }

        if (!parseFloat(cfg, "gui_period", static_cast<float>(1e-3), static_cast<float>(1e5), gui_period))
        {
            return false;
        }

        return true;
    }
};

enum class Axis
{
    WS = 0,
    AD = 1,
    UP_DOWN = 2,
    LEFT_RIGHT = 3
};

struct AxisSettings
{
    int sign;
    size_t index;
};

struct AxesSettings
{
    std::unordered_map<Axis, AxisSettings> axes;

    std::string wasd_label = "WASD";
    std::string arrows_label = "Arrows";

    bool parseFromConfigFile(yarp::os::Searchable& cfg)
    {
        if (!cfg.check("axes"))
        {
            yCInfo(KEYBOARDJOYPAD) << "The key \"axes\" is not present in the configuration file. Enabling both wasd and the arrows.";
            axes[Axis::AD] = {+1, 0};
            axes[Axis::WS] = {+1, 1};
            axes[Axis::LEFT_RIGHT] = {+1, 2};
            axes[Axis::UP_DOWN] = {+1, 3};
            return true;
        }

        if (!cfg.find("axes").isList())
        {
            yCError(KEYBOARDJOYPAD) << "The value of \"axes\" is not a list";
            return false;
        }

        yarp::os::Bottle* axes_list = cfg.find("axes").asList();

        for (size_t i = 0; i < axes_list->size(); i++)
        {
            if (!axes_list->get(i).isString())
            {
                yCError(KEYBOARDJOYPAD) << "The value at index" << i << "of the axes list is not a string.";
                return false;
            }

            std::string axis = axes_list->get(i).asString();

            //Check if the first character is a - or a + and remove it
            int sign = +1;
            if (axis[0] == '-' || axis[0] == '+')
            {
                sign = axis[0] == '-' ? -1 : +1;
                axis = axis.substr(1);
            }

            std::transform(axis.begin(), axis.end(), axis.begin(), ::tolower);

            if (axis == "ws")
            {
                axes[Axis::WS] = {sign, i};
            }
            else if (axis == "ad")
            {
                axes[Axis::AD] = {sign, i};
            }
            else if (axis == "up_down")
            {
                axes[Axis::UP_DOWN] = {sign, i};
            }
            else if (axis == "left_right")
            {
                axes[Axis::LEFT_RIGHT] = {sign, i};
            }
            else if (axis != "")
            {
                yCError(KEYBOARDJOYPAD) << "The value of the axes list (" << axis << ") is not a valid axis."
                                        << "Allowed values(\"ws\", \"ad\", \"up_down\", \"left_right\","
                                        << "eventually with a + or - as prefix, and \"\")";
                return false;
            }
        }

        if (cfg.check("wasd_label"))
        {
            wasd_label = cfg.find("wasd_label").asString();
        }
        else
        {
            yCInfo(KEYBOARDJOYPAD) << "The key \"wasd_label\" is not present in the configuration file."
                                   << "Using the default value:" << wasd_label;
        }

        if (cfg.check("arrows_label"))
        {
            arrows_label = cfg.find("arrows_label").asString();
        }
        else
        {
            yCInfo(KEYBOARDJOYPAD) << "The key \"arrows_label\" is not present in the configuration file."
                                   << "Using the default value:" << arrows_label;
        }

        return true;
    }
};

class yarp::dev::KeyboardJoypad::Impl
{
public:
    GLFWwindow* window = nullptr;

    std::atomic_bool need_to_close{false}, closed{false};

    std::mutex mutex;

    ImVec4 button_inactive_color;
    ImVec4 button_active_color;

    Settings settings;
    AxesSettings axes_settings;

    std::vector<std::pair<std::string, ButtonsTable>> sticks;
    std::vector<std::vector<size_t>> sticks_to_axes;
    std::vector<double> axes_values;
    std::vector<std::vector<double>> sticks_values;

    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);


    static void GLMessageCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei, const GLchar* message, const void*) {
        yCError(KEYBOARDJOYPAD, "GL CALLBACK: %s source = 0x%x, type = 0x%x, id = 0x%x, severity = 0x%x, message = %s",
            (type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : ""),
            source, type, id, severity, message);
    }

    static void glfwErrorCallback(int error, const char* description) {
        yCError(KEYBOARDJOYPAD, "GLFW error %d: %s", error, description);
    }

    void renderButtonsTable(const std::string& name, ButtonsTable& buttons_table, const ImVec2& position)
    {

        //Define the size of the buttons
        ImVec2 buttonSize(settings.button_size, settings.button_size);

        const int& n_cols = buttons_table.second;
        ImGui::SetNextWindowPos(position, ImGuiCond_FirstUseEver);
        ImGui::Begin(name.c_str(), 0, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings);
        ImGui::SetWindowFontScale(settings.font_multiplier);

        ImGui::BeginTable(name.c_str(), n_cols, ImGuiTableFlags_NoSavedSettings | ImGuiTableFlags_SizingMask_);
        for (auto& row : buttons_table.first)
        {
            ImGui::TableNextRow();
            for (auto& button : row)
            {
                ImGui::TableSetColumnIndex(button.second.col);

                if (ImGui::IsKeyPressed(button.second.key))
                {
                    button.second.buttonPressed = true;
                    button.second.active = true;
                }

                if (button.second.buttonPressed && ImGui::IsKeyReleased(button.second.key))
                {
                    button.second.buttonPressed = false;
                    button.second.active = false;
                }

                ImGuiStyle& style = ImGui::GetStyle();
                ImVec4& buttonColor = button.second.active ? button_active_color : button_inactive_color;
                style.Colors[ImGuiCol_Button] = buttonColor;
                style.Colors[ImGuiCol_ButtonHovered] = buttonColor;
                style.Colors[ImGuiCol_ButtonActive] = buttonColor;

                axes_values[button.second.index] += button.second.sign * button.second.active;

                // Create a button
                if (ImGui::Button(button.first.c_str(), buttonSize))
                {
                    button.second.active = !button.second.active;
                }
            }
        }
        ImGui::EndTable();
        ImGui::End();
    }

};



yarp::dev::KeyboardJoypad::KeyboardJoypad()
    : yarp::dev::DeviceDriver(),
    yarp::os::PeriodicThread(0.033, yarp::os::ShouldUseSystemClock::Yes)
{
    m_pimpl = std::make_unique<Impl>();
}

yarp::dev::KeyboardJoypad::~KeyboardJoypad()
{
    this->stop();
}

bool yarp::dev::KeyboardJoypad::open(yarp::os::Searchable& cfg)
{
    if (!m_pimpl->settings.parseFromConfigFile(cfg))
    {
        return false;
    }

    if (!m_pimpl->axes_settings.parseFromConfigFile(cfg))
    {
        return false;
    }

    this->setPeriod(m_pimpl->settings.gui_period);

    // Start the thread
    if (!this->start()) {
        yCError(KEYBOARDJOYPAD) << "Thread start failed, aborting.";
        this->close();
        return false;
    }

    return true;
}

bool yarp::dev::KeyboardJoypad::close()
{
    yCInfo(KEYBOARDJOYPAD) << "Closing the device";
    this->askToStop();
    return true;
}

bool yarp::dev::KeyboardJoypad::threadInit()
{
    std::lock_guard<std::mutex> lock(m_pimpl->mutex);

    glfwSetErrorCallback(&KeyboardJoypad::Impl::glfwErrorCallback);
    if (!glfwInit()) {
        yCError(KEYBOARDJOYPAD, "Unable to initialize GLFW");
        return false;
    }

    m_pimpl->window = glfwCreateWindow(m_pimpl->settings.window_width, m_pimpl->settings.window_height,
                                       "YARP Keyboard as Joypad Device Window", nullptr, nullptr);
    if (!m_pimpl->window) {
        yCError(KEYBOARDJOYPAD, "Could not create window");
        return false;
    }

    glfwMakeContextCurrent(m_pimpl->window);
    glfwSwapInterval(1);

    // Initialize the GLEW OpenGL 3.x bindings
    // GLEW must be initialized after creating the window
    glewExperimental = GL_TRUE;
    GLenum err = glewInit();
    if (err != GLEW_OK) {
        yCError(KEYBOARDJOYPAD) << "glewInit failed, aborting.";
        return false;
    }
    yCInfo(KEYBOARDJOYPAD) << "Using GLEW" << (const char*)glewGetString(GLEW_VERSION);

    glDebugMessageControl(GL_DEBUG_SOURCE_API, GL_DEBUG_TYPE_OTHER, GL_DEBUG_SEVERITY_NOTIFICATION, 0, NULL, GL_FALSE); //This is to ignore message 0x20071 about the use of the VIDEO memory

    glDebugMessageCallback(&KeyboardJoypad::Impl::GLMessageCallback, NULL);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glEnable(GL_DEBUG_OUTPUT);


    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavNoCaptureKeyboard;

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(m_pimpl->window, true);
    ImGui_ImplOpenGL3_Init();

    m_pimpl->button_inactive_color = ImGui::GetStyle().Colors[ImGuiCol_Button];
    m_pimpl->button_active_color = ImVec4(0.7f, 0.5f, 0.3f, 1.0f);

    int ws = m_pimpl->axes_settings.axes.find(Axis::WS) != m_pimpl->axes_settings.axes.end();
    int ad = m_pimpl->axes_settings.axes.find(Axis::AD) != m_pimpl->axes_settings.axes.end();
    int up_down = m_pimpl->axes_settings.axes.find(Axis::UP_DOWN) != m_pimpl->axes_settings.axes.end();
    int left_right = m_pimpl->axes_settings.axes.find(Axis::LEFT_RIGHT) != m_pimpl->axes_settings.axes.end();
    int number_of_axes = ws + ad + up_down + left_right;

    m_pimpl->axes_values.resize(static_cast<size_t>(number_of_axes), 0.0);
    m_pimpl->sticks_to_axes.clear();

    if (ws || ad)
    {
        m_pimpl->sticks_to_axes.emplace_back();
        m_pimpl->sticks_values.emplace_back();
        ButtonsTable wasd;
        wasd.second = ws + 2 * ad; //Number of columns
        if (ws)
        {
            AxisSettings& ws_settings = m_pimpl->axes_settings.axes[Axis::WS];
            int sign = ws_settings.sign;
            size_t index = ws_settings.index;
            wasd.first.push_back(ButtonsMap({ {"W", {.key = ImGuiKey_W, .col = ad, .sign = -sign, .index = index}} }));
        }
        if (ad)
        {
            AxisSettings& ws_settings = m_pimpl->axes_settings.axes[Axis::AD];
            int sign = ws_settings.sign;
            size_t index = ws_settings.index;
            m_pimpl->sticks_to_axes.back().push_back(index);
            m_pimpl->sticks_values.back().push_back(0);
            wasd.first.push_back(ButtonsMap({ {"A", {.key = ImGuiKey_A, .col = 0, .sign = -sign, .index = index}},
                                                   {"D", {.key = ImGuiKey_D, .col = 1 + ws, .sign = sign, .index = index}} }));
        }
        if (ws)
        {
            AxisSettings& ws_settings = m_pimpl->axes_settings.axes[Axis::WS];
            int sign = ws_settings.sign;
            size_t index = ws_settings.index;
            m_pimpl->sticks_to_axes.back().push_back(index);
            m_pimpl->sticks_values.back().push_back(0);
            wasd.first.push_back(ButtonsMap({ {"S", {.key = ImGuiKey_S, .col = ad, .sign = sign, .index = index}} }));
        }

        m_pimpl->sticks.push_back(std::make_pair(m_pimpl->axes_settings.wasd_label, wasd));
    }

    if (up_down || left_right)
    {
        m_pimpl->sticks_to_axes.emplace_back();
        m_pimpl->sticks_values.emplace_back();
        ButtonsTable arrows;
        arrows.second = up_down + 2 * left_right; //Number of columns
        if (up_down)
        {
            AxisSettings& ws_settings = m_pimpl->axes_settings.axes[Axis::UP_DOWN];
            int sign = ws_settings.sign;
            size_t index = ws_settings.index;
            arrows.first.push_back(ButtonsMap({{"top", {.key = ImGuiKey_UpArrow, .col = left_right, .sign = -sign, .index = index}}}));
        }
        if (left_right)
        {
            AxisSettings& ws_settings = m_pimpl->axes_settings.axes[Axis::LEFT_RIGHT];
            int sign = ws_settings.sign;
            size_t index = ws_settings.index;
            m_pimpl->sticks_to_axes.back().push_back(index);
            m_pimpl->sticks_values.back().push_back(0);
            arrows.first.push_back(ButtonsMap({{"left", {.key = ImGuiKey_LeftArrow, .col = 0, .sign = -sign, .index = index}},
                                                    {"right", {.key = ImGuiKey_RightArrow, .col = 1 + up_down, .sign = sign, .index = index}}}));
        }
        if (up_down)
        {
            AxisSettings& ws_settings = m_pimpl->axes_settings.axes[Axis::UP_DOWN];
            int sign = ws_settings.sign;
            size_t index = ws_settings.index;
            m_pimpl->sticks_to_axes.back().push_back(index);
            m_pimpl->sticks_values.back().push_back(0);
            arrows.first.push_back(ButtonsMap({ {"bottom", {.key = ImGuiKey_DownArrow, .col = left_right, .sign  = sign, .index = index}} }));
        }

        m_pimpl->sticks.push_back(std::make_pair(m_pimpl->axes_settings.arrows_label, arrows));
    }

    return true;
}

void yarp::dev::KeyboardJoypad::threadRelease()
{
    if (m_pimpl->closed)
        return;

    std::lock_guard<std::mutex> lock(m_pimpl->mutex);

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    if (m_pimpl->window)
    {
        glfwDestroyWindow(m_pimpl->window);
        glfwTerminate();
        m_pimpl->window = nullptr;
    }

    m_pimpl->closed = true;
}

void yarp::dev::KeyboardJoypad::run()
{
    if (m_pimpl->closed)
    {
        return;
    }
    {
        std::lock_guard<std::mutex> lock(m_pimpl->mutex);
        m_pimpl->need_to_close = glfwWindowShouldClose(m_pimpl->window);
    }
    double period = 0;
    double desired_period = 0;

    if (!m_pimpl->need_to_close)
    {
        std::lock_guard<std::mutex> lock(m_pimpl->mutex);

        glfwPollEvents();

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        for (double& value : m_pimpl->axes_values)
        {
            value = 0;
        }

        ImVec2 position(m_pimpl->settings.button_size, m_pimpl->settings.button_size);

        for (auto& stick : m_pimpl->sticks)
        {
            m_pimpl->renderButtonsTable(stick.first, stick.second, position);
            position.x += 4 * m_pimpl->settings.button_size; // Move the next table to the right (3 columns + 1 space)
        }
        position.x = m_pimpl->settings.button_size;
        position.y += 4 * m_pimpl->settings.button_size; // Move the next table down (3 rows + 1 space)

        for (size_t i = 0; i < m_pimpl->sticks_to_axes.size(); ++i)
        {
            for (size_t j = 0; j < m_pimpl->sticks_to_axes[i].size(); j++)
            {
                m_pimpl->sticks_values[i][j] = m_pimpl->axes_values[m_pimpl->sticks_to_axes[i][j]];
            }
        }

        ImGui::SetNextWindowPos(position, ImGuiCond_FirstUseEver);
        ImGui::Begin("Settings", 0, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings);
        ImGui::SetWindowFontScale(m_pimpl->settings.font_multiplier);
        ImGuiIO& io = ImGui::GetIO();
        period = io.DeltaTime;
        ImGui::Text("Application average %.1f ms/frame (%.1f FPS)", period * 1000.0f, io.Framerate);

        int width, height;
        glfwGetWindowSize(m_pimpl->window, &width, &height);

        ImGui::Text("Window size: %d x %d", width, height);
        ImGui::SliderFloat("Button size", &m_pimpl->settings.button_size, m_pimpl->settings.min_button_size, m_pimpl->settings.max_button_size);
        ImGui::SliderFloat("Font multiplier", &m_pimpl->settings.font_multiplier, m_pimpl->settings.min_font_multiplier, m_pimpl->settings.max_font_multiplier);
        ImGui::End();

        // Rendering
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(m_pimpl->window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(m_pimpl->clear_color.x * m_pimpl->clear_color.w, m_pimpl->clear_color.y * m_pimpl->clear_color.w, m_pimpl->clear_color.z * m_pimpl->clear_color.w, m_pimpl->clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(m_pimpl->window);

        desired_period = getPeriod();

    }
    else
    {
        this->close();
    }

    if (period > desired_period)
    {
        yCWarningThrottle(KEYBOARDJOYPAD, 5.0, "The period of the GUI is higher than the period of the thread. The GUI will be updated at a lower rate.");
        yarp::os::Time::delay(1e-3); //Sleep for 1 ms to avoid the other threads to go to starvation
    }


}

bool yarp::dev::KeyboardJoypad::startService()
{
    //To let the device driver knowing that it need to poll updateService continuosly
    return false;
}

bool yarp::dev::KeyboardJoypad::updateService()
{
    //To let the device driver that we are still alive
    return !m_pimpl->closed;
}

bool yarp::dev::KeyboardJoypad::stopService()
{
    yCInfo(KEYBOARDJOYPAD) << "Stopping the service";
    return this->close();
}

bool yarp::dev::KeyboardJoypad::getAxisCount(unsigned int& axis_count)
{
    std::lock_guard<std::mutex> lock(m_pimpl->mutex);
    axis_count = static_cast<unsigned int>(m_pimpl->axes_values.size());
    return true;
}

bool yarp::dev::KeyboardJoypad::getButtonCount(unsigned int& button_count)
{
    std::lock_guard<std::mutex> lock(m_pimpl->mutex);
    button_count = 0;
    return true;
}

bool yarp::dev::KeyboardJoypad::getTrackballCount(unsigned int& trackball_count)
{
    trackball_count = 0;
    return true;
}

bool yarp::dev::KeyboardJoypad::getHatCount(unsigned int& hat_count)
{
    hat_count = 0;
    return true;
}

bool yarp::dev::KeyboardJoypad::getTouchSurfaceCount(unsigned int& touch_count)
{
    touch_count = 0;
    return true;
}

bool yarp::dev::KeyboardJoypad::getStickCount(unsigned int& stick_count)
{
    std::lock_guard<std::mutex> lock(m_pimpl->mutex);
    stick_count = static_cast<unsigned int>(m_pimpl->sticks_to_axes.size());
    return true;
}

bool yarp::dev::KeyboardJoypad::getStickDoF(unsigned int stick_id, unsigned int& dof)
{
    std::lock_guard<std::mutex> lock(m_pimpl->mutex);
    if (stick_id >= m_pimpl->sticks_to_axes.size())
    {
        yCError(KEYBOARDJOYPAD) << "The stick with id" << stick_id << "does not exist.";
        return false;
    }

    dof = static_cast<unsigned int>(m_pimpl->sticks_to_axes[stick_id].size());

    return true;
}

bool yarp::dev::KeyboardJoypad::getButton(unsigned int button_id, float& value)
{
    return false; //TODO
}

bool yarp::dev::KeyboardJoypad::getTrackball(unsigned int /*trackball_id*/, yarp::sig::Vector& /*value*/)
{
    yCError(KEYBOARDJOYPAD) << "This device does not consider trackballs.";
    return false;
}

bool yarp::dev::KeyboardJoypad::getHat(unsigned int /*hat_id*/, unsigned char& /*value*/)
{
    yCError(KEYBOARDJOYPAD) << "This device does not consider hats.";
    return false;
}

bool yarp::dev::KeyboardJoypad::getAxis(unsigned int axis_id, double& value)
{
    std::lock_guard<std::mutex> lock(m_pimpl->mutex);
    if (axis_id >= m_pimpl->axes_values.size())
    {
        yCError(KEYBOARDJOYPAD) << "The axis with id" << axis_id << "does not exist.";
        return false;
    }
    value = m_pimpl->axes_values[axis_id];
    return true;
}

bool yarp::dev::KeyboardJoypad::getStick(unsigned int stick_id, yarp::sig::Vector& value, JoypadCtrl_coordinateMode coordinate_mode)
{
    std::lock_guard<std::mutex> lock(m_pimpl->mutex);
    if (stick_id >= m_pimpl->sticks_values.size())
    {
        yCError(KEYBOARDJOYPAD) << "The stick with id" << stick_id << "does not exist.";
        return false;
    }

    value.resize(m_pimpl->sticks_values[stick_id].size());

    for (size_t i = 0; i < value.size(); i++)
    {
        value[i] = m_pimpl->sticks_values[stick_id][i];
    }

    if (value.size() != 2)
    {
        return true;
    }

    if (coordinate_mode == JoypadCtrl_coordinateMode::JypCtrlcoord_POLAR)
    {
        double norm = sqrt(value[0] * value[0] + value[1] * value[1]);
        double angle = atan2(value[1], value[0]);
        value[0] = norm;
        value[1] = angle;
    }

    return true;
}

bool yarp::dev::KeyboardJoypad::getTouch(unsigned int /*touch_id*/, yarp::sig::Vector& /*value*/)
{
    yCError(KEYBOARDJOYPAD) << "This device does not consider touch surfaces.";
    return false;
}
