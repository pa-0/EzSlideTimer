/*
 * main.cpp
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
 * Copyright 2022 Winterreisender.
 * This file is part of the EzSlideTimer.
 * This program is free software: you can redistribute it and/or modify it under the terms of the GNU Affero General Public License as published by the Free Software Foundation, only version 3 of the License.
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for more details.
 * You should have received a copy of the GNU Affero General Public License along with this program, see the file LICENSE. If not, see https://www.gnu.org/licenses/.
 */

#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>

#include <GLFW/glfw3.h>
#define GL_SILENCE_DEPRECATION
#if defined(IMGUI_IMPL_OPENGL_ES2)
#include <GLES2/gl2.h>
#endif
#include <gl/glew.h>

#if defined(_MSC_VER) && (_MSC_VER >= 1900) && !defined(IMGUI_DISABLE_WIN32_FUNCTIONS)
#pragma comment(lib, "legacy_stdio_definitions")
#endif

#include <iostream>
#include <thread>
#include <chrono>
using namespace std;

constexpr auto AppName = "ShowTimer";

int main(int, char **)
{
    constexpr int windowHeightCompact = 35;
    constexpr int windowHeightFull = 42;
    constexpr int windowWidthFull = 270;
    const auto windowColorFull = ImVec4(.9f, .9f, .9f, 1.0f);
    constexpr int windowWidthCompact = 80;
    const auto windowColorCompact = ImVec4(1.0f, 1.0f, 1.0f, 0.5f);
    constexpr int fpsIntervalCompact = 12; // 1/12 vertical sync 60fp screen -> 5fps program
    constexpr int fpsIntervalFull = 1;     // vertical sync

    glfwSetErrorCallback(
        [](int error, const char *description)
        { cerr << "Glfw Error" << error << ":" << description << endl; });

    assert(glfwInit());

// Decide GL+GLSL versions
#if defined(IMGUI_IMPL_OPENGL_ES2)
    // GL ES 2.0 + GLSL 100
    const char *glsl_version = "#version 100";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
#elif defined(__APPLE__)
    // GL 3.2 + GLSL 150
    const char *glsl_version = "#version 150";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); // 3.2+ only
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);           // Required on Mac
#else
    // GL 3.0 + GLSL 130
    const char *glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    // glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
    // glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // 3.0+ only
#endif

    // Windows OS: borderless, transparent interface
    glfwWindowHint(GLFW_DECORATED, false);
    glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, GLFW_TRUE);

    // Create window
    GLFWwindow *window = glfwCreateWindow(windowWidthFull, windowHeightFull, AppName, NULL, NULL);
    assert(window);

    glfwMakeContextCurrent(window);
    glfwSwapInterval(fpsIntervalCompact); // 1/12 vertical sync frame rate 60fps->5fps

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();

    ImGui::StyleColorsLight();

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // Set default font
    //ImFont *font = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\MSYH.ttc", 21.0f, NULL,
                                               // io.Fonts->GetGlyphRangesChineseSimplifiedCommon());
    // IM_ASSERT(font != NULL);

    // Set default background color
    ImVec4 clear_color = windowColorCompact; // Fully transparent

    // Timer thread
    chrono::steady_clock::time_point beginTime = chrono::steady_clock::now(); // UI write, timer read
    chrono::steady_clock::duration offset = chrono::nanoseconds(0ll);         // UI write, timer read
    long long timerCount = 0ul;                                               // Timer write, UI read/write

    //Three states
    enum class TimerState
    {
        RUNNING,
        PAUSED,
        CANCELED
    } timerState = TimerState::RUNNING;

    std::thread timerThread([&beginTime, &offset, &timerCount, window, &timerState]()
                            {
        while (timerState != TimerState::CANCELED)
        {
            std::this_thread::sleep_for(chrono::milliseconds(500ll));
            
            switch (timerState)
            {
            case TimerState::RUNNING:
                timerCount = chrono::duration_cast<chrono::seconds>(chrono::steady_clock::now() - beginTime + offset).count();
                break;
            case TimerState::PAUSED:
                continue;
            case TimerState::CANCELED:
                return;
            }
            glfwSetWindowAttrib(window, GLFW_FLOATING, GLFW_TRUE);
        } });
    timerThread.detach();

    // main loop
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Imgui populates window
        const ImGuiViewport *viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->Pos);
        ImGui::SetNextWindowSize(viewport->Size);

        // UI part
        {
            ImGui::Begin("ShowTimer", NULL,
                         ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoMove |
                             ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings);

            ImGui::SetNextItemWidth(90.0f);
            ImGui::Text("%lld:%2.2lld:%2.2lld", timerCount / 3600, timerCount % 3600 / 60, timerCount % 60);
            ImGui::SameLine();
            // printf("Average FPS: %.1f\r", ImGui::GetIO().Framerate);

            // when mouse hovers over window
            if (glfwGetWindowAttrib(window, GLFW_HOVERED))
            {
                glfwSwapInterval(fpsIntervalFull); // increase frame rate
                glfwSetWindowSize(window, windowWidthFull, windowHeightFull);
                clear_color = windowColorFull;

                ImGui::SameLine();
                if (timerState == TimerState::RUNNING && ImGui::Button("Pause"))
                {
                    timerState = TimerState::PAUSED;
                    offset += chrono::steady_clock::now() - beginTime;
                    // cout << chrono::duration_cast<chrono::seconds>(offset).count() << endl;
                }
                else if (timerState == TimerState::PAUSED && ImGui::Button("Start"))
                {
                    beginTime = chrono::steady_clock::now();
                    timerState = TimerState::RUNNING;
                }

                ImGui::SameLine();
                if (ImGui::Button("Clear"))
                {
                    timerCount = 0ul;
                    beginTime = chrono::steady_clock::now();
                    offset = chrono::nanoseconds(0ll);
                }

                ImGui::SameLine();
                if (ImGui::Button("About"))
                {
                    system("start https://github.com/Winterreisender/EzSlideTimer");
                }

                ImGui::SameLine();
                if (ImGui::Button("Exit"))
                {
                    timerState = TimerState::CANCELED;
                    glfwSetWindowShouldClose(window, true);
                }
            }
            else
            {
                clear_color = windowColorCompact; // transparent
                glfwSetWindowSize(window, windowWidthCompact, windowHeightCompact);
                glfwSwapInterval(12); // reduce frame rate
            }

            ImGui::End();
        }

        // rendering
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w,
                     clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
