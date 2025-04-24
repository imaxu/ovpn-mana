cmake_minimum_required(VERSION 3.10)

# 基本信息
set(PROJECT_MAIN_VERSION 1.0.0.1)
set(PROJECT_MAKE_BATCH 250424)
set(PROJECT_COMPANY_NAME "xuwh, xudev")
set(PROJECT_NAME openvpn-mana)


project(${PROJECT_NAME} VERSION ${PROJECT_MAIN_VERSION} LANGUAGES C CXX)

# 设置C++标准并链接必要库
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 可配置编译参数
set(EASY_RSA_DIR "/root/easy-rsa" CACHE PATH "Easy-RSA目录")
set(OVPN_DIR "/etc/openvpn" CACHE PATH "OpenVPN配置目录")

# 定义输出路径
set(PROJECT_OUTPUT_DIR ${PROJECT_SOURCE_DIR}/build/${CMAKE_HOST_SYSTEM_NAME}_${CMAKE_SYSTEM_PROCESSOR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_OUTPUT_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_OUTPUT_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_OUTPUT_DIR})

# 生成配置头文件
configure_file(
    "${PROJECT_SOURCE_DIR}/include/config.hpp.in"
    "${PROJECT_SOURCE_DIR}/include/config.hpp"
)

# 头文件目录
include_directories(
    ${PROJECT_SOURCE_DIR}/include
)


# 查找必要的库
find_package(GTest REQUIRED)
find_package(Threads REQUIRED)

# 配置选项
option(BUILD_TESTS "Build tests" ON)

# 主库
add_library(openvpn_mgr
    src/OpenVPNManager.cpp
)




# 公开的动态库
add_library(ovpn-mana SHARED
    src/ovpn-mana.cpp
    src/OpenVPNManager.cpp
)
target_link_libraries(ovpn-mana PRIVATE openvpn_mgr)
set_target_properties(ovpn-mana PROPERTIES
    VERSION ${PROJECT_MAIN_VERSION}
    SOVERSION 1
    PUBLIC_HEADER include/ovpn-mana.h
)
install(TARGETS ovpn-mana
    LIBRARY DESTINATION lib
    PUBLIC_HEADER DESTINATION include
)
# 安装头文件
install(FILES include/ovpn-mana.h
    DESTINATION include
)

# 拷贝ovpn-mana.ini到输出目录
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/ovpn-mana.ini
    DESTINATION ${CMAKE_CURRENT_BINARY_DIR}
)


# 主程序
add_executable(openvpnmgr
    src/main.cpp
)
target_link_libraries(openvpnmgr PRIVATE ovpn-mana)

# 单元测试
if(BUILD_TESTS)
    add_executable(test_client
        test/test_client.cpp
    )
    target_link_libraries(test_client PRIVATE 
        openvpn_mgr
        GTest::GTest 
        GTest::Main
        Threads::Threads
    )
    add_test(NAME client_test COMMAND test_client)
endif()


# 添加安装后脚本设置权限
install(CODE "
    execute_process(
        COMMAND chown root:root ${OVPN_DIR}/*.crt ${OVPN_DIR}/*.key ${OVPN_DIR}/*.pem
        COMMAND chmod 644 ${OVPN_DIR}/*.crt ${OVPN_DIR}/*.pem
        COMMAND chmod 600 ${OVPN_DIR}/*.key
    )
")