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

#include <yarp/os/LogStream.h>

#include <KeyboardJoypad.h>
#include <KeyboardJoypadLogComponent.h>

class yarp::dev::KeyboardJoypad::Impl
{
public:
    GLFWwindow* window = nullptr;

    std::atomic_bool need_to_close{false}, closed{false};

    std::mutex mutex;

    //---TO BE REMOVED
    bool show_demo_window = true;
    bool show_another_window = false;
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
    //---

    static void GLMessageCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei, const GLchar* message, const void*) {
        yCError(KEYBOARDJOYPAD, "GL CALLBACK: %s source = 0x%x, type = 0x%x, id = 0x%x, severity = 0x%x, message = %s",
            (type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : ""),
            source, type, id, severity, message);
    }

    static void glfwErrorCallback(int error, const char* description) {
        yCError(KEYBOARDJOYPAD, "GLFW error %d: %s", error, description);
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

    m_pimpl->window = glfwCreateWindow(1280, 720, "YARP OpenXr Device Window", nullptr, nullptr);
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
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(m_pimpl->window, true);
    ImGui_ImplOpenGL3_Init();

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
        // Poll and handle events (inputs, window resize, etc.)
                // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
                // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
                // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
                // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
        glfwPollEvents();

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
        if (m_pimpl->show_demo_window)
            ImGui::ShowDemoWindow(&m_pimpl->show_demo_window);

        // 2. Show a simple window that we create ourselves. We use a Begin/End pair to create a named window.
        {
            static float f = 0.0f;
            static int counter = 0;

            ImGui::Begin("Hello, world!");                          // Create a window called "Hello, world!" and append into it.

            ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)
            ImGui::Checkbox("Demo Window", &m_pimpl->show_demo_window);      // Edit bools storing our window open/close state
            ImGui::Checkbox("Another Window", &m_pimpl->show_another_window);

            ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f
            ImGui::ColorEdit3("clear color", (float*)&m_pimpl->clear_color); // Edit 3 floats representing a color

            if (ImGui::Button("Button"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
                counter++;
            ImGui::SameLine();
            ImGui::Text("counter = %d", counter);

            ImGuiIO& io = ImGui::GetIO();
            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
            ImGui::End();
        }

        // 3. Show another simple window.
        if (m_pimpl->show_another_window)
        {
            ImGui::Begin("Another Window", &m_pimpl->show_another_window);   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
            ImGui::Text("Hello from another window!");
            if (ImGui::Button("Close Me"))
                m_pimpl->show_another_window = false;
            ImGui::End();
        }

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
