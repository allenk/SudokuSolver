cmake_minimum_required(VERSION 3.20)
project(SudokuSolver VERSION 1.0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# =============================================================================
# Build Options
# =============================================================================

# =============================================================================
# Application Metadata
# =============================================================================
set(APP_PRODUCT_NAME "Sudoku Solver")
set(APP_DESCRIPTION "A high-performance, cross-platform Sudoku solver featuring multiple algorithms, OCR image recognition, and comprehensive benchmarking tools.")
set(APP_COMPANY "Kwyshell")
set(APP_COPYRIGHT "Copyright (C) 2025 Allen Kuo. MIT License.")

# =============================================================================
# Platform Detection
# =============================================================================
if(WIN32)
    set(PLATFORM_NAME "Windows")
elseif(APPLE)
    set(PLATFORM_NAME "macOS")
elseif(ANDROID)
    set(PLATFORM_NAME "Android")
elseif(UNIX)
    set(PLATFORM_NAME "Linux")
else()
    set(PLATFORM_NAME "Unknown")
endif()

# =============================================================================
# Windows Configuration
# =============================================================================
if(WIN32)
    # MSVC specific settings
    if(MSVC)
        # Use static runtime library (/MT instead of /MD)
        set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

        # Compiler flags
        add_compile_options(/W4 /utf-8)

        # Release optimizations
        set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /O2 /GL /Gy")
        set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /LTCG /OPT:REF /OPT:ICF")
    endif()
endif()

# =============================================================================
# macOS Configuration
# =============================================================================
if(APPLE)
    # macOS minimum deployment target
    set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum macOS version")

    # Support both Intel and Apple Silicon (Universal Binary)
    set(CMAKE_OSX_ARCHITECTURES "x86_64;arm64" CACHE STRING "Build architectures")

    # Compiler warnings
    add_compile_options(-Wall -Wextra -Wpedantic)

    # Release optimizations
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        add_compile_options(-O3 -DNDEBUG)
    endif()

    # Link against macOS frameworks if needed
    find_library(COREFOUNDATION_LIBRARY CoreFoundation)
    if(COREFOUNDATION_LIBRARY)
        message(STATUS "Found CoreFoundation: ${COREFOUNDATION_LIBRARY}")
    endif()
endif()

# =============================================================================
# Linux Configuration
# =============================================================================
if(UNIX AND NOT APPLE AND NOT ANDROID)
    # Compiler warnings
    add_compile_options(-Wall -Wextra -Wpedantic)

    # Release optimizations
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        add_compile_options(-O3 -DNDEBUG)
    endif()

    # Static linking options (for standalone distribution)
    # Try to link statically where possible
    set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libgcc -static-libstdc++")

    # Required for std::filesystem on older GCC
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 9.0)
        link_libraries(stdc++fs)
    endif()
endif()

# =============================================================================
# Android Configuration
# =============================================================================
if(ANDROID)
    # Android specific settings
    message(STATUS "Building for Android: ABI=${ANDROID_ABI}, Platform=${ANDROID_PLATFORM}")

    # Compiler warnings
    add_compile_options(-Wall -Wextra)

    # Release optimizations
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        add_compile_options(-O3 -DNDEBUG)
    endif()

    # Android uses Bionic libc, no need for special filesystem handling
endif()


# =============================================================================
# Enable OpenMP
# unset(ENABLE_OPENMP CACHE)
# =============================================================================
option(ENABLE_OPENMP "Enable OpenMP parallel processing" OFF)

if(ENABLE_OPENMP)
    # Find OpenMP
    find_package(OpenMP)
    if(OpenMP_CXX_FOUND)
        message(STATUS "OpenMP found - parallel processing enabled")
    else()
        message(WARNING "OpenMP not found - parallel processing disabled")
    endif()
else()
    message(STATUS "OpenMP disabled by user")
endif()

# =============================================================================
# Dependencies (via vcpkg)
# =============================================================================
find_package(OpenCV CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(CLI11 CONFIG REQUIRED)

# Tesseract is optional (not available on Android)
if(NOT ANDROID)
    find_package(Tesseract CONFIG REQUIRED)
    set(HAS_TESSERACT TRUE)
else()
    set(HAS_TESSERACT FALSE)
    message(STATUS "Tesseract disabled on Android")
endif()

# Collect source files
set(SOURCES
    src/main.cpp
    src/board.cpp
    src/solver_backtrack.cpp
    src/solver_dlx.cpp
    src/json_handler.cpp
    src/benchmark.cpp
    src/system_info.cpp
)

# Tesseract is optional (disabled on Android)
if(NOT ANDROID AND NOT IOS)
    find_package(Tesseract CONFIG)
    if(Tesseract_FOUND)
        set(HAS_TESSERACT TRUE)
        list(APPEND SOURCES src/ocr_processor.cpp)
        message(STATUS "Tesseract found - OCR enabled")
    else()
        set(HAS_TESSERACT FALSE)
        message(STATUS "Tesseract not found - OCR disabled")
    endif()
else()
    set(HAS_TESSERACT FALSE)
    message(STATUS "Tesseract disabled on ${PLATFORM_NAME}")
endif()

set(HEADERS
    include/board.hpp
    include/solver.hpp
    include/solver_backtrack.hpp
    include/solver_dlx.hpp
    include/ocr_processor.hpp
    include/json_handler.hpp
    include/benchmark.hpp
    include/types.hpp
    include/system_info.hpp
)

# Create executable
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# OpenMP linking
if(OpenMP_CXX_FOUND)
    target_link_libraries(${PROJECT_NAME} PRIVATE OpenMP::OpenMP_CXX)
    target_compile_definitions(${PROJECT_NAME} PRIVATE USE_OPENMP)
endif()

# Link libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
    ${OpenCV_LIBS}
    nlohmann_json::nlohmann_json
    CLI11::CLI11
)

