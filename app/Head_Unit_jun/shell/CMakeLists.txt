set(SHELL_SOURCES
    main.cpp
    ShellWindow.h
    ShellWindow.cpp
    PdcController.h
    PdcController.cpp
    PdcBeepController.h
    PdcBeepController.cpp
    ModuleController.h
    ModuleController.cpp
    ModuleBridge.h
    ModuleBridge.cpp
    widgets/TabBar.h
    widgets/TabBar.cpp
    widgets/StatusBar.h
    widgets/StatusBar.cpp
    widgets/GlowOverlay.h
    widgets/GlowOverlay.cpp
    widgets/GearPanel.h
    widgets/GearPanel.cpp
    widgets/SplashScreen.h
    widgets/SplashScreen.cpp
    widgets/ReverseCameraWindow.h
    widgets/ReverseCameraWindow.cpp
    widgets/PdcOverlayPainter.h
    widgets/PdcOverlayPainter.cpp
    ${CMAKE_SOURCE_DIR}/resources/headunit.qrc
)

if(Qt5WaylandCompositor_FOUND AND Qt5OpenGL_FOUND)
    list(APPEND SHELL_SOURCES
        HUCompositor.h
        HUCompositor.cpp
        ModuleSurfaceWidget.h
        ModuleSurfaceWidget.cpp
        ClusterOutputWindow.h
        ClusterOutputWindow.cpp
    )
endif()

add_executable(hu_shell ${SHELL_SOURCES})

target_include_directories(hu_shell PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/widgets
)

target_link_libraries(hu_shell PRIVATE
    hu_core
    Qt5::Core
    Qt5::Widgets
    Qt5::Network
)

find_package(PkgConfig)
if(PkgConfig_FOUND)
    pkg_check_modules(GST gstreamer-1.0 gstreamer-app-1.0 gstreamer-video-1.0)
    if(GST_FOUND)
        target_include_directories(hu_shell PRIVATE ${GST_INCLUDE_DIRS})
        target_compile_options(hu_shell PRIVATE ${GST_CFLAGS_OTHER})
        target_link_directories(hu_shell PRIVATE ${GST_LIBRARY_DIRS})
        target_link_libraries(hu_shell PRIVATE ${GST_LIBRARIES})
        target_compile_definitions(hu_shell PRIVATE HU_CAMERA_PREVIEW_AVAILABLE)
        message(STATUS "GStreamer found - libcamerasrc pipeline enabled")
    else()
        message(WARNING "GStreamer not found - rear camera disabled")
    endif()
endif()

if(Qt5WaylandCompositor_FOUND AND Qt5OpenGL_FOUND)
    target_link_libraries(hu_shell PRIVATE
        Qt5::WaylandCompositor
        Qt5::OpenGL
    )
endif()

install(TARGETS hu_shell RUNTIME DESTINATION bin)
