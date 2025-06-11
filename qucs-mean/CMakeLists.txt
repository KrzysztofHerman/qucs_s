cmake_minimum_required(VERSION 3.10)
project(qucs-mean)

find_package(Qt5Widgets REQUIRED)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(SOURCES
    main.cpp
    qucsmeanwindow.cpp
)

set(HEADERS
    qucsmeanwindow.h
)

add_executable(qucs-mean ${SOURCES} ${HEADERS})

target_link_libraries(qucs-mean Qt5::Widgets)

install(TARGETS qucs-mean DESTINATION bin)
