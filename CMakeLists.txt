cmake_minimum_required(VERSION 3.10)
project(rwandan_mines_backend CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Drogon REQUIRED)

add_executable(mines_server main.cc)
target_link_libraries(mines_server PRIVATE Drogon::Drogon)