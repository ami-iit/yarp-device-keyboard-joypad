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

#include <yarp/os/LogStream.h>

#include <KeyboardJoypad.h>
#include <KeyboardJoypadLogComponent.h>


struct ButtonState {
    ImGuiKey key = ImGuiKey_COUNT;
    int col = 0;
    bool active = false;
    bool buttonPressed = false;
};

using ButtonsMap = std::unordered_map<std::string, ButtonState>;
using ButtonsTable = std::vector<ButtonsMap>;

class yarp::dev::KeyboardJoypad::Impl
{
public:
    GLFWwindow* window = nullptr;

    std::atomic_bool need_to_close{false}, closed{false};

    std::mutex mutex;

    ImVec4 button_inactive_color;
    ImVec4 button_active_color;
    float button_size = 100;
    float min_button_size = 50;
    float max_button_size = 200;
    float font_multiplier = 1.0;
    float min_font_multiplier = 0.5;
    float max_font_multiplier = 4.0;

    std::vector<std::pair<std::string, ButtonsTable>> sticks;

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
        ImVec2 buttonSize(button_size, button_size);

        const int n_cols = 3;
        ImGui::SetNextWindowPos(position, ImGuiCond_FirstUseEver);
        ImGui::Begin(name.c_str(), 0, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings);
        ImGui::SetWindowFontScale(font_multiplier);

        ImGui::BeginTable(name.c_str(), n_cols, ImGuiTableFlags_NoSavedSettings | ImGuiTableFlags_SizingMask_);
        for (auto& row : buttons_table)
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
    yarp::os::PeriodicThread(0.01, yarp::os::ShouldUseSystemClock::Yes)
{
    m_pimpl = std::make_unique<Impl>();
}

yarp::dev::KeyboardJoypad::~KeyboardJoypad()
{
    this->stop();
}

bool yarp::dev::KeyboardJoypad::open(yarp::os::Searchable& cfg)
{
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

    m_pimpl->window = glfwCreateWindow(1280, 720, "YARP Keyboard as Joypad Device Window", nullptr, nullptr);
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

    ButtonsTable wasd;
    wasd.push_back(ButtonsMap({ {"W", {.key = ImGuiKey_W, .col = 1}} }));
    wasd.push_back(ButtonsMap({ {"A", {.key = ImGuiKey_A, .col = 0}}, {"D", {.key = ImGuiKey_D, .col = 2}} }));
    wasd.push_back(ButtonsMap({ {"S", {.key = ImGuiKey_S, .col = 1}} }));

    m_pimpl->sticks.push_back(std::make_pair("WASD", wasd));

    ButtonsTable arrows;
    arrows.push_back(ButtonsMap({{"top", {.key = ImGuiKey_UpArrow, .col = 1}}}));
    arrows.push_back(ButtonsMap({{"left", {.key = ImGuiKey_LeftArrow, .col = 0}}, {"right", {.key = ImGuiKey_RightArrow, .col = 2}}}));
    arrows.push_back(ButtonsMap({ {"bottom", {.key = ImGuiKey_DownArrow, .col = 1}} }));

    m_pimpl->sticks.push_back(std::make_pair("Arrows", arrows));

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

    if (!m_pimpl->need_to_close)
    {
        std::lock_guard<std::mutex> lock(m_pimpl->mutex);

        glfwPollEvents();

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImVec2 position(m_pimpl->button_size, m_pimpl->button_size);

        for (auto& stick : m_pimpl->sticks)
        {
            m_pimpl->renderButtonsTable(stick.first, stick.second, position);
            position.x += 4 * m_pimpl->button_size; // Move the next table to the right (3 columns + 1 space)
        }
        position.x = m_pimpl->button_size;
        position.y += 4 * m_pimpl->button_size; // Move the next table down (3 rows + 1 space)

        ImGui::SetNextWindowPos(position, ImGuiCond_FirstUseEver);
        ImGui::Begin("Settings", 0, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings);
        ImGui::SetWindowFontScale(m_pimpl->font_multiplier);
        ImGuiIO& io = ImGui::GetIO();
        ImGui::Text("Application average %.1f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
        ImGui::SliderFloat("Button size", &m_pimpl->button_size, m_pimpl->min_button_size, m_pimpl->max_button_size);
        ImGui::SliderFloat("Font multiplier", &m_pimpl->font_multiplier, m_pimpl->min_font_multiplier, m_pimpl->max_font_multiplier);
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

    }
    else
    {
        this->close();
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
    return false;
}

bool yarp::dev::KeyboardJoypad::getButtonCount(unsigned int& button_count)
{
    return false;
}

bool yarp::dev::KeyboardJoypad::getTrackballCount(unsigned int& trackball_count)
{
    return false;
}

bool yarp::dev::KeyboardJoypad::getHatCount(unsigned int& hat_count)
{
    return false;
}

bool yarp::dev::KeyboardJoypad::getTouchSurfaceCount(unsigned int& touch_count)
{
    return false;
}

bool yarp::dev::KeyboardJoypad::getStickCount(unsigned int& stick_count)
{
    return false;
}

bool yarp::dev::KeyboardJoypad::getStickDoF(unsigned int stick_id, unsigned int& dof)
{
    return false;
}

bool yarp::dev::KeyboardJoypad::getButton(unsigned int button_id, float& value)
{
    return false;
}

bool yarp::dev::KeyboardJoypad::getTrackball(unsigned int trackball_id, yarp::sig::Vector& value)
{
    return false;
}

bool yarp::dev::KeyboardJoypad::getHat(unsigned int hat_id, unsigned char& value)
{
    return false;
}

bool yarp::dev::KeyboardJoypad::getAxis(unsigned int axis_id, double& value)
{
    return false;
}

bool yarp::dev::KeyboardJoypad::getStick(unsigned int stick_id, yarp::sig::Vector& value, JoypadCtrl_coordinateMode coordinate_mode)
{
    return false;
}

bool yarp::dev::KeyboardJoypad::getTouch(unsigned int touch_id, yarp::sig::Vector& value)
{
    return false;
}
