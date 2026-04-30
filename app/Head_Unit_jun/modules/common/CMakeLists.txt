add_library(hu_common STATIC
    widgets/GearPanel.h
    widgets/GearPanel.cpp
)

target_include_directories(hu_common PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/widgets
)

target_link_libraries(hu_common PUBLIC
    hu_core
    Qt5::Core
    Qt5::Widgets
)
