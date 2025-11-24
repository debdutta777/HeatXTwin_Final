cmake_minimum_required(VERSION 3.20)
project(HeatXTwin_Pro VERSION 2.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find dependencies
find_package(Eigen3 REQUIRED)
find_package(fmt REQUIRED)
find_package(spdlog REQUIRED)
find_package(tomlplusplus REQUIRED)
find_package(Qt6 REQUIRED COMPONENTS Core Widgets Charts)

# Sources
set(SOURCES
    src/main.cpp
    src/app/ui/ChartWidget.cpp
    src/app/ui/MainWindow.cpp
    src/app/ui/SimWorker.cpp
    src/core/ControllerPID.cpp
    src/core/EstimatorRLS.cpp
    src/core/Fouling.cpp
    src/core/Hydraulics.cpp
    src/core/Model.cpp
    src/core/Simulator.cpp
    src/core/Thermo.cpp
    src/core/Validation.cpp
    src/io/Config.cpp
    src/io/CsvLogger.cpp
)

# Executable
add_executable(HeatXTwin_Pro ${SOURCES})

# Include directories
target_include_directories(HeatXTwin_Pro PRIVATE src)

# Link libraries
target_link_libraries(HeatXTwin_Pro PRIVATE
    Eigen3::Eigen
    fmt::fmt
    spdlog::spdlog
    tomlplusplus::tomlplusplus
    Qt6::Core
    Qt6::Widgets
    Qt6::Charts
)

# Qt MOC
set_property(TARGET HeatXTwin_Pro PROPERTY AUTOMOC ON)
set_property(TARGET HeatXTwin_Pro PROPERTY AUTORCC ON)
set_property(TARGET HeatXTwin_Pro PROPERTY AUTOUIC ON)

# Deployment (Windows)
if(WIN32)
    find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS "${CMAKE_SOURCE_DIR}/build/vcpkg_installed/x64-windows/tools/Qt6/bin")
    if(WINDEPLOYQT_EXECUTABLE)
        add_custom_command(TARGET HeatXTwin_Pro POST_BUILD
            COMMAND "${WINDEPLOYQT_EXECUTABLE}"
                --verbose 0
                --no-compiler-runtime
                --dir $<TARGET_FILE_DIR:HeatXTwin_Pro>
                $<TARGET_FILE:HeatXTwin_Pro>
            COMMENT "Running windeployqt to deploy Qt dependencies..."
        )
    else()
        message(WARNING "windeployqt not found! You may need to manually copy Qt DLLs.")
    endif()
endif()
