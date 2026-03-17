# =============================================================
# TcrSdk - Windows 平台配置
# =============================================================
# 动态链接 TcrSdk.dll，构建后自动复制 DLL/PDB 到输出目录
# =============================================================

# 链接 Windows 系统库（用于 CrashDumpHandler）
target_link_libraries(${PROJECT_NAME} PRIVATE dbghelp)

# 检测目标架构
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(TCRSDK_ARCH x64)
else()
    set(TCRSDK_ARCH Win32)
endif()

# 根据构建类型选择 Debug/Release 路径
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(TCRSDK_DIR "${TCRSDK_BASE_DIR}/win/Debug/${TCRSDK_ARCH}")
else()
    set(TCRSDK_DIR "${TCRSDK_BASE_DIR}/win/Release/${TCRSDK_ARCH}")
endif()

# 头文件
target_include_directories(${PROJECT_NAME} PRIVATE
    ${TCRSDK_BASE_DIR}/include
)

# 链接导入库
target_link_libraries(${PROJECT_NAME} PRIVATE
    ${TCRSDK_DIR}/TcrSdk.lib
)

# 复制 DLL 和 PDB 到构建输出目录
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${TCRSDK_DIR}/TcrSdk.dll"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${TCRSDK_DIR}/TcrSdk.pdb"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
    COMMENT "Copying TcrSdk DLL and PDB to output directory"
    VERBATIM
)
