cmake_minimum_required(VERSION 3.24)
project(HeatXTwin_Final VERSION 2.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Export compile commands for better IntelliSense
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Warnings
if(MSVC)
  add_compile_options(/W4 /permissive-)
else()
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# vcpkg manifest mode support
if(DEFINED ENV{VCPKG_ROOT})
  message(STATUS "Using vcpkg from $ENV{VCPKG_ROOT}")
else()
  message(WARNING "VCPKG_ROOT not set. Please set it to use vcpkg.")
endif()

# Find packages (vcpkg provides them)
find_package(Eigen3 CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(tomlplusplus CONFIG REQUIRED)
find_package(Qt6 REQUIRED COMPONENTS Core Widgets Charts)

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/src)

# Core simulation library
set(CORE_SOURCES
    src/core/Thermo.cpp
    src/core/Hydraulics.cpp
    src/core/Fouling.cpp
    src/core/Simulator.cpp
    src/core/Model.cpp
    src/core/ControllerPID.cpp
    src/core/EstimatorRLS.cpp
    src/core/Validation.cpp
)

# UI sources
set(UI_SOURCES
    src/app/ui/MainWindow.cpp
    src/app/ui/ChartWidget.cpp
    src/app/ui/SimWorker.cpp
)

# IO utilities
set(IO_SOURCES
    src/io/Config.cpp
    src/io/CsvLogger.cpp
)

# Main executable
add_executable(HeatXTwin_Pro
    src/main.cpp
    ${CORE_SOURCES}
    ${UI_SOURCES}
    ${IO_SOURCES}
)

target_link_libraries(HeatXTwin_Pro PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::Charts
    Eigen3::Eigen
    fmt::fmt
    spdlog::spdlog
    tomlplusplus::tomlplusplus
)

# Set output directories
set_target_properties(HeatXTwin_Pro PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# Copy config file to build directory
file(COPY configs/baseline.toml DESTINATION ${CMAKE_BINARY_DIR}/configs)

# Installation
install(TARGETS HeatXTwin_Pro RUNTIME DESTINATION bin)
install(FILES configs/baseline.toml DESTINATION configs)
