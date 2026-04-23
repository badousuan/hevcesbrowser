cmake_minimum_required(VERSION 3.10)

project(hevcesbrowser_gui_wx)

# Find wxWidgets
if(WIN32)
    set(WX_ROOT ${CMAKE_SOURCE_DIR}/wxWidgets)
    set(WX_INC_DIR ${WX_ROOT}/include)
    set(WX_LIB_DEBUG_DIR ${WX_ROOT}/wxMSW-3.3.2_vc14x_x64_DebugDLL/lib/vc14x_x64_dll)
    set(WX_LIB_RELEASE_DIR ${WX_ROOT}/wxMSW-3.3.2_vc14x_x64_ReleaseDLL/lib/vc14x_x64_dll)

    set(wxWidgets_INCLUDE_DIRS 
        ${WX_INC_DIR}
        $<$<CONFIG:Debug>:${WX_LIB_DEBUG_DIR}/mswud>
        $<$<CONFIG:Release>:${WX_LIB_RELEASE_DIR}/mswu>
    )

    # Link directories
    link_directories(
        $<$<CONFIG:Debug>:${WX_LIB_DEBUG_DIR}>
        $<$<CONFIG:Release>:${WX_LIB_RELEASE_DIR}>
    )

    set(wxWidgets_LIBRARIES
        $<$<CONFIG:Debug>:wxmsw33ud_core wxbase33ud wxmsw33ud_adv wxbase33ud_net wxmsw33ud_aui wxmsw33ud_html wxmsw33ud_xrc wxbase33ud_xml>
        $<$<CONFIG:Release>:wxmsw33u_core wxbase33u wxmsw33u_adv wxbase33u_net wxmsw33u_aui wxmsw33u_html wxmsw33u_xrc wxbase33u_xml>
        comctl32 rpcrt4 oleacc shlwapi version ws2_32 winmm
    )
    add_definitions(-D__WXMSW__ -DUNICODE -D_UNICODE -DWXUSINGDLL)
else()
    find_package(wxWidgets REQUIRED COMPONENTS core base adv net)
    include(${wxWidgets_USE_FILE})
endif()

# Include directories
include_directories(
    .
    ../../hevcparser/include
    ../common
    ${wxWidgets_INCLUDE_DIRS}
    ../hevc_es_browser_console/src
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
    ../hevc_es_browser_console/src/HEVCInfoWriter.cpp
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
    set_target_properties(hevcesbrowser_gui_wx PROPERTIES 
        MACOSX_BUNDLE TRUE
        MACOSX_DEPLOYMENT_TARGET "12.0"
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.virinext.hevcesbrowser"
        MACOSX_BUNDLE_BUNDLE_NAME "HEVCESBrowser"
        MACOSX_BUNDLE_BUNDLE_VERSION "1.0.0"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "1.0.0"
    )
endif()
