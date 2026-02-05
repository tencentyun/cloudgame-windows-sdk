cmake_minimum_required(VERSION 3.16)

project(QtQuick_Demo VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(QT_NO_PRIVATE_MODULE_WARNING ON)

find_package(Qt6 REQUIRED COMPONENTS Quick Network Concurrent ShaderTools GuiPrivate)

qt_standard_project_setup(REQUIRES 6.8)

# 获取src目录下的所有.cpp文件（包括子目录）
file(GLOB_RECURSE SRC_CPP_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "src/*.cpp")

qt_add_executable(QtQuick_Demo
    ${SRC_CPP_FILES}
)

# 基础 include 目录（不包含 TcrSdk，TcrSdk 在平台特定配置中添加）
target_include_directories(QtQuick_Demo PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/utils
)

# 添加着色器文件到资源系统
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

# ----------------- TcrSdk 库和 DLL 处理 -----------------
set(TCRSDK_BASE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/third_party/TcrSdk)

# Qt 模块链接（通用部分）
target_link_libraries(QtQuick_Demo
    PRIVATE Qt6::Quick
            Qt6::Network
            Qt6::Concurrent
            Qt6::GuiPrivate
)

if(WIN32)
    # Windows 平台特定配置
    target_link_libraries(QtQuick_Demo PRIVATE dbghelp)

    # Windows TcrSdk include 目录
    target_include_directories(QtQuick_Demo PRIVATE
        ${TCRSDK_BASE_DIR}/win/Release/include
    )

    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(TCRSDK_PLATFORM x64)
    else()
        set(TCRSDK_PLATFORM Win32)
    endif()

    # 用生成器表达式拼接路径
    set(TCRSDK_DLL "$<IF:$<CONFIG:Debug>,${TCRSDK_BASE_DIR}/win/Debug/${TCRSDK_PLATFORM}/TcrSdk.dll,${TCRSDK_BASE_DIR}/win/Release/${TCRSDK_PLATFORM}/TcrSdk.dll>")
    set(TCRSDK_PDB "$<IF:$<CONFIG:Debug>,${TCRSDK_BASE_DIR}/win/Debug/${TCRSDK_PLATFORM}/TcrSdk.pdb,${TCRSDK_BASE_DIR}/win/Release/${TCRSDK_PLATFORM}/TcrSdk.pdb>")
    set(TCRSDK_LIB "$<IF:$<CONFIG:Debug>,${TCRSDK_BASE_DIR}/win/Debug/${TCRSDK_PLATFORM}/TcrSdk.lib,${TCRSDK_BASE_DIR}/win/Release/${TCRSDK_PLATFORM}/TcrSdk.lib>")

    target_link_libraries(QtQuick_Demo PRIVATE ${TCRSDK_LIB})

    add_custom_command(TARGET QtQuick_Demo POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo "Copying TcrSdk DLL: ${TCRSDK_DLL}"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${TCRSDK_DLL}
            $<TARGET_FILE_DIR:QtQuick_Demo>
        COMMAND ${CMAKE_COMMAND} -E echo "Copying TcrSdk PDB: ${TCRSDK_PDB}"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${TCRSDK_PDB}
            $<TARGET_FILE_DIR:QtQuick_Demo>
        COMMENT "Copying TcrSdk DLL and PDB for current config"
        VERBATIM
    )

elseif(APPLE)
    # macOS 平台特定配置
    set(TCRSDK_FRAMEWORK_DIR "${TCRSDK_BASE_DIR}/macos")
    
    # 查找 TcrSdk framework 或 dylib
    # 假设 macos 目录下有 TcrSdk.framework 或 libTcrSdk.dylib
    if(EXISTS "${TCRSDK_FRAMEWORK_DIR}/TcrSdk.framework")
        # 使用 framework - framework 自带头文件
        target_include_directories(QtQuick_Demo PRIVATE
            ${TCRSDK_FRAMEWORK_DIR}/TcrSdk.framework/Headers
        )
        target_link_libraries(QtQuick_Demo PRIVATE
            "-framework TcrSdk"
        )
        target_link_directories(QtQuick_Demo PRIVATE
            ${TCRSDK_FRAMEWORK_DIR}
        )
        # 复制 framework 到应用包
        add_custom_command(TARGET QtQuick_Demo POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${TCRSDK_FRAMEWORK_DIR}/TcrSdk.framework"
                "$<TARGET_BUNDLE_DIR:QtQuick_Demo>/Contents/Frameworks/TcrSdk.framework"
            COMMENT "Copying TcrSdk.framework to app bundle"
            VERBATIM
        )
    elseif(EXISTS "${TCRSDK_FRAMEWORK_DIR}/libTcrSdk.dylib")
        # 使用 dylib - 需要单独的 include 目录
        if(EXISTS "${TCRSDK_FRAMEWORK_DIR}/include")
            target_include_directories(QtQuick_Demo PRIVATE
                ${TCRSDK_FRAMEWORK_DIR}/include
            )
        else()
            message(WARNING "TcrSdk include directory not found at ${TCRSDK_FRAMEWORK_DIR}/include")
        endif()
        target_link_libraries(QtQuick_Demo PRIVATE
            "${TCRSDK_FRAMEWORK_DIR}/libTcrSdk.dylib"
        )
        # 复制 dylib 到应用包
        add_custom_command(TARGET QtQuick_Demo POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory
                "$<TARGET_BUNDLE_DIR:QtQuick_Demo>/Contents/Frameworks"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${TCRSDK_FRAMEWORK_DIR}/libTcrSdk.dylib"
                "$<TARGET_BUNDLE_DIR:QtQuick_Demo>/Contents/Frameworks/"
            COMMENT "Copying libTcrSdk.dylib to app bundle"
            VERBATIM
        )
        # 设置 rpath
        set_target_properties(QtQuick_Demo PROPERTIES
            INSTALL_RPATH "@executable_path/../Frameworks"
            BUILD_WITH_INSTALL_RPATH TRUE
        )
    else()
        message(FATAL_ERROR "TcrSdk not found in ${TCRSDK_FRAMEWORK_DIR}. Expected TcrSdk.framework or libTcrSdk.dylib")
    endif()
endif()

include(GNUInstallDirs)
install(TARGETS QtQuick_Demo
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)