cmake_minimum_required(VERSION 3.27)
project(CppPlayground)

add_executable(Cpp17Playground cpp17main.cpp
        cpp17/counter.h
        cpp17/mutable_compile_time_list.h)

add_executable(Cpp20Playground cpp20main.cpp
        cpp20/counter.h)

set_property(TARGET Cpp17Playground PROPERTY CXX_STANDARD 17)

set_property(TARGET Cpp20Playground PROPERTY CXX_STANDARD 20)
