cmake_minimum_required(VERSION 3.13)

project(redis-starter-cpp)

set(CMAKE_CXX_STANDARD 23) # Enable the C++23 standard
set(THREADS_PREFER_PTHREAD_FLAG ON)

find_package(Threads REQUIRED)
find_package(asio CONFIG REQUIRED)


add_executable(server
    src/Server.cpp)

target_link_libraries(server PRIVATE asio asio::asio)
target_link_libraries(server PRIVATE Threads::Threads)

add_executable(client
src/Client.cpp)

target_link_libraries(client PRIVATE asio asio::asio)
target_link_libraries(client PRIVATE Threads::Threads)