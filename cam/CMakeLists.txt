# Define the minimum CMAKE version required:
cmake_minimum_required(VERSION 3.11)

# Define the project:
project(MocapCam C CXX)

# Set C++ stardard:
set(CMAKE_CXX_STANDARD 17)

# Enable automatic include of generated files (like paths.h):
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# Make code optimized:
set(CMAKE_CXX_FLAGS "-Wall -Wextra")
set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O1")

find_package(OpenMP)

find_package(PkgConfig)

pkg_check_modules(LIBCAMERA REQUIRED IMPORTED_TARGET libcamera)
message(STATUS "libcamera library found:")
message(STATUS "    version: ${LIBCAMERA_VERSION}")
message(STATUS "    libraries: ${LIBCAMERA_LINK_LIBRARIES}")
message(STATUS "    include path: ${LIBCAMERA_INCLUDE_DIRS}")

include_directories(${CMAKE_SOURCE_DIR} ${LIBCAMERA_INCLUDE_DIRS})

# Define all header files we want to compile:
set(HEADERS
    src/MarkerFinder.h
    src/PiCamera.h
    src/Semaphore.h
    src/Vec4f.h
)

# Define all source files we want to compile:
set(SOURCES 
    src/main.cpp

    src/PiCamera.cpp
)

# Define shader & resources which should be listed in IDE:
set(RESOURCES

)
# Allow to include files directly in this paths
# (without the need to specify folders):
include_directories(src)

# Define executables with source files and resources:
add_executable(MocapCam ${SOURCES} ${HEADERS} ${RESOURCES})

# IMGUI specific compile definition:
target_compile_definitions(MocapCam PUBLIC -DCMAKE_SOURCE_DIR="${CMAKE_SOURCE_DIR}")

# OpenMP
target_link_libraries(MocapCam PUBLIC OpenMP::OpenMP_CXX)

target_link_libraries(MocapCam PRIVATE ${LIBCAMERA_LINK_LIBRARIES})
