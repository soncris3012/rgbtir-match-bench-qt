cmake_minimum_required(VERSION 3.22)

project(RgbTirMatchBench VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 6.4 REQUIRED COMPONENTS Widgets Test)
# OpenCV 5 renamed features2d/calib3d to features/calib. Loading the
# exported package without a component list keeps this project compatible
# with both OpenCV 4 and 5.
find_package(OpenCV REQUIRED)

add_library(rgbtir_core
    src/core/BenchmarkEngine.cpp
    src/core/DatasetPairer.cpp
    src/core/FeatureMatcher.cpp
    src/core/MetricCalculator.cpp
    src/core/ReportWriter.cpp
)
target_include_directories(rgbtir_core PUBLIC src ${OpenCV_INCLUDE_DIRS})
target_link_libraries(rgbtir_core PUBLIC Qt6::Core ${OpenCV_LIBS})
target_compile_definitions(rgbtir_core PUBLIC QT_NO_CAST_FROM_ASCII QT_NO_CAST_TO_ASCII)

add_executable(rgbtir-match-bench
    src/main.cpp
    src/ui/DropZone.cpp
    src/ui/MainWindow.cpp
    src/worker/BenchmarkWorker.cpp
)
target_link_libraries(rgbtir-match-bench PRIVATE rgbtir_core Qt6::Widgets)

if(APPLE)
    set_target_properties(rgbtir-match-bench PROPERTIES MACOSX_BUNDLE TRUE)
endif()

include(CTest)
if(BUILD_TESTING)
    add_executable(rgbtir-tests
        tests/TestCore.cpp
    )
    target_link_libraries(rgbtir-tests PRIVATE rgbtir_core Qt6::Test)
    add_test(NAME rgbtir-tests COMMAND rgbtir-tests)
endif()

install(TARGETS rgbtir-match-bench
    BUNDLE DESTINATION .
    RUNTIME DESTINATION bin
)

