cmake_minimum_required(VERSION 3.16)

project(ImGui_Demo VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# =============================================================
# 第三方依赖
# =============================================================
include(FetchContent)

# --- ImGui ---
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG        v1.91.8-docking
)
FetchContent_MakeAvailable(imgui)

# --- nlohmann_json ---
FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG        v3.11.3
)
set(JSON_BuildTests OFF CACHE INTERNAL "")
FetchContent_MakeAvailable(nlohmann_json)

# --- SDL2 ---
find_package(SDL2 REQUIRED)

# =============================================================
# ImGui 库（源码编译）
# =============================================================
set(IMGUI_DIR ${imgui_SOURCE_DIR})

set(IMGUI_SOURCES
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    # SDL2 后端
    ${IMGUI_DIR}/backends/imgui_impl_sdl2.cpp
)

# 平台特定的 ImGui 后端
if(WIN32)
    list(APPEND IMGUI_SOURCES
        ${IMGUI_DIR}/backends/imgui_impl_dx11.cpp
    )
elseif(APPLE)
    list(APPEND IMGUI_SOURCES
        ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
    )
else()
    list(APPEND IMGUI_SOURCES
        ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
    )
endif()

add_library(imgui_lib STATIC ${IMGUI_SOURCES})
target_include_directories(imgui_lib PUBLIC
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
)
target_link_libraries(imgui_lib PUBLIC SDL2::SDL2)

if(WIN32)
    target_link_libraries(imgui_lib PUBLIC d3d11 dxgi d3dcompiler)
elseif(APPLE)
    find_package(OpenGL REQUIRED)
    target_link_libraries(imgui_lib PUBLIC OpenGL::GL)
    target_compile_definitions(imgui_lib PUBLIC GL_SILENCE_DEPRECATION)
else()
    find_package(OpenGL REQUIRED)
    target_link_libraries(imgui_lib PUBLIC OpenGL::GL)
endif()

# =============================================================
# 源文件
# =============================================================
set(SOURCES
    src/main.cpp
    src/app.cpp
    src/config.cpp
    src/video_renderer.cpp
    src/http_client.cpp
)

# =============================================================
# 可执行目标
# =============================================================
if(WIN32)
    add_executable(${PROJECT_NAME} WIN32 ${SOURCES})
elseif(APPLE)
    add_executable(${PROJECT_NAME} MACOSX_BUNDLE ${SOURCES})
    set_target_properties(${PROJECT_NAME} PROPERTIES
        MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
        MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    )
else()
    add_executable(${PROJECT_NAME} ${SOURCES})
endif()

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    imgui_lib
    SDL2::SDL2
    nlohmann_json::nlohmann_json
)

# 平台特定链接
if(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE d3d11 dxgi d3dcompiler)
    target_compile_definitions(${PROJECT_NAME} PRIVATE RENDERER_D3D11)
elseif(APPLE)
    find_package(OpenGL REQUIRED)
    target_link_libraries(${PROJECT_NAME} PRIVATE OpenGL::GL)
    target_compile_definitions(${PROJECT_NAME} PRIVATE RENDERER_OPENGL)
else()
    find_package(OpenGL REQUIRED)
    target_link_libraries(${PROJECT_NAME} PRIVATE OpenGL::GL)
    target_compile_definitions(${PROJECT_NAME} PRIVATE RENDERER_OPENGL)
endif()

# SDL2 main 入口（Windows 需要）
if(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE SDL2::SDL2main)
endif()

# =============================================================
# TcrSdk 动态库配置（按平台分文件）
# =============================================================
set(TCRSDK_BASE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/third_party/TcrSdk)

if(WIN32)
    include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/TcrSdkWindows.cmake)
elseif(APPLE)
    include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/TcrSdkMacOS.cmake)
endif()

# =============================================================
# 复制 config.json 到可执行文件目录
# =============================================================
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/config.json"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>/config.json"
    COMMENT "Copying config.json to output directory"
    VERBATIM
)

# macOS: SDL_GetBasePath() 返回 Resources/ 目录，需要额外拷贝一份
if(APPLE)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory
            "$<TARGET_BUNDLE_DIR:${PROJECT_NAME}>/Contents/Resources"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/config.json"
            "$<TARGET_BUNDLE_DIR:${PROJECT_NAME}>/Contents/Resources/config.json"
        COMMENT "Copying config.json to app bundle Resources"
        VERBATIM
    )
endif()

# =============================================================
# 安装配置
# =============================================================
include(GNUInstallDirs)

install(TARGETS ${PROJECT_NAME}
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/config.json"
    DESTINATION ${CMAKE_INSTALL_BINDIR}
)
