cmake_minimum_required(VERSION 3.18.0)

# vcpkg link
set(CMAKE_TOOLCHAIN_FILE "vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "Vcpkg toolchain file")

# set output directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/main)

project(gst_main)


include_directories(src/gstreamer_wrapper)

add_executable(${PROJECT_NAME} src/main.cpp)

add_executable(test src/gstreamer_wrapper_test/test.cpp)



find_package(PkgConfig REQUIRED)
pkg_check_modules(GSTREAMER REQUIRED IMPORTED_TARGET gstreamer-1.0)
target_link_libraries(${PROJECT_NAME} PRIVATE PkgConfig::GSTREAMER)
target_link_libraries(test PRIVATE PkgConfig::GSTREAMER)

find_package(OpenCV REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE ${OpenCV_LIBS})


# vcpkg packages
message(STATUS "Using spdlog installed by vcpkg")
find_package(fmt CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE fmt::fmt)

message(STATUS "Using spdlog installed by vcpkg")
find_package(spdlog CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE spdlog::spdlog)