add_executable(hu_module_call
    main.cpp
    service/CallService.h
    service/CallService.cpp
    ui/CallWindow.h
    ui/CallWindow.cpp
)

target_include_directories(hu_module_call PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/service
    ${CMAKE_CURRENT_SOURCE_DIR}/ui
    ${CMAKE_CURRENT_SOURCE_DIR}/../common/widgets
)

target_link_libraries(hu_module_call PRIVATE
    hu_core
    hu_common
    Qt5::Core
    Qt5::Widgets
    Qt5::Network
)

install(TARGETS hu_module_call RUNTIME DESTINATION bin)
