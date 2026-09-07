cmake_minimum_required(VERSION 3.10)

# 基本信息
set(PROJECT_MAJOR 1)
set(PROJECT_MINOR 0)
set(PROJECT_PATCH 3)

# 自动计算 DAYS（UTC 1970 起累计天数）
string(TIMESTAMP BUILD_DAYS "%s" UTC)
math(EXPR PROJECT_DAYS "${BUILD_DAYS} / 86400")

set(PROJECT_MAIN_VERSION "${PROJECT_MAJOR}.${PROJECT_MINOR}.${PROJECT_PATCH}.${PROJECT_DAYS}")
set(PROJECT_MAKE_BATCH 250424)
set(PROJECT_COMPANY_NAME "xuwh, xudev")
set(PROJECT_NAME openvpn-mana)

project(${PROJECT_NAME} VERSION ${PROJECT_MAIN_VERSION} LANGUAGES C CXX)

# 设置C++标准并链接必要库
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 可配置编译参数
set(EASY_RSA_DIR "/home/xuwh/easy-rsa" CACHE PATH "Easy-RSA目录")
set(OVPN_DIR "/etc/openvpn" CACHE PATH "OpenVPN配置目录")

# 平台兼容定义
if(UNIX)
  add_definitions(-DUNIX)
elseif(WIN32)
  add_definitions(-DWIN)
  set(CMAKE_TOOLCHAIN_FILE "d:/dev/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "VCPKG toolchain file")
  set(GTest_DIR "vcpkg_installed/x64-windows-static/share/gtest" CACHE STRING "GTest dir")
  add_compile_options(/utf-8)
  add_definitions(-D_CRT_SECURE_NO_WARNINGS)
  add_compile_options(/wd4267)
endif()

# 定义输出路径
set(PROJECT_OUTPUT_DIR ${PROJECT_SOURCE_DIR}/build/${CMAKE_HOST_SYSTEM_NAME}_${CMAKE_SYSTEM_PROCESSOR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_OUTPUT_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_OUTPUT_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_OUTPUT_DIR})

# 生成配置头文件
configure_file(
    "${PROJECT_SOURCE_DIR}/cmake/config.hpp.in"
    "${PROJECT_SOURCE_DIR}/include/config.hpp"
)

# 生成版本头文件
configure_file(
    "${PROJECT_SOURCE_DIR}/cmake/version.hpp.in"
    "${PROJECT_SOURCE_DIR}/include/ovpn-mana/ovpn_mana_version.h"
)

# 头文件目录
include_directories(
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/src
)

# 查找必要的库
find_package(GTest REQUIRED)
find_package(Threads REQUIRED)

# 配置选项
option(BUILD_TESTS "Build tests" ON)

# 公开的动态库
add_library(ovpn-mana SHARED
    src/ovpn_mana_api.cpp
    src/core/openvpn_manager.cpp
    src/core/validators.cpp
    src/core/command_templates.cpp
)
set_target_properties(ovpn-mana PROPERTIES
    VERSION ${PROJECT_MAIN_VERSION}
    SOVERSION 1
    PUBLIC_HEADER include/ovpn-mana/ovpn_mana_api.h
)
target_compile_definitions(ovpn-mana PRIVATE OVPN_MANA_EXPORTS)
if(WIN32)
    target_sources(ovpn-mana PRIVATE cmake/libovpn-mana.def)
endif()

# 主程序
add_executable(openvpnmgr
    src/main.cpp
    src/cli/console_renderer.cpp
    src/cli/cli_handler.cpp
)
target_link_libraries(openvpnmgr PRIVATE ovpn-mana)

# 安装
install(CODE "
    execute_process(
        COMMAND chown root:root ${OVPN_DIR}/*.crt ${OVPN_DIR}/*.key ${OVPN_DIR}/*.pem
        COMMAND chmod 644 ${OVPN_DIR}/*.crt ${OVPN_DIR}/*.pem
        COMMAND chmod 600 ${OVPN_DIR}/*.key
    )
")

if(BUILD_TESTS)
    add_executable(test_validators
        test/test_validators.cpp
        src/core/validators.cpp
    )
    target_link_libraries(test_validators PRIVATE GTest::GTest GTest::Main)
    if(MSVC)
        target_compile_options(test_validators PRIVATE
            $<$<CONFIG:Debug>:/MTd>
            $<$<CONFIG:Release>:/MT>
        )
    endif()
    add_test(NAME validators COMMAND test_validators)

    add_executable(test_command_templates
        test/test_command_templates.cpp
        src/core/command_templates.cpp
    )
    target_link_libraries(test_command_templates PRIVATE GTest::GTest GTest::Main)
    if(MSVC)
        target_compile_options(test_command_templates PRIVATE
            $<$<CONFIG:Debug>:/MTd>
            $<$<CONFIG:Release>:/MT>
        )
    endif()
    add_test(NAME command_templates COMMAND test_command_templates)

    add_executable(test_validators_integration
        test/test_validators_integration.cpp
        src/core/validators.cpp
        src/core/openvpn_manager.cpp
        src/core/command_templates.cpp
    )
    target_link_libraries(test_validators_integration PRIVATE GTest::GTest GTest::Main ovpn-mana)
    if(MSVC)
        target_compile_options(test_validators_integration PRIVATE
            $<$<CONFIG:Debug>:/MTd>
            $<$<CONFIG:Release>:/MT>
        )
    endif()
    add_test(NAME validators_integration COMMAND test_validators_integration)

    add_executable(test_appconfig_defaults
        test/test_appconfig_defaults.cpp
        src/core/command_templates.cpp
    )
    target_link_libraries(test_appconfig_defaults PRIVATE GTest::GTest GTest::Main)
    if(MSVC)
        target_compile_options(test_appconfig_defaults PRIVATE
            $<$<CONFIG:Debug>:/MTd>
            $<$<CONFIG:Release>:/MT>
        )
    endif()
    add_test(NAME appconfig_defaults COMMAND test_appconfig_defaults)

    add_executable(test_config_loader
        test/test_config_loader.cpp
        src/core/command_templates.cpp
    )
    target_link_libraries(test_config_loader PRIVATE GTest::GTest GTest::Main)
    if(MSVC)
        target_compile_options(test_config_loader PRIVATE
            $<$<CONFIG:Debug>:/MTd>
            $<$<CONFIG:Release>:/MT>
        )
    endif()
    add_test(NAME config_loader COMMAND test_config_loader)
endif()

install(FILES config/ovpn-mana.json.example DESTINATION ${CMAKE_INSTALL_PREFIX}/etc/openvpn RENAME ovpn-mana.json)

add_executable(check_config
    tools/check_config.cpp
    src/core/command_templates.cpp
)
target_link_libraries(check_config PRIVATE ovpn-mana)