add_executable(hu_module_media
    main.cpp
    service/MediaService.h
    service/MediaService.cpp
    ui/MediaWindow.h
    ui/MediaWindow.cpp
    media/MediaPlayer.h
    media/MediaPlayer.cpp
    media/PlaylistModel.h
    media/PlaylistModel.cpp
)

target_include_directories(hu_module_media PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/service
    ${CMAKE_CURRENT_SOURCE_DIR}/ui
    ${CMAKE_CURRENT_SOURCE_DIR}/media
    ${CMAKE_CURRENT_SOURCE_DIR}/../common/widgets
)

target_link_libraries(hu_module_media PRIVATE
    hu_core
    hu_common
    Qt5::Core
    Qt5::Widgets
    Qt5::Network
)

if(Qt5Multimedia_FOUND)
    target_link_libraries(hu_module_media PRIVATE Qt5::Multimedia)
endif()

install(TARGETS hu_module_media RUNTIME DESTINATION bin)
