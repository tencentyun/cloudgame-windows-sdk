# =============================================================
# TcrSdk - Linux 平台配置
# =============================================================
# 动态链接 libTcrSdk.so
#
# 预编译包目录结构（均位于 third_party/TcrSdk/linux/ 下）：
#   lib/libTcrSdk.so   - TcrSdk 动态库（包含所有内部依赖）
#   include/           - 公共头文件
# =============================================================

set(TCRSDK_DIR "${TCRSDK_BASE_DIR}/linux")
set(TCRSDK_LIB_DIR "${TCRSDK_DIR}/lib")

# 头文件
target_include_directories(${PROJECT_NAME} PRIVATE
    ${TCRSDK_BASE_DIR}/include
)

# 编译定义
target_compile_definitions(${PROJECT_NAME} PRIVATE
    WEBRTC_LINUX
    WEBRTC_POSIX
)

# --- 查找 .so 文件 ---
find_library(TCRSDK_SO TcrSdk PATHS "${TCRSDK_LIB_DIR}" NO_DEFAULT_PATH)

if(NOT TCRSDK_SO)
    message(FATAL_ERROR "TcrSdk shared library not found in ${TCRSDK_LIB_DIR}/\n"
        "Expected: ${TCRSDK_LIB_DIR}/libTcrSdk.so")
endif()

message(STATUS "Using TcrSdk shared library: ${TCRSDK_SO}")

# --- 链接 ---
target_link_libraries(${PROJECT_NAME} PRIVATE ${TCRSDK_SO})

# --- rpath ---
# $ORIGIN       : 运行时在可执行文件同目录查找 libTcrSdk.so（构建目录运行）
# $ORIGIN/../lib: install 部署后从 ../lib/ 查找
set_target_properties(${PROJECT_NAME} PROPERTIES
    INSTALL_RPATH "$ORIGIN;$ORIGIN/../lib"
    BUILD_WITH_INSTALL_RPATH TRUE
)

# --- 构建后复制 libTcrSdk.so 到可执行文件同目录 ---
# 使 $ORIGIN rpath 直接生效，无需设置 LD_LIBRARY_PATH
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${TCRSDK_SO}"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>/libTcrSdk.so"
    COMMENT "Copying libTcrSdk.so to executable directory"
    VERBATIM
)
