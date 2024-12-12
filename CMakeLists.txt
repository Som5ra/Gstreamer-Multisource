cmake_minimum_required(VERSION 3.18.0)

# vcpkg link
set(CMAKE_TOOLCHAIN_FILE "vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "Vcpkg toolchain file")

# set output directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/main)

project(gst_main)


include_directories(src/gstreamer_wrapper)

add_executable(${PROJECT_NAME} src/main.cpp)

add_executable(test src/gstreamer_wrapper_test/test.cpp)

set(OpenCV_DIR /media/sombrali/HDD1/opencv-unity/gusto_dnn/3rdparty/opencv/opencv-static-full) # for debug build
find_package(OpenCV REQUIRED)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GSTREAMER REQUIRED IMPORTED_TARGET gstreamer-1.0 gstreamer-app-1.0)


include_directories(${GST_INCLUDE_DIRS} ${OpenCV_INCLUDE_DIRS})
# link_directories(${GST_LIBRARY_DIRS} ${OpenCV_LIBRARY_DIRS})
message(STATUS "GST_INCLUDE_DIRS: ${GST_INCLUDE_DIRS}")

target_link_libraries(${PROJECT_NAME} PRIVATE PkgConfig::GSTREAMER)


target_link_libraries(test PRIVATE  PkgConfig::GSTREAMER fmt::fmt spdlog::spdlog ${OpenCV_LIBS})

find_package(OpenCV REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE ${OpenCV_LIBS})


# vcpkg packages
message(STATUS "Using spdlog installed by vcpkg")
find_package(fmt CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE fmt::fmt)

message(STATUS "Using spdlog installed by vcpkg")
find_package(spdlog CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE spdlog::spdlog)