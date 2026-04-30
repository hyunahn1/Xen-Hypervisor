add_executable(hu_module_navigation
    main.cpp
    service/NavigationService.h
    service/NavigationService.cpp
    ui/NavigationWindow.h
    ui/NavigationWindow.cpp
)

target_include_directories(hu_module_navigation PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/service
    ${CMAKE_CURRENT_SOURCE_DIR}/ui
    ${CMAKE_CURRENT_SOURCE_DIR}/../common/widgets
)

target_link_libraries(hu_module_navigation PRIVATE
    hu_core
    hu_common
    Qt5::Core
    Qt5::Widgets
    Qt5::Network
)

if(Qt5WebEngineWidgets_FOUND)
    target_link_libraries(hu_module_navigation PRIVATE Qt5::WebEngineWidgets)
endif()

install(TARGETS hu_module_navigation RUNTIME DESTINATION bin)
