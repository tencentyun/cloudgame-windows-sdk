# =============================================================
# TcrSdk - macOS 平台配置
# =============================================================
# 动态链接 libTcrSdk.dylib，构建后自动复制到 .app bundle
# 并使用 macdeployqt 部署 Qt 依赖
# =============================================================

set(TCRSDK_DIR "${TCRSDK_BASE_DIR}/macos")

# 头文件
target_include_directories(${PROJECT_NAME} PRIVATE
    ${TCRSDK_BASE_DIR}/include
)

# --- 查找 dylib 文件 ---
file(GLOB TCRSDK_DYLIB_FILES "${TCRSDK_DIR}/lib/libTcrSdk*.dylib")

if(NOT TCRSDK_DYLIB_FILES)
    message(FATAL_ERROR "TcrSdk dynamic library not found in ${TCRSDK_DIR}/lib/")
endif()

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

# --- 链接与 rpath ---
target_link_libraries(${PROJECT_NAME} PRIVATE ${TCRSDK_DYLIB})

set_target_properties(${PROJECT_NAME} PROPERTIES
    INSTALL_RPATH "@executable_path/../Frameworks"
    BUILD_WITH_INSTALL_RPATH TRUE
)

# --- 复制 dylib 到 app bundle 的 Frameworks 目录 ---
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory
        "$<TARGET_BUNDLE_DIR:${PROJECT_NAME}>/Contents/Frameworks"
    COMMENT "Creating Frameworks directory in app bundle"
    VERBATIM
)

foreach(dylib_file ${TCRSDK_DYLIB_FILES})
    get_filename_component(dylib_name ${dylib_file} NAME)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
            "${dylib_file}"
            "$<TARGET_BUNDLE_DIR:${PROJECT_NAME}>/Contents/Frameworks/${dylib_name}"
        COMMENT "Copying ${dylib_name} to app bundle"
        VERBATIM
    )
endforeach()

# --- macdeployqt 自动部署 Qt 依赖 ---
find_program(MACDEPLOYQT_EXECUTABLE macdeployqt HINTS "${Qt6_DIR}/../../../bin")
if(MACDEPLOYQT_EXECUTABLE)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND "${MACDEPLOYQT_EXECUTABLE}"
            "$<TARGET_BUNDLE_DIR:${PROJECT_NAME}>"
            -qmldir=${CMAKE_CURRENT_SOURCE_DIR}/qml
            -verbose=1
        COMMENT "Running macdeployqt to bundle Qt frameworks and QML modules"
        VERBATIM
    )
else()
    message(STATUS "macdeployqt not found. Qt frameworks must be deployed manually.")
endif()
