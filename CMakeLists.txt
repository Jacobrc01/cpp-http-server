cmake_minimum_required(VERSION 3.10)
project(cpp_http_server)

set(CMAKE_CXX_STANDARD 17)
set(SRC_DIR "${CMAKE_SOURCE_DIR}/src")

file(GLOB SOURCES "${SRC_DIR}/*.cpp")

add_executable(server ${SOURCES})
target_include_directories(server PRIVATE "${CMAKE_SOURCE_DIR}/include")

# Din CMakeLists.txt er det “opskriftshæfte”, som CMake bruger til at 
# genere et build-system (f.eks. Makefiles eller Visual Studio-projektfiler). 
