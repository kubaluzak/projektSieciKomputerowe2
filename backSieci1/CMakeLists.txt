cmake_minimum_required(VERSION 3.30)
project(projektSK2)

set(CMAKE_CXX_STANDARD 20)

add_executable(projektSK2 main.cpp)


find_package(OpenSSL REQUIRED)
target_link_libraries(projektSK2 OpenSSL::SSL OpenSSL::Crypto)

