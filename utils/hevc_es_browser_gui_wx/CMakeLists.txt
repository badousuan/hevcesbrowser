cmake_minimum_required(VERSION 3.10)

project(hevcesbrowser_gui_wx)

# Find wxWidgets
find_package(wxWidgets REQUIRED COMPONENTS core base adv net)
include(${wxWidgets_USE_FILE})

# Include directories
include_directories(
    .
    ../../hevcparser/include
    ../common
)

# Source files
set(SOURCES
    CentralWidget.cpp
    CommonInfoViewer.cpp
    HDRInfoViewer.cpp
    MainWindow.cpp
    ProfileConformanceAnalyzer.cpp
    StreamInfoViewer.cpp
    SyntaxViewer.cpp
    WarningsViewer.cpp
    main.cpp
    wxHexView.cpp
)

# Add executable
add_executable(hevcesbrowser_gui_wx MACOSX_BUNDLE WIN32 ${SOURCES})

# Link libraries
target_link_libraries(hevcesbrowser_gui_wx
    hevcparser
    utils_common
    ${wxWidgets_LIBRARIES}
)

# On macOS, ensure we build a bundle if needed, or just a normal executable
if(APPLE)
    # set_target_properties(hevcesbrowser_gui PROPERTIES MACOSX_BUNDLE TRUE)
endif()
