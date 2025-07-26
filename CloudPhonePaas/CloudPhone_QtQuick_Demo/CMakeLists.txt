cmake_minimum_required(VERSION 3.16)

project(CloudPhone_QtQuick VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Quick Network Concurrent ShaderTools)

qt_standard_project_setup(REQUIRES 6.8)

qt_add_executable(CloudPhone_QtQuick
    src/main.cpp
    src/services/ApiService.cpp
    src/services/NetworkService.cpp
    src/viewmodels/LoginViewModel.cpp
    src/viewmodels/AndroidInstanceModel.cpp
    src/viewmodels/BatchTaskOperatorModel.cpp
    src/viewmodels/StreamingViewModel.cpp
    src/utils/Logger.cpp
    src/utils/InstanceImageProvider.cpp
    src/utils/InstanceImageDownloader.cpp
    src/utils/VariantListConverter.cpp
    src/core/BatchTaskOperator.cpp
    src/core/video/VideoRenderItem.cpp
    src/core/video/YuvTestPattern.cpp
    src/core/video/YuvNode.cpp
    src/core/video/YuvMaterial.cpp
)

target_include_directories(CloudPhone_QtQuick PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/utils
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/TcrSdk/Release/include
)

# 添加着色器文件到资源系统
qt6_add_shaders(CloudPhone_QtQuick "shaders"
    BATCHABLE
    PRECOMPILE
    OPTIMIZED
    PREFIX
        "/shaders"
    FILES
        "shaders/yuv.vert"
        "shaders/yuv.frag"
)

# 获取qml目录下的所有QML文件（相对路径）
file(GLOB_RECURSE QML_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "qml/*.qml")
qt_add_qml_module(CloudPhone_QtQuick
    URI CloudPhone_QtQuick
    VERSION 1.0
    OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/CloudPhone_QtQuick
    QML_FILES
        ${QML_FILES}
)

set_target_properties(CloudPhone_QtQuick PROPERTIES
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

# ----------------- TcrSdk 库和 DLL 处理 -----------------
# 统一 Qt 模块链接
target_link_libraries(CloudPhone_QtQuick
    PRIVATE Qt6::Quick
            Qt6::Network
            Qt6::Concurrent
)

# 处理 TcrSdk 库和 DLL
set(TCRSDK_BASE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/third_party/TcrSdk)
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(TCRSDK_PLATFORM x64)
else()
    set(TCRSDK_PLATFORM Win32)
endif()

# 用生成器表达式拼接路径
set(TCRSDK_DLL "$<IF:$<CONFIG:Debug>,${TCRSDK_BASE_DIR}/Debug/${TCRSDK_PLATFORM}/TcrSdk.dll,${TCRSDK_BASE_DIR}/Release/${TCRSDK_PLATFORM}/TcrSdk.dll>")
set(TCRSDK_PDB "$<IF:$<CONFIG:Debug>,${TCRSDK_BASE_DIR}/Debug/${TCRSDK_PLATFORM}/TcrSdk.pdb,${TCRSDK_BASE_DIR}/Release/${TCRSDK_PLATFORM}/TcrSdk.pdb>")
set(TCRSDK_LIB "$<IF:$<CONFIG:Debug>,${TCRSDK_BASE_DIR}/Debug/${TCRSDK_PLATFORM}/TcrSdk.lib,${TCRSDK_BASE_DIR}/Release/${TCRSDK_PLATFORM}/TcrSdk.lib>")

target_link_libraries(CloudPhone_QtQuick
    PRIVATE
        ${TCRSDK_LIB}
)

add_custom_command(TARGET CloudPhone_QtQuick POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E echo "Copying TcrSdk DLL: ${TCRSDK_DLL}"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${TCRSDK_DLL}
        $<TARGET_FILE_DIR:CloudPhone_QtQuick>
    COMMAND ${CMAKE_COMMAND} -E echo "Copying TcrSdk PDB: ${TCRSDK_PDB}"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${TCRSDK_PDB}
        $<TARGET_FILE_DIR:CloudPhone_QtQuick>
    COMMENT "Copying TcrSdk DLL and PDB for current config"
    VERBATIM
)

include(GNUInstallDirs)
install(TARGETS CloudPhone_QtQuick
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)