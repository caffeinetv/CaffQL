cmake_minimum_required(VERSION 3.13.1)

project(CaffQL LANGUAGES CXX)

SET(CMAKE_CXX_STANDARD 17)
SET(CMAKE_CXX_EXTENSIONS OFF)

add_executable(CaffQL
    src/main.cpp
    src/Json.hpp
    src/BoxedOptional.hpp
    src/CodeGeneration.hpp
    src/CodeGeneration.cpp
)

target_include_directories(CaffQL
    PRIVATE
    third_party/nlohmann_json/single_include
    third_party/cxxopts/include
)

option(BUILD_TESTING "Enable tests" ON)

if(BUILD_TESTING)
	enable_testing()
	add_subdirectory(tests)
endif()