if(HAS_TESSERACT)
    target_link_libraries(${PROJECT_NAME} PRIVATE Tesseract::libtesseract)
    target_compile_definitions(${PROJECT_NAME} PRIVATE HAS_TESSERACT)
endif()

# Win32 optimizations
if(WIN32)
    # Prevent Windows.h from defining min/max macros
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        NOMINMAX
        WIN32_LEAN_AND_MEAN
    )
    # MSVC specific settings
    if(MSVC)
        target_compile_options(${PROJECT_NAME} PRIVATE
            # Release optimizations
            $<$<CONFIG:Release>:/O2>           # Maximum optimization
            $<$<CONFIG:Release>:/Oi>           # Enable intrinsic functions
            $<$<CONFIG:Release>:/Ot>           # Favor fast code
            $<$<CONFIG:Release>:/GL>           # Whole program optimization
            $<$<CONFIG:Release>:/fp:fast>      # Fast floating-point
            $<$<CONFIG:Release>:/arch:AVX2>    # Use AVX2 instructions
            # RelWithDebInfo optimizations
            $<$<CONFIG:RelWithDebInfo>:/O2>
            $<$<CONFIG:RelWithDebInfo>:/Oi>
            $<$<CONFIG:RelWithDebInfo>:/Ot>
            $<$<CONFIG:RelWithDebInfo>:/arch:AVX2>
        )
        target_link_options(${PROJECT_NAME} PRIVATE
            $<$<CONFIG:Release>:/LTCG>         # Link-time code generation
        )
        # Definitions for Windows
        target_compile_definitions(SudokuSolver PRIVATE
            WIN32_LEAN_AND_MEAN     # Exclude rarely-used Windows headers
            _CRT_SECURE_NO_WARNINGS # Disable secure CRT warnings
            NOMINMAX                # Prevent definition of min/max macros
        )
    endif()
endif()

# macOS framework linking
if(APPLE AND COREFOUNDATION_LIBRARY)
    target_link_libraries(${PROJECT_NAME} PRIVATE ${COREFOUNDATION_LIBRARY})
endif()

# =============================================================================
# Compile Definitions
# =============================================================================
target_compile_definitions(${PROJECT_NAME} PRIVATE
    APP_VERSION="${PROJECT_VERSION}"
    APP_NAME="${PROJECT_NAME}"
)

# =============================================================================
# Strip binary for release (Linux/macOS/Android)
# =============================================================================
if(NOT WIN32 AND CMAKE_BUILD_TYPE STREQUAL "Release")
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_STRIP} $<TARGET_FILE:${PROJECT_NAME}>
        COMMENT "Stripping binary for release"
    )
endif()

# =============================================================================
# Install
# =============================================================================
install(TARGETS ${PROJECT_NAME} DESTINATION bin)

# Install tessdata directory (if exists)
if(EXISTS "${CMAKE_SOURCE_DIR}/tessdata")
    install(DIRECTORY ${CMAKE_SOURCE_DIR}/tessdata
        DESTINATION bin
    )
endif()

# Install examples directory
if(EXISTS "${CMAKE_SOURCE_DIR}/examples")
    install(DIRECTORY ${CMAKE_SOURCE_DIR}/examples
        DESTINATION bin
    )
endif()

# =============================================================================
# Configuration Summary
# =============================================================================
message(STATUS "")
message(STATUS "=== Sudoku Solver Configuration ===")
message(STATUS "Version: ${PROJECT_VERSION}")
message(STATUS "Platform: ${PLATFORM_NAME}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "OpenMP: ${OpenMP_CXX_FOUND}")
message(STATUS "OpenCV: ${OpenCV_VERSION}")
if(APPLE)
    message(STATUS "macOS Architectures: ${CMAKE_OSX_ARCHITECTURES}")
    message(STATUS "macOS Deployment Target: ${CMAKE_OSX_DEPLOYMENT_TARGET}")
endif()
if(ANDROID)
    message(STATUS "Android ABI: ${ANDROID_ABI}")
    message(STATUS "Android Platform: ${ANDROID_PLATFORM}")
endif()
message(STATUS "=============================================")
