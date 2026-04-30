cmake_minimum_required(VERSION 3.16)
project(PiRacerDashboard VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# Qt5/Qt6 auto-detection
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Widgets SerialPort)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Widgets SerialPort)

# VSOMEIP (optional) - receive gear from Head Unit via IPC
# -DIC_HAS_VSOMEIP=ON 으로 강제 활성화 가능 (Yocto 빌드 시 사용)
option(IC_HAS_VSOMEIP "Enable VSOMEIP for Instrument Cluster gear IPC" OFF)

if(IC_HAS_VSOMEIP)
    find_package(vsomeip3 REQUIRED)
    get_target_property(VSOMEIP_INCLUDE_DIR vsomeip3 INTERFACE_INCLUDE_DIRECTORIES)
    set(VSOMEIP_LIBRARY vsomeip3)
    message(STATUS "VSOMEIP (forced ON) - gear IPC enabled")
    add_compile_definitions(IC_HAS_VSOMEIP)
else()
    find_path(VSOMEIP_INCLUDE_DIR NAMES vsomeip/vsomeip.hpp)
    find_library(VSOMEIP_LIBRARY NAMES vsomeip3)
    if(VSOMEIP_INCLUDE_DIR AND VSOMEIP_LIBRARY)
        message(STATUS "VSOMEIP (auto-detected) - gear IPC enabled")
        add_compile_definitions(IC_HAS_VSOMEIP)
        set(IC_HAS_VSOMEIP ON)
    else()
        message(STATUS "VSOMEIP not found - gear IPC disabled")
    endif()
endif()

if(QT_VERSION_MAJOR EQUAL 6)
    qt_standard_project_setup()
endif()

if(APPLE AND TARGET Qt${QT_VERSION_MAJOR}::Gui)
    # Qt package metadata on some macOS SDK/toolchain combos still carries legacy AGL.
    # Strip it to avoid link failure on modern macOS where AGL framework is unavailable.
    get_target_property(_qt_gui_link_libs Qt${QT_VERSION_MAJOR}::Gui INTERFACE_LINK_LIBRARIES)
    if(_qt_gui_link_libs)
        list(REMOVE_ITEM _qt_gui_link_libs "-framework AGL")
        set_target_properties(Qt${QT_VERSION_MAJOR}::Gui PROPERTIES
            INTERFACE_LINK_LIBRARIES "${_qt_gui_link_libs}"
        )
    endif()
endif()

if(APPLE AND TARGET WrapOpenGL::WrapOpenGL)
    # Qt 6.8.3's FindWrapOpenGL can unconditionally append AGL fallback.
    # Modern macOS SDKs removed AGL, so filter out those entries.
    get_target_property(_wrap_opengl_libs WrapOpenGL::WrapOpenGL INTERFACE_LINK_LIBRARIES)
    if(_wrap_opengl_libs)
        list(FILTER _wrap_opengl_libs EXCLUDE REGEX "AGL")
        set_target_properties(WrapOpenGL::WrapOpenGL PROPERTIES
            INTERFACE_LINK_LIBRARIES "${_wrap_opengl_libs}"
        )
    endif()
endif()

# Source files
set(SOURCES
    src/main.cpp
    src/MainWindow.cpp
    src/widgets/SpeedometerWidget.cpp
    src/widgets/RpmGauge.cpp
    src/widgets/BatteryWidget.cpp
    src/serial/SerialReader.cpp
    src/utils/DataProcessor.cpp
    src/utils/CalibrationManager.cpp
)
if(VSOMEIP_INCLUDE_DIR AND VSOMEIP_LIBRARY)
    list(APPEND SOURCES src/ipc/VSomeIPGearReceiver.cpp)
endif()

set(HEADERS
    src/MainWindow.h
    src/widgets/SpeedometerWidget.h
    src/widgets/RpmGauge.h
    src/widgets/BatteryWidget.h
    src/serial/SerialReader.h
    src/utils/DataProcessor.h
    src/utils/CalibrationManager.h
)
if(VSOMEIP_INCLUDE_DIR AND VSOMEIP_LIBRARY)
    list(APPEND HEADERS src/ipc/VSomeIPGearReceiver.h)
endif()

# Qt resources
set(RESOURCES
    resources/dashboard.qrc
)

# Executable
if(QT_VERSION_MAJOR EQUAL 6)
    qt_add_executable(${PROJECT_NAME}
        MANUAL_FINALIZATION
        ${SOURCES}
        ${HEADERS}
        ${RESOURCES}
    )
else()
    add_executable(${PROJECT_NAME}
        ${SOURCES}
        ${HEADERS}
        ${RESOURCES}
    )
endif()

# Link Qt libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::SerialPort
)
if(VSOMEIP_INCLUDE_DIR AND VSOMEIP_LIBRARY)
    target_include_directories(${PROJECT_NAME} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src/ipc
        ${VSOMEIP_INCLUDE_DIR}
    )
    target_link_libraries(${PROJECT_NAME} PRIVATE ${VSOMEIP_LIBRARY})
endif()

if(APPLE)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        MACOSX_BUNDLE TRUE
    )
endif()

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/widgets
    ${CMAKE_CURRENT_SOURCE_DIR}/src/serial
    ${CMAKE_CURRENT_SOURCE_DIR}/src/utils
)
if(VSOMEIP_INCLUDE_DIR AND VSOMEIP_LIBRARY)
    target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/ipc)
endif()

# Install
if(APPLE)
    install(TARGETS ${PROJECT_NAME}
        BUNDLE DESTINATION .
        RUNTIME DESTINATION bin
    )
else()
    install(TARGETS ${PROJECT_NAME}
        RUNTIME DESTINATION bin
    )
endif()

# Copy Python bridge
install(DIRECTORY python/
    DESTINATION bin/python
    FILES_MATCHING PATTERN "*.py"
)

# Copy config
install(DIRECTORY config/
    DESTINATION bin/config
    FILES_MATCHING PATTERN "*.json"
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(${PROJECT_NAME})
endif()
