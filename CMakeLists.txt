cmake_minimum_required(VERSION 3.20)
project(SudokuSolver VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Static linking for single executable deployment
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
set(BUILD_SHARED_LIBS OFF)

# Enable OpenMP
# unset(ENABLE_OPENMP CACHE)
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

# Find packages via vcpkg
find_package(OpenCV CONFIG REQUIRED)
find_package(Tesseract CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(CLI11 CONFIG REQUIRED)

# Collect source files
set(SOURCES
    src/main.cpp
    src/board.cpp
    src/solver_backtrack.cpp
    src/solver_dlx.cpp
    src/ocr_processor.cpp
    src/json_handler.cpp
    src/benchmark.cpp
    src/system_info.cpp
)

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

# Link libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
    ${OpenCV_LIBS}
    Tesseract::libtesseract
    nlohmann_json::nlohmann_json
    CLI11::CLI11
)

# OpenMP linking
if(OpenMP_CXX_FOUND)
    target_link_libraries(${PROJECT_NAME} PRIVATE OpenMP::OpenMP_CXX)
    target_compile_definitions(${PROJECT_NAME} PRIVATE USE_OPENMP)
endif()

# Compiler optimizations
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
else()
    target_compile_options(${PROJECT_NAME} PRIVATE
        -O3
        -march=native
        -flto
    )
endif()

# Windows specific: static linking of runtime
if(WIN32)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>"
    )
    # Prevent Windows.h from defining min/max macros
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        NOMINMAX
        WIN32_LEAN_AND_MEAN
    )
endif()

# Install
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
)

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

message(STATUS "")
message(STATUS "=== Sudoku Solver Configuration ===")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "OpenMP: ${OpenMP_CXX_FOUND}")
message(STATUS "OpenCV: ${OpenCV_VERSION}")
message(STATUS "===================================")
