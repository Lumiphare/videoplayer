cmake_minimum_required(VERSION 3.27.7)
project(qtin)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(CMAKE_PREFIX_PATH "D:/Qt6/6.7.0/mingw_64/lib/cmake")

find_package(Qt6 COMPONENTS
        Core
        Gui
        Widgets
        Multimedia
        OpenGLWidgets
        OpenGL
        REQUIRED)

qt_add_resources(RESOURCE_ADDED resources.qrc)

include_directories(${CMAKE_SOURCE_DIR}/lib/ffmpeg/include ${CMAKE_SOURCE_DIR})

link_directories(
        "${CMAKE_SOURCE_DIR}/lib/SDL2"
        "${CMAKE_SOURCE_DIR}/lib/ffmpeg/lib"
)

set(SDL2_LIBS SDL2)
set(FFMPEG_LIBS
        avcodec
        avdevice
        avfilter
        avformat
        avutil
        swresample
        swscale
)
add_executable(qtin main.cpp
        AVCodecHandler.cpp
        AVCodecHandler.h
        playerwindow.cpp
        playerwindow.h
        playerwindow.ui
        TSQueue.cpp
        TSQueue.h
        YUVDataDefined.h
        AudioPlayer.cpp
        AudioPlayer.h
        OpenGLWidget.cpp
        OpenGLWidget.h
        ${RESOURCE_ADDED}
)
target_link_libraries(qtin
        Qt::Core
        Qt::Gui
        Qt::Widgets
        Qt::Multimedia
        Qt::OpenGLWidgets
        Qt::OpenGL
        ${FFMPEG_LIBS}
        -lopengl32
)
