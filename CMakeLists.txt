cmake_minimum_required(VERSION 3.16)

project(explain
    VERSION 1.1.0
    DESCRIPTION "Linux 命令智能解释器（豆包大模型）"
    LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

find_package(CURL REQUIRED)

add_executable(explain
    src/main.cpp
    src/config.cpp
    src/http.cpp
    src/manpage.cpp
    src/history.cpp
    src/output.cpp
    src/util.cpp
)

target_include_directories(explain PRIVATE include)
target_link_libraries(explain PRIVATE CURL::libcurl)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(explain PRIVATE -Wall -Wextra)
endif()

install(TARGETS explain RUNTIME DESTINATION bin)
install(FILES explainrc.example DESTINATION share/explain)
