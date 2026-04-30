add_executable(hu_module_youtube
    main.cpp
    service/YouTubeService.h
    service/YouTubeService.cpp
    ui/YouTubeWindow.h
    ui/YouTubeWindow.cpp
)

target_include_directories(hu_module_youtube PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/service
    ${CMAKE_CURRENT_SOURCE_DIR}/ui
    ${CMAKE_CURRENT_SOURCE_DIR}/../common/widgets
)

target_link_libraries(hu_module_youtube PRIVATE
    hu_core
    hu_common
    Qt5::Core
    Qt5::Widgets
    Qt5::Network
)

if(Qt5WebEngineWidgets_FOUND)
    target_link_libraries(hu_module_youtube PRIVATE Qt5::WebEngineWidgets)
endif()

install(TARGETS hu_module_youtube RUNTIME DESTINATION bin)
