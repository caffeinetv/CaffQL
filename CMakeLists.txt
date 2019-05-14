cmake_minimum_required(VERSION 3.13.1)

project(CaffQL LANGUAGES CXX)

add_executable(CaffQL
    src/main.cpp
    src/Json.hpp
    src/BoxedOptional.hpp
)

target_compile_features(CaffQL PRIVATE cxx_std_17)
set_target_properties(CaffQL PROPERTIES CXX_EXTENSIONS OFF)

target_include_directories(CaffQL
    PRIVATE
    third_party/nlohmann_json/single_include
)
