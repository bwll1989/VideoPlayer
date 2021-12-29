cmake_minimum_required(VERSION 3.19)
project(untitled)

set(CMAKE_CXX_STANDARD 14)
set(SDL_DIR /opt/homebrew/Cellar/sdl2/2.0.18/) # 这个SDL开发包的路径，可以通过brew info sdl2查出来
set(FFMPEG_DIR /opt/homebrew/Cellar/ffmpeg/4.4.1_3/) # FFmpeg的安装目录，可以通过命令"brew info ffmpeg"获取
include_directories(${FFMPEG_DIR}/include/) # ffmpeg头文件搜索路径
include_directories(${SDL_DIR}/include/) # 添加SDL头文件搜索路径
link_directories(${FFMPEG_DIR}/lib/) # ffmpeg动态链接库或静态链接库的搜索路径
link_directories(${SDL_DIR}/lib/) # sdl2动态链接库或静态链接库的搜索路径
find_package(Threads REQUIRED)
add_executable(untitled main.cpp)
# 添加链接库
target_link_libraries(untitled
        SDL2
        SDL2_test
        SDL2main
        avcodec
        avdevice
        avfilter
        avformat
        )