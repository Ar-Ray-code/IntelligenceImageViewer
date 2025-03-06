cmake_minimum_required(VERSION 3.10)
project(AIPluginViewer)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)

find_package(Qt5 COMPONENTS Widgets Concurrent REQUIRED)
find_package(OpenCV REQUIRED)

include_directories(src)

set(SOURCES
    src/main.cpp
    src/ai_plugin_manager.cpp
)
add_executable(AIPluginViewer ${SOURCES})
target_include_directories(AIPluginViewer PRIVATE ${PROJECT_SOURCE_DIR})
target_link_libraries(AIPluginViewer Qt5::Widgets Qt5::Concurrent ${OpenCV_LIBS})

add_library(hsv_plugin SHARED
    plugins/hsv/hsv_plugin.cpp
)
target_include_directories(hsv_plugin PRIVATE ${PROJECT_SOURCE_DIR})
target_link_libraries(hsv_plugin Qt5::Core ${OpenCV_LIBS})

set_target_properties(hsv_plugin PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/plugins
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/plugins
    OUTPUT_NAME "hsv_plugin"
)
