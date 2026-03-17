# =============================================================
# TcrSdk - Linux 平台配置
# =============================================================
# 静态链接 libTcrSdk.a 及其所有传递依赖（WebRTC、vcpkg 库等）
# 使用 --start-group/--end-group 解决静态库间的循环依赖
# =============================================================

set(TCRSDK_DIR "${TCRSDK_BASE_DIR}/linux")

# TcrSdk 的上游项目目录（用于查找 WebRTC 和 vcpkg 依赖）
set(TCRSDK_PROJECT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../../TcrSdkProject")

# 头文件
target_include_directories(${PROJECT_NAME} PRIVATE
    ${TCRSDK_BASE_DIR}/include
)

# 编译定义
target_compile_definitions(${PROJECT_NAME} PRIVATE
    WEBRTC_LINUX
    WEBRTC_POSIX
    TCRSDK_STATIC
)

# rpath 设置
# $ORIGIN       : 查找同目录下的库（构建时使用）
# $ORIGIN/../lib: 查找 ../lib/ 下的库（install 部署后使用）
set_target_properties(${PROJECT_NAME} PROPERTIES
    INSTALL_RPATH "$ORIGIN;$ORIGIN/../lib"
    BUILD_WITH_INSTALL_RPATH TRUE
)

# --- 静态库路径 ---
set(WEBRTC_LIB "${TCRSDK_PROJECT_DIR}/TcrSdk/third_party/webrtc/lib/linux/libwebrtc.a")
set(VCPKG_LIB_DIR "${TCRSDK_PROJECT_DIR}/vcpkg_installed/x64/x64-linux/lib")

# --- 链接静态库及系统依赖 ---
# 注意：libTcrSdk.a 的依赖不会自动传递，必须显式链接
target_link_libraries(${PROJECT_NAME} PRIVATE
    -Wl,--start-group
    ${TCRSDK_DIR}/lib/libTcrSdk.a
    ${WEBRTC_LIB}
    ${VCPKG_LIB_DIR}/libjsoncpp.a
    ${VCPKG_LIB_DIR}/libspdlog.a
    ${VCPKG_LIB_DIR}/libfmt.a
    ${VCPKG_LIB_DIR}/libcurl.a
    ${VCPKG_LIB_DIR}/libmbedtls.a
    ${VCPKG_LIB_DIR}/libmbedx509.a
    ${VCPKG_LIB_DIR}/libmbedcrypto.a
    ${VCPKG_LIB_DIR}/libeverest.a
    ${VCPKG_LIB_DIR}/libp256m.a
    ${VCPKG_LIB_DIR}/libz.a
    ${VCPKG_LIB_DIR}/libboost_system.a
    -Wl,--end-group
    pthread
    dl
    rt
    m
)

target_link_options(${PROJECT_NAME} PRIVATE
    -Wl,--allow-multiple-definition
)
