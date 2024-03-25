cmake_minimum_required(VERSION 3.21)
project(CppCoroutines-Tasks)

set(CMAKE_CXX_STANDARD 20)

file(GLOB basic_files "*.h" "*.hpp" "*.inl" "*.cpp" "*.c")
add_executable("06.sleep" ${basic_files})