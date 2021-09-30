cmake_minimum_required(VERSION 3.20)
project(DLA)

set(CMAKE_CXX_STANDARD 17)

# Main executable
add_executable(DLA
        main.cpp
        Shader.cpp Shader.h
        stb_image.h
        imgui/imgui.cpp  imgui/imgui_widgets.cpp imgui/imgui_tables.cpp imgui/imgui_draw.cpp imgui/imgui_demo.cpp
        imgui/backends/imgui_impl_sdl.cpp imgui/backends/imgui_impl_opengl3.cpp)

find_package(SDL2 REQUIRED)
include_directories(DLA ${SDL2_INCLUDE_DIRS})

target_include_directories(DLA PUBLIC imgui)
target_include_directories(DLA PUBLIC imgui/backends)

target_link_libraries(DLA -lSDL2 -lGL -lGLEW ${CMAKE_DL_LIBS})
