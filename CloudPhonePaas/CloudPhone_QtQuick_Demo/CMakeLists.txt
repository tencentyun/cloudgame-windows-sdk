cmake_minimum_required(VERSION 3.16)

project(CloudPhone_QtQuick VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(QT_NO_PRIVATE_MODULE_WARNING ON)

# =============================================================
# Qt 依赖
# =============================================================
find_package(Qt6 REQUIRED COMPONENTS
    Quick Network Concurrent ShaderTools
    GuiPrivate QuickControls2 QuickLayouts QuickDialogs2
)

qt_standard_project_setup(REQUIRES 6.8)

# =============================================================
# 源文件收集
# =============================================================
file(GLOB_RECURSE SRC_CPP_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "src/*.cpp")

# 排除平台特定文件
if(NOT WIN32)
    list(FILTER SRC_CPP_FILES EXCLUDE REGEX ".*CrashDumpHandler\\.cpp$")
endif()

# =============================================================
# 可执行目标
# =============================================================
qt_add_executable(${PROJECT_NAME}
    ${SRC_CPP_FILES}
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/utils
)

# =============================================================
# 着色器资源
# =============================================================
qt6_add_shaders(${PROJECT_NAME} "shaders"
    BATCHABLE
    PRECOMPILE
    OPTIMIZED
    PREFIX "/shaders"
    FILES
        "shaders/yuv.vert"
        "shaders/yuv.frag"
)

# =============================================================
# QML 模块
# =============================================================
file(GLOB_RECURSE QML_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "qml/*.qml")

qt_add_qml_module(${PROJECT_NAME}
    URI ${PROJECT_NAME}
    VERSION 1.0
    OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}
    QML_FILES ${QML_FILES}
)

# =============================================================
# 目标属性（跨平台通用）
# =============================================================
set_target_properties(${PROJECT_NAME} PROPERTIES
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

# Linux: 将可执行文件输出到 bin/ 子目录，避免与 QML 模块的同名目录冲突
if(UNIX AND NOT APPLE)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/bin"
    )
endif()

# macOS: Info.plist 隐私权限声明（摄像头/麦克风 TCC）
if(APPLE)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/Info.plist.in"
    )
endif()

# =============================================================
# Qt 模块链接
# =============================================================
target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Quick
    Qt6::Network
    Qt6::Concurrent
    Qt6::GuiPrivate
    Qt6::QuickControls2
    Qt6::QuickLayouts
    Qt6::QuickDialogs2
)

# =============================================================
# TcrSdk 动态库/静态库配置（按平台分文件）
# =============================================================
set(TCRSDK_BASE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/third_party/TcrSdk)

if(WIN32)
    include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/TcrSdkWindows.cmake)
elseif(APPLE)
    include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/TcrSdkMacOS.cmake)
else()
    include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/TcrSdkLinux.cmake)
endif()

# =============================================================
# 复制 config.json 到可执行文件目录（如果存在）
# =============================================================
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/config.json")
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/config.json"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>/config.json"
        COMMENT "Copying config.json to output directory"
        VERBATIM
    )
endif()

# =============================================================
# 安装与部署配置
# =============================================================
include(GNUInstallDirs)

install(TARGETS ${PROJECT_NAME}
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

qt_generate_deploy_qml_app_script(
    TARGET ${PROJECT_NAME}
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})
