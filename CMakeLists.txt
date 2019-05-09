cmake_minimum_required(VERSION 3.13.1)

project(CaffQL LANGUAGES CXX)

add_executable(CaffQL
    src/main.cpp
)

target_compile_features(CaffQL PRIVATE cxx_std_17)

target_include_directories(CaffQL
    PRIVATE
    third_party/nlohmann_json/single_include
)
