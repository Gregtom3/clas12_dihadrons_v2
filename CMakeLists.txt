cmake_minimum_required(VERSION 3.20)
project(clas12_dihadrons VERSION 0.3.0 LANGUAGES CXX)

option(CLAS12_BUILD_TESTS "Build tests" ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(nlohmann_json 3.10 REQUIRED)
find_package(yaml-cpp REQUIRED)

add_library(clas12_analysis
    src/Modulation.cpp
    src/analysis/CandidateBuilder.cpp
    src/config/AnalysisConfig.cpp
    src/golden/GoldenAnalysis.cpp
    src/provenance/RunManifest.cpp
)
target_include_directories(clas12_analysis PUBLIC include)
target_link_libraries(clas12_analysis PUBLIC nlohmann_json::nlohmann_json yaml-cpp)

add_executable(clas12-analysis apps/analysis.cpp)
target_link_libraries(clas12-analysis PRIVATE clas12_analysis)
add_executable(clas12-golden-check apps/golden-check.cpp)
target_link_libraries(clas12-golden-check PRIVATE clas12_analysis)

if(CLAS12_BUILD_TESTS)
  include(CTest)
  add_executable(candidate-builder-test tests/analysis/test_candidate_builder.cpp)
  target_link_libraries(candidate-builder-test PRIVATE clas12_analysis)
  add_test(NAME candidate-builder COMMAND candidate-builder-test)
  add_test(NAME configuration-valid COMMAND clas12-analysis validate
    ${CMAKE_CURRENT_SOURCE_DIR}/config/rga.yaml)
  add_test(NAME configuration-rejects-sweights COMMAND clas12-analysis validate
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/config/invalid-sweights.yaml)
  set_tests_properties(configuration-rejects-sweights PROPERTIES WILL_FAIL TRUE)
  add_test(NAME manifest-writes COMMAND clas12-analysis manifest
    ${CMAKE_CURRENT_SOURCE_DIR}/config/rga.yaml
    ${CMAKE_CURRENT_BINARY_DIR}/test-output/manifest.json)
  add_test(NAME golden-analysis-pass COMMAND clas12-golden-check
    --reference ${CMAKE_CURRENT_SOURCE_DIR}/tests/golden/fixtures/reference.json
    --candidate ${CMAKE_CURRENT_SOURCE_DIR}/tests/golden/fixtures/candidate-pass.json)
  add_test(NAME golden-analysis-detects-regression COMMAND clas12-golden-check
    --reference ${CMAKE_CURRENT_SOURCE_DIR}/tests/golden/fixtures/reference.json
    --candidate ${CMAKE_CURRENT_SOURCE_DIR}/tests/golden/fixtures/candidate-fail.json)
  set_tests_properties(golden-analysis-detects-regression PROPERTIES WILL_FAIL TRUE)
endif()
