cmake_minimum_required(VERSION 3.20)
project(clas12_dihadrons VERSION 0.1.0 LANGUAGES CXX)

option(CLAS12_BUILD_TESTS "Build tests" ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(nlohmann_json 3.10 REQUIRED)

add_library(clas12_analysis
    src/Modulation.cpp
    src/golden/GoldenAnalysis.cpp
)
target_include_directories(clas12_analysis PUBLIC include)
target_link_libraries(clas12_analysis PUBLIC nlohmann_json::nlohmann_json)

add_executable(clas12-golden-check apps/golden-check.cpp)
target_link_libraries(clas12-golden-check PRIVATE clas12_analysis)

if(CLAS12_BUILD_TESTS)
  include(CTest)
  add_test(NAME golden-analysis-pass COMMAND clas12-golden-check
    --reference ${CMAKE_CURRENT_SOURCE_DIR}/tests/golden/fixtures/reference.json
    --candidate ${CMAKE_CURRENT_SOURCE_DIR}/tests/golden/fixtures/candidate-pass.json)
  add_test(NAME golden-analysis-detects-regression COMMAND clas12-golden-check
    --reference ${CMAKE_CURRENT_SOURCE_DIR}/tests/golden/fixtures/reference.json
    --candidate ${CMAKE_CURRENT_SOURCE_DIR}/tests/golden/fixtures/candidate-fail.json)
  set_tests_properties(golden-analysis-detects-regression PROPERTIES WILL_FAIL TRUE)
endif()
