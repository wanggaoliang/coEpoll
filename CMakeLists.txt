cmake_minimum_required(VERSION 3.10)

project(sock5 VERSION 1.1)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# 添加可执行目标
add_subdirectory(outf)
add_executable(mian main.cpp)
add_executable(client client.cpp)
target_link_libraries(mian pthread sock)
target_link_libraries(client pthread sock)