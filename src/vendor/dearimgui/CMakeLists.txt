# Copyright (C) 2024 Istituto Italiano di Tecnologia (IIT)
# All rights reserved.
#
# This software may be modified and distributed under the terms of the
# BSD-2-Clause license. See the accompanying LICENSE file for details.

if (NOT USE_VENDORED_IMGUI)
    find_package(imgui REQUIRED)
    return()
endif()

include(FetchContent)

set(imgui_TAG v1.90.4)
set(imgui-feedstock_TAG 7f402e0ac895620420310a7e4791860a2dc6e79d)
set(IMGUI_BUILD_GLFW_BINDING ON)
set(IMGUI_BUILD_OPENGL3_BINDING ON)

FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui
    GIT_TAG        ${imgui_TAG}
    GIT_SHALLOW 1
)

FetchContent_GetProperties(imgui)
if(NOT imgui_POPULATED)
    message(STATUS "Fetching imgui(${imgui_TAG})...")
    FetchContent_Populate(imgui)
    set(CMAKELIST_URL "https://raw.githubusercontent.com/conda-forge/imgui-feedstock/${imgui-feedstock_TAG}/recipe")
    message (STATUS "Downloading ${CMAKELIST_URL}/CMakeLists.txt to ${imgui_SOURCE_DIR}/CMakeLists.txt")
    file(DOWNLOAD "${CMAKELIST_URL}/CMakeLists.txt" "${imgui_SOURCE_DIR}/CMakeLists.txt")
    message (STATUS "Downloading ${CMAKELIST_URL}/imgui-config.cmake.in to ${imgui_SOURCE_DIR}/imgui-config.cmake.in")
    file(DOWNLOAD "${CMAKELIST_URL}/imgui-config.cmake.in" "${imgui_SOURCE_DIR}/imgui-config.cmake.in")
    add_subdirectory(${imgui_SOURCE_DIR} ${imgui_BINARY_DIR})
endif()
