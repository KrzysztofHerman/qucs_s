cmake_minimum_required(VERSION 3.16)

project(QucsMeanQt6 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 COMPONENTS Core Gui Widgets REQUIRED)

add_executable(qucsmean
    main.cpp
    meanwindow.cpp
    meanwindow.h
)

target_link_libraries(qucsmean Qt6::Core Qt6::Gui Qt6::Widgets)

install(TARGETS qucsmean
    RUNTIME DESTINATION bin # Installs to <prefix>/bin
    # For bundles on macOS
    BUNDLE DESTINATION bin
)
