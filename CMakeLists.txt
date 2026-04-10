cmake_minimum_required(VERSION 3.20)
project(HeatXTwin_Pro VERSION 2.0.0 LANGUAGES CXX)

# ============================================================================
# BUILD CONFIGURATION & VALIDATION
# ============================================================================
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Enable full diagnostics for debugging
option(HEATXTWIN_VERBOSE_CMAKE "Enable verbose CMake diagnostics" OFF)
option(HEATXTWIN_ENABLE_WINDEPLOYQT "Run windeployqt after build" OFF)
option(HEATXTWIN_STRICT_VALIDATION "Fail on any validation warning" OFF)

# Diagnostic output
if(HEATXTWIN_VERBOSE_CMAKE)
    message(STATUS "========== HeatXTwin Pro Build Diagnostics ==========")
    message(STATUS "CMake Version: ${CMAKE_VERSION}")
    message(STATUS "Build System: ${CMAKE_SYSTEM_NAME}")
    message(STATUS "Compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
    message(STATUS "Source Dir: ${CMAKE_SOURCE_DIR}")
    message(STATUS "Build Dir: ${CMAKE_BINARY_DIR}")
    message(STATUS "Install Dir: ${CMAKE_INSTALL_PREFIX}")
    set(CMAKE_FIND_DEBUG_MODE TRUE)
endif()

# ============================================================================
# DEPENDENCY VALIDATION & ROBUST FINDING
# ============================================================================

# Macro for robust package finding with fallback and diagnostics
macro(find_package_with_diagnostics PACKAGE_NAME REQUIRED_FLAG)
    set(options REQUIRED QUIET)
    set(single_value_args)
    set(multi_value_args COMPONENTS)
    cmake_parse_arguments(FPD "${options}" "${single_value_args}" "${multi_value_args}" ${ARGN})
    
    set(_fpd_required_arg)
    if("${REQUIRED_FLAG}" STREQUAL "REQUIRED")
        set(_fpd_required_arg REQUIRED)
    endif()
    
    set(_fpd_quiet_arg)
    if(FPD_QUIET)
        set(_fpd_quiet_arg QUIET)
    endif()
    
    message(STATUS "[Dependency] Searching for ${PACKAGE_NAME}...")
    
    if(FPD_COMPONENTS)
        find_package(${PACKAGE_NAME} ${_fpd_required_arg} COMPONENTS ${FPD_COMPONENTS} ${_fpd_quiet_arg})
    else()
        find_package(${PACKAGE_NAME} ${_fpd_required_arg} ${_fpd_quiet_arg})
    endif()
    
    if(${PACKAGE_NAME}_FOUND)
        message(STATUS "[Dependency] ✓ Found: ${PACKAGE_NAME}")
        if(HEATXTWIN_VERBOSE_CMAKE)
            if(DEFINED ${PACKAGE_NAME}_VERSION)
                message(STATUS "  Version: ${${PACKAGE_NAME}_VERSION}")
            endif()
            if(DEFINED ${PACKAGE_NAME}_DIR)
                message(STATUS "  Path: ${${PACKAGE_NAME}_DIR}")
            endif()
        endif()
    else()
        if(FPD_REQUIRED)
            message(FATAL_ERROR "[Dependency] ✗ REQUIRED package not found: ${PACKAGE_NAME}")
        else()
            message(WARNING "[Dependency] Optional package not found: ${PACKAGE_NAME}")
        endif()
    endif()
endmacro()

# Find all required dependencies
find_package_with_diagnostics(Eigen3 REQUIRED)
find_package_with_diagnostics(fmt REQUIRED)
find_package_with_diagnostics(spdlog REQUIRED)
find_package_with_diagnostics(tomlplusplus REQUIRED)
find_package_with_diagnostics(Qt6 REQUIRED COMPONENTS Core Widgets Charts)

# Validate critical dependencies
if(NOT Eigen3_FOUND)
    message(FATAL_ERROR "Eigen3 is required for matrix operations. Install via vcpkg: vcpkg install eigen3:x64-windows")
endif()

if(NOT Qt6_FOUND)
    message(FATAL_ERROR "Qt6 is required for GUI. Install via vcpkg: vcpkg install qt6:x64-windows")
endif()

message(STATUS "========== All Dependencies Validated ==========")
message(STATUS "")

# ============================================================================
# SOURCE FILES VALIDATION
# ============================================================================

# Verify all source files exist before compilation
set(SOURCES
    src/main.cpp
    src/app/ui/ChartWidget.cpp
    src/app/ui/Diagnostics.cpp
    src/app/ui/HeatExchangerWidget.cpp
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

# Validate source files before compilation
message(STATUS "[Sources] Validating ${CMAKE_SOURCE_DIR}...")
foreach(SOURCE ${SOURCES})
    if(NOT EXISTS "${CMAKE_SOURCE_DIR}/${SOURCE}")
        message(FATAL_ERROR "[Sources] ✗ Source file not found: ${SOURCE}")
    else()
        if(HEATXTWIN_VERBOSE_CMAKE)
            message(STATUS "[Sources] ✓ Found: ${SOURCE}")
        endif()
    endif()
endforeach()
message(STATUS "[Sources] All source files validated")

# ============================================================================
# EXECUTABLE & COMPILATION SETTINGS
# ============================================================================

# Create executable
add_executable(HeatXTwin_Pro ${SOURCES})

# Include directories with validation
message(STATUS "[Includes] Adding include directories...")
target_include_directories(HeatXTwin_Pro PRIVATE 
    ${CMAKE_SOURCE_DIR}/src
)

if(NOT EXISTS "${CMAKE_SOURCE_DIR}/src")
    message(FATAL_ERROR "[Includes] Source directory not found: ${CMAKE_SOURCE_DIR}/src")
endif()
message(STATUS "[Includes] ✓ Include paths configured")

# ============================================================================
# LINK LIBRARIES (WITH VALIDATION)
# ============================================================================

message(STATUS "[Libraries] Configuring link libraries...")
target_link_libraries(HeatXTwin_Pro PRIVATE
    Eigen3::Eigen
    fmt::fmt
    spdlog::spdlog
    tomlplusplus::tomlplusplus
    Qt6::Core
    Qt6::Widgets
    Qt6::Charts
)
message(STATUS "[Libraries] ✓ All libraries linked")

# ============================================================================
# COMPILER & WARNING CONFIGURATION
# ============================================================================

message(STATUS "[Compiler] Configuring compiler settings...")

# MSVC compiler settings
if(MSVC)
    # Add warning flags
    target_compile_options(HeatXTwin_Pro PRIVATE
        /W4              # Warning level 4
        /WX              # Treat warnings as errors (optional)
        /permissive-     # Strict C++ conformance
        /Zc:inline       # Remove unreferenced COMDAT
        /Gm-             # Disable minimal rebuild
        /GR              # Enable RTTI
        /EHsc            # Enable exceptions
    )
    
    # Release optimization flags
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        target_compile_options(HeatXTwin_Pro PRIVATE
            /O2              # Maximum optimization
            /Oi              # Inline function expansion
            /GL              # Whole program optimization
        )
        target_link_options(HeatXTwin_Pro PRIVATE /LTCG)  # Link-time code generation
        message(STATUS "[Compiler] Release build optimizations enabled")
    endif()
    
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_options(HeatXTwin_Pro PRIVATE
        -Wall
        -Wextra
        -Wpedantic
        -Wconversion
        -Wsign-conversion
        -Wnon-virtual-dtor
    )
    
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        target_compile_options(HeatXTwin_Pro PRIVATE -O3 -march=native)
        message(STATUS "[Compiler] Release build optimizations enabled")
    endif()
endif()

message(STATUS "[Compiler] ✓ Compiler settings configured")

# ============================================================================
# QT META OBJECT COMPILER & RESOURCES
# ============================================================================

message(STATUS "[Qt] Configuring Qt meta object compiler...")
set_property(TARGET HeatXTwin_Pro PROPERTY AUTOMOC ON)
set_property(TARGET HeatXTwin_Pro PROPERTY AUTORCC ON)
set_property(TARGET HeatXTwin_Pro PROPERTY AUTOUIC ON)
message(STATUS "[Qt] ✓ Qt MOC/RCC/UIC enabled")

# ============================================================================
# DEPLOYMENT & POST-BUILD CONFIGURATION
# ============================================================================

if(WIN32)
    message(STATUS "[Deployment] Configuring Windows deployment...")
    
    # Qt plugin deployment (optional)
    if(HEATXTWIN_ENABLE_WINDEPLOYQT)
        message(STATUS "[Deployment] Enabling windeployqt post-build step...")
        
        # Find Qt tools
        get_target_property(QT6_QMAKE_EXECUTABLE Qt6::qmake IMPORTED_LOCATION)
        get_filename_component(QT6_BIN_DIR "${QT6_QMAKE_EXECUTABLE}" DIRECTORY)
        find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS "${QT6_BIN_DIR}")
        
        if(WINDEPLOYQT_EXECUTABLE)
            message(STATUS "[Deployment] ✓ Found windeployqt: ${WINDEPLOYQT_EXECUTABLE}")
            add_custom_command(TARGET HeatXTwin_Pro POST_BUILD
                COMMAND "${WINDEPLOYQT_EXECUTABLE}"
                    --verbose 0
                    --no-compiler-runtime
                    --dir $<TARGET_FILE_DIR:HeatXTwin_Pro>
                    $<TARGET_FILE:HeatXTwin_Pro>
                COMMENT "[Deployment] Running windeployqt to deploy Qt dependencies..."
                VERBATIM
            )
            message(STATUS "[Deployment] ✓ windeployqt post-build step configured")
        else()
            message(WARNING "[Deployment] windeployqt not found. Manual Qt DLL deployment may be required.")
            message(STATUS "  Qt bin directory: ${QT6_BIN_DIR}")
            message(STATUS "  To fix: Ensure Qt6 is correctly installed via vcpkg")
        endif()
    else()
        message(STATUS "[Deployment] windeployqt disabled (set HEATXTWIN_ENABLE_WINDEPLOYQT=ON to enable)")
    endif()
    
    message(STATUS "[Deployment] ✓ Windows deployment configured")
endif()

# ============================================================================
# BUILD SUMMARY
# ============================================================================

message(STATUS "")
message(STATUS "========== HeatXTwin Pro Build Summary ==========")
message(STATUS "Project Version: ${PROJECT_VERSION}")
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Compiler: ${CMAKE_CXX_COMPILER_ID} (C++${CMAKE_CXX_STANDARD})")
message(STATUS "Source Directory: ${CMAKE_SOURCE_DIR}")
message(STATUS "Build Directory: ${CMAKE_BINARY_DIR}")
message(STATUS "Executable Target: HeatXTwin_Pro")
message(STATUS "")
message(STATUS "Options:")
message(STATUS "  HEATXTWIN_VERBOSE_CMAKE: ${HEATXTWIN_VERBOSE_CMAKE}")
message(STATUS "  HEATXTWIN_ENABLE_WINDEPLOYQT: ${HEATXTWIN_ENABLE_WINDEPLOYQT}")
message(STATUS "  HEATXTWIN_STRICT_VALIDATION: ${HEATXTWIN_STRICT_VALIDATION}")
message(STATUS "")
message(STATUS "Dependencies: ")
message(STATUS "  ✓ Eigen3 (matrix operations)")
message(STATUS "  ✓ fmt (string formatting)")
message(STATUS "  ✓ spdlog (logging)")
message(STATUS "  ✓ toml++ (configuration)")
message(STATUS "  ✓ Qt6 (GUI)")
message(STATUS "")
message(STATUS "Next steps:")
message(STATUS "  1. Run: cmake --build build --config Release")
message(STATUS "  2. Deploy: ./run_app.bat")
message(STATUS "  3. For diagnostics: cmake -DHEATXTWIN_VERBOSE_CMAKE=ON -B build")
message(STATUS "")
message(STATUS "========== Configuration Complete ==========")
message(STATUS "")
