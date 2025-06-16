cmake_minimum_required(VERSION 3.1.0)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# Find includes in corresponding build directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)
# Instruct CMake to run moc automatically when needed
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)

project(qucs-touchstone-viewer)

find_package(Qt5Core)
find_package(Qt5Gui)
find_package(Qt5Widgets)
find_package(Qt5PrintSupport) # Add PrintSupport for QPrinter
find_package(Qt5Svg REQUIRED)

set(APPLICATION_SRCS
    main.cpp
    qucstouchstoneviewer.cpp
    # ../qucs-s-spar-viewer/rectangularplotwidget.cpp # Reusing from s-spar-viewer
    # ../qucs-s-spar-viewer/polarplotwidget.cpp    # Reusing from s-spar-viewer
    # ../qucs-s-spar-viewer/smithchartwidget.cpp   # Reusing from s-spar-viewer
    # ../qucs-s-spar-viewer/matrixcombopopup.cpp # Reusing from s-spar-viewer
)

set(APPLICATION_HDRS
    qucstouchstoneviewer.h
    # ../qucs-s-spar-viewer/rectangularplotwidget.h # Reusing from s-spar-viewer
    # ../qucs-s-spar-viewer/polarplotwidget.h    # Reusing from s-spar-viewer
    # ../qucs-s-spar-viewer/smithchartwidget.h   # Reusing from s-spar-viewer
    # ../qucs-s-spar-viewer/matrixcombopopup.h # Reusing from s-spar-viewer
)

set(APPLICATION_RESOURCES
    qucstouchstoneviewer.qrc
)

if(WIN32)
    set(APPLICATION_ICON_RESOURCE_WINDOWS ${CMAKE_SOURCE_DIR}/qucs/qucs_icon.rc)
endif()

if(APPLE)
    set(MACOSX_BUNDLE_ICON_NAME qucs.icns)
    set(MACOSX_BUNDLE_GUI_IDENTIFIER io.github.qucs.qucs-s)
    set(MACOSX_BUNDLE_BUNDLE_NAME qucs-touchstone-viewer)
    set(MACOSX_BUNDLE_INFO_PLIST ${CMAKE_SOURCE_DIR}/Info.plist)
    set(APPLICATION_ICON_RESOURCE_MACOS ${CMAKE_SOURCE_DIR}/qucs/bitmaps/qucs.icns)
endif()

qt5_add_resources(qucs_RESOURCES ${APPLICATION_RESOURCES})

add_executable(qucs-touchstone-viewer MACOSX_BUNDLE
    ${APPLICATION_SRCS}
    ${APPLICATION_HDRS}
    ${qucs_RESOURCES}
    ${APPLICATION_ICON_RESOURCE_WINDOWS}
    ${APPLICATION_ICON_RESOURCE_MACOS}
)

target_link_libraries(qucs-touchstone-viewer Qt5::Core Qt5::Gui Qt5::Widgets Qt5::PrintSupport Qt5::Svg)

# Include directory for s-spar-viewer components
# include_directories(../qucs-s-spar-viewer)

install(TARGETS qucs-touchstone-viewer DESTINATION bin)
