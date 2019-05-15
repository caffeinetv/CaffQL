add_executable(tests
    src/test-main.cpp
    src/BoxedOptionalTests.cpp
)

target_include_directories(tests
    PRIVATE
    third_party/doctest
    ${CMAKE_SOURCE_DIR}/third_party/nlohmann_json/single_include
    ${CMAKE_SOURCE_DIR}/src
)

add_test(NAME CaffQLTests COMMAND tests)
