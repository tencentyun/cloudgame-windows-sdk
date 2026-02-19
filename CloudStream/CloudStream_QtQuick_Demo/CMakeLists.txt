cmake_minimum_required(VERSION 3.16)

project(QtQuick_Demo VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(QT_NO_PRIVATE_MODULE_WARNING ON)

find_package(Qt6 REQUIRED COMPONENTS Quick Network Concurrent ShaderTools GuiPrivate)

qt_standard_project_setup(REQUIRES 6.8)

# 获取src目录下的所有.cpp文件（包括子目录）
file(GLOB_RECURSE SRC_CPP_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "src/*.cpp")

# 排除平台特定文件
if(NOT WIN32)
    # 在非 Windows 平台上排除 Windows 特定文件
    list(FILTER SRC_CPP_FILES EXCLUDE REGEX ".*CrashDumpHandler\\.cpp$")
    list(FILTER SRC_CPP_FILES EXCLUDE REGEX ".*D3D11Node\\.cpp$")
    list(FILTER SRC_CPP_FILES EXCLUDE REGEX ".*D3D11Material\\.cpp$")
endif()

qt_add_executable(QtQuick_Demo
    ${SRC_CPP_FILES}
)

# 基础 include 目录
target_include_directories(QtQuick_Demo PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/utils
)

# 添加着色器文件到资源系统
if(WIN32)
    # Windows: 包含 D3D11 着色器
    qt6_add_shaders(QtQuick_Demo "shaders"
        BATCHABLE
        PRECOMPILE
        OPTIMIZED
        PREFIX
            "/shaders"
        FILES
            "shaders/yuv.vert"
            "shaders/yuv.frag"
            "shaders/d3d11_texture.vert"
            "shaders/d3d11_texture.frag"
    )
else()
    # macOS/Linux: 只包含 YUV 着色器
    qt6_add_shaders(QtQuick_Demo "shaders"
        BATCHABLE
        PRECOMPILE
        OPTIMIZED
        PREFIX
            "/shaders"
        FILES
            "shaders/yuv.vert"
            "shaders/yuv.frag"
    )
endif()

# 获取qml目录下的所有QML文件（相对路径）
file(GLOB_RECURSE QML_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "qml/*.qml")
qt_add_qml_module(QtQuick_Demo
    URI QtQuick_Demo
    VERSION 1.0
    OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/QtQuick_Demo
    QML_FILES
        ${QML_FILES}
)

set_target_properties(QtQuick_Demo PROPERTIES
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

# Qt 模块链接
target_link_libraries(QtQuick_Demo
    PRIVATE Qt6::Quick
            Qt6::Network
            Qt6::Concurrent
            Qt6::GuiPrivate
)

# ----------------- TcrSdk 动态库配置 -----------------
set(TCRSDK_BASE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/third_party/TcrSdk)

if(WIN32)
    # ============= Windows 配置 =============
    target_link_libraries(QtQuick_Demo PRIVATE dbghelp)

    # 检测架构
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(TCRSDK_ARCH x64)
    else()
        set(TCRSDK_ARCH Win32)
    endif()

    # 根据构建类型选择路径
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        set(TCRSDK_DIR "${TCRSDK_BASE_DIR}/win/Debug/${TCRSDK_ARCH}")
    else()
        set(TCRSDK_DIR "${TCRSDK_BASE_DIR}/win/Release/${TCRSDK_ARCH}")
    endif()

    # 添加头文件目录
    target_include_directories(QtQuick_Demo PRIVATE
        ${TCRSDK_BASE_DIR}/win/Release/include
    )

    # 链接导入库
    target_link_libraries(QtQuick_Demo PRIVATE
        ${TCRSDK_DIR}/TcrSdk.lib
    )

    # 复制 DLL 和 PDB 到构建目录
    add_custom_command(TARGET QtQuick_Demo POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${TCRSDK_DIR}/TcrSdk.dll"
            "$<TARGET_FILE_DIR:QtQuick_Demo>"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${TCRSDK_DIR}/TcrSdk.pdb"
            "$<TARGET_FILE_DIR:QtQuick_Demo>"
        COMMENT "Copying TcrSdk DLL and PDB to output directory"
        VERBATIM
    )

elseif(APPLE)
    # ============= macOS 配置 =============
    set(TCRSDK_DIR "${TCRSDK_BASE_DIR}/macos")

    # 添加头文件目录
    target_include_directories(QtQuick_Demo PRIVATE
        ${TCRSDK_DIR}/include
    )

    # 查找 dylib 文件（支持版本化文件名）
    file(GLOB TCRSDK_DYLIB_FILES "${TCRSDK_DIR}/lib/libTcrSdk*.dylib")

    if(NOT TCRSDK_DYLIB_FILES)
        message(FATAL_ERROR "TcrSdk dynamic library not found in ${TCRSDK_DIR}/lib/")
    endif()

    # 找到实际的库文件（非符号链接）
    foreach(dylib_file ${TCRSDK_DYLIB_FILES})
        if(NOT IS_SYMLINK ${dylib_file})
            get_filename_component(dylib_name ${dylib_file} NAME)
            # 匹配版本化文件名 libTcrSdk.x.y.z.dylib
            if(dylib_name MATCHES "libTcrSdk\\.[0-9]+\\.[0-9]+\\.[0-9]+\\.dylib")
                set(TCRSDK_DYLIB ${dylib_file})
                break()
            endif()
        endif()
    endforeach()

    # 如果没找到版本化文件，使用第一个非符号链接文件
    if(NOT TCRSDK_DYLIB)
        foreach(dylib_file ${TCRSDK_DYLIB_FILES})
            if(NOT IS_SYMLINK ${dylib_file})
                set(TCRSDK_DYLIB ${dylib_file})
                break()
            endif()
        endforeach()
    endif()

    if(NOT TCRSDK_DYLIB)
        message(FATAL_ERROR "Could not find actual TcrSdk dylib file")
    endif()

    message(STATUS "Using TcrSdk dylib: ${TCRSDK_DYLIB}")

    # 链接动态库
    target_link_libraries(QtQuick_Demo PRIVATE
        ${TCRSDK_DYLIB}
    )

    # 设置 rpath（让应用在运行时能找到 dylib）
    set_target_properties(QtQuick_Demo PROPERTIES
        INSTALL_RPATH "@executable_path/../Frameworks"
        BUILD_WITH_INSTALL_RPATH TRUE
    )

    # 复制所有 dylib 文件（包括符号链接）到应用包
    add_custom_command(TARGET QtQuick_Demo POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory
            "$<TARGET_BUNDLE_DIR:QtQuick_Demo>/Contents/Frameworks"
        COMMENT "Creating Frameworks directory in app bundle"
        VERBATIM
    )

    # 复制所有 libTcrSdk*.dylib 文件
    foreach(dylib_file ${TCRSDK_DYLIB_FILES})
        get_filename_component(dylib_name ${dylib_file} NAME)
        add_custom_command(TARGET QtQuick_Demo POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy
                "${dylib_file}"
                "$<TARGET_BUNDLE_DIR:QtQuick_Demo>/Contents/Frameworks/${dylib_name}"
            COMMENT "Copying ${dylib_name} to app bundle"
            VERBATIM
        )
    endforeach()

    # 使用 macdeployqt 自动部署 Qt 依赖
    find_program(MACDEPLOYQT_EXECUTABLE macdeployqt HINTS "${Qt6_DIR}/../../../bin")
    if(MACDEPLOYQT_EXECUTABLE)
        add_custom_command(TARGET QtQuick_Demo POST_BUILD
            COMMAND "${MACDEPLOYQT_EXECUTABLE}"
                "$<TARGET_BUNDLE_DIR:QtQuick_Demo>"
                -verbose=1
            COMMENT "Running macdeployqt to bundle Qt frameworks"
            VERBATIM
        )
    else()
        message(STATUS "macdeployqt not found. Qt frameworks must be deployed manually.")
    endif()

else()
    message(FATAL_ERROR "Unsupported platform. Only Windows and macOS are supported.")
endif()

# 安装配置
include(GNUInstallDirs)
install(TARGETS QtQuick_Demo
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
