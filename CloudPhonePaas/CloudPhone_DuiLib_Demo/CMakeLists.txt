cmake_minimum_required(VERSION 3.21)
project(CloudPhone_DuiLib_Demo LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# =============================================================
# DuiLib (static library, built from source via add_subdirectory)
# =============================================================
add_subdirectory(third_party/duilib)

# =============================================================
# TcrSdk base directory (used by cmake/TcrSdkWindows.cmake)
# =============================================================
set(TCRSDK_BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/TcrSdk")

# =============================================================
# vcpkg-managed dependencies
# =============================================================
find_package(CURL CONFIG REQUIRED)
find_package(jsoncpp CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(libyuv CONFIG REQUIRED)

# =============================================================
# Source files
# =============================================================
set(SOURCES
    src/main.cpp
    src/windows/LoginWindow.cpp
    src/windows/MainWindow.cpp
    src/windows/StreamingWindow.cpp
    src/windows/dialogs/BatchDialog.cpp
    src/windows/dialogs/ConfirmDialog.cpp
    src/windows/dialogs/InputDialogs.cpp
    src/windows/dialogs/ResultDialog.cpp
    src/utils/Logger.cpp
    src/utils/CrashDumpHandler.cpp
    src/utils/InstanceImageDownloader.cpp
    src/services/NetworkService.cpp
    src/services/ApiService.cpp
    src/viewmodels/AndroidInstanceModel.cpp
    src/viewmodels/StreamingViewModel.cpp
    src/viewmodels/BatchTaskOperatorModel.cpp
    src/core/StreamConfig.cpp
    src/core/BatchTaskOperator.cpp
    src/core/video/VideoRenderer.cpp
    src/core/video/VideoTransformHelper.cpp
)

set(HEADERS
    src/windows/LoginWindow.h
    src/windows/MainWindow.h
    src/windows/StreamingWindow.h
    src/windows/dialogs/BatchDialog.h
    src/windows/dialogs/ConfirmDialog.h
    src/windows/dialogs/InputDialogs.h
    src/windows/dialogs/ResultDialog.h
    src/utils/Logger.h
    src/utils/CrashDumpHandler.h
    src/utils/InstanceImageDownloader.h
    src/utils/ImageCache.h
    src/utils/StringUtils.h
    src/utils/UiThreadHelper.h
    src/services/NetworkService.h
    src/services/ApiService.h
    src/viewmodels/AndroidInstanceModel.h
    src/viewmodels/StreamingViewModel.h
    src/viewmodels/BatchTaskOperatorModel.h
    src/core/StreamConfig.h
    src/core/BatchTaskOperator.h
    src/core/video/Frame.h
    src/core/video/VideoRenderer.h
    src/core/video/VideoTransformHelper.h
)

# =============================================================
# Executable (WIN32 = no console window)
# =============================================================
add_executable(${PROJECT_NAME} WIN32 ${SOURCES} ${HEADERS})

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# =============================================================
# Link libraries
# =============================================================
target_link_libraries(${PROJECT_NAME} PRIVATE
    duilib
    CURL::libcurl
    JsonCpp::JsonCpp
    spdlog::spdlog
    yuv
    dbghelp
    comctl32
)

# =============================================================
# TcrSdk (Windows platform config — include/link/copy DLL)
# =============================================================
include(cmake/TcrSdkWindows.cmake)

# MSVC runtime: match vcpkg x64-windows-static-md (use /MD)
if(MSVC)
    set_property(TARGET ${PROJECT_NAME} PROPERTY
        MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL"
    )
endif()

# =============================================================
# Copy skin files to output directory
# =============================================================
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/skin"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>/skin"
    COMMENT "Copying skin files to output directory"
)
