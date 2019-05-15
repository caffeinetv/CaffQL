cmake_minimum_required(VERSION 3.13.1)

project(CaffQL LANGUAGES CXX)

SET(CMAKE_CXX_STANDARD 17)
SET(CMAKE_CXX_EXTENSIONS OFF)

add_library(caffql
	STATIC
	src/Json.hpp
    src/BoxedOptional.hpp
    src/CodeGeneration.hpp
    src/CodeGeneration.cpp
)

add_executable(caffql-cli
    src/main.cpp
)

target_link_libraries(caffql-cli PRIVATE caffql)

target_include_directories(caffql
    PRIVATE
    third_party/nlohmann_json/single_include
)

target_include_directories(caffql-cli
    PRIVATE
    third_party/nlohmann_json/single_include
    third_party/cxxopts/include
)

set_target_properties(caffql-cli
	PROPERTIES
	OUTPUT_NAME caffql
)

option(BUILD_TESTING "Enable tests" ON)

if(BUILD_TESTING)
	enable_testing()
	add_subdirectory(tests)
endif()
