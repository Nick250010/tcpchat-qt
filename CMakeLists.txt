cmake_minimum_required(VERSION 3.21)
project(TCPChatQt VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 6.4 REQUIRED COMPONENTS Core Network Widgets)

add_executable(tcpchat_server
    server/main.cpp
    server/chat_server.cpp
    server/chat_server.hpp
    shared/protocol.hpp
)
target_link_libraries(tcpchat_server PRIVATE Qt6::Core Qt6::Network)
target_include_directories(tcpchat_server PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(tcpchat_client WIN32
    client/main.cpp
    client/chat_window.cpp
    client/chat_window.hpp
    client/chat_connection.cpp
    client/chat_connection.hpp
    shared/protocol.hpp
)
target_link_libraries(tcpchat_client PRIVATE Qt6::Core Qt6::Network Qt6::Widgets)
target_include_directories(tcpchat_client PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

if(MSVC)
    target_compile_options(tcpchat_server PRIVATE /W4 /permissive-)
    target_compile_options(tcpchat_client PRIVATE /W4 /permissive-)
else()
    target_compile_options(tcpchat_server PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(tcpchat_client PRIVATE -Wall -Wextra -Wpedantic)
endif()

include(CTest)
if(BUILD_TESTING)
    find_package(Python3 COMPONENTS Interpreter)
    if(Python3_Interpreter_FOUND)
        add_test(NAME protocol_integration
            COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/integration_test.py $<TARGET_FILE:tcpchat_server>)
    endif()
endif()
