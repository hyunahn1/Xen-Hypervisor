cmake_minimum_required(VERSION 3.16)
project(piracer_head_unit VERSION 2.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# ── Qt5 고정 (RPi 환경) ────────────────────────────────────────────────
find_package(Qt5 REQUIRED COMPONENTS Core Widgets Network)

# ── Wayland Compositor (RPi 필수, 데스크탑 선택) ──────────────────────
# 데스크탑에서 설치: sudo apt-get install libqt5waylandcompositor5-dev libqt5waylandclient5-dev
find_package(Qt5 OPTIONAL_COMPONENTS WaylandCompositor OpenGL)
if(Qt5WaylandCompositor_FOUND AND Qt5OpenGL_FOUND)
    message(STATUS "Qt5WaylandCompositor found - Wayland compositor enabled")
    add_compile_definitions(HU_WAYLAND_COMPOSITOR)
else()
    message(WARNING "Qt5WaylandCompositor NOT found - install libqt5waylandcompositor5-dev for local build")
    message(WARNING "Yocto/RPi build will have it via PACKAGECONFIG:append:pn-qtwayland")
endif()

# ── VSOMEIP (HU ↔ IC IPC) ─────────────────────────────────────────────
option(HU_HAS_VSOMEIP "Enable VSOMEIP for Head Unit ↔ Cluster IPC" OFF)
if(HU_HAS_VSOMEIP)
    find_package(vsomeip3 REQUIRED)
    message(STATUS "VSOMEIP found - real IPC enabled (gear/speed/battery)")
    add_compile_definitions(HU_HAS_VSOMEIP)
else()
    message(STATUS "VSOMEIP not found - mock IPC mode (no real vehicle data)")
endif()

# ── 옵셔널 컴포넌트 ────────────────────────────────────────────────────
find_package(Qt5 OPTIONAL_COMPONENTS Multimedia MultimediaWidgets)
if(Qt5Multimedia_FOUND)
    message(STATUS "Qt Multimedia found - real audio enabled")
    add_compile_definitions(HU_MULTIMEDIA_AVAILABLE)
else()
    message(STATUS "Qt Multimedia NOT found - stub audio")
endif()

if(Qt5MultimediaWidgets_FOUND)
    message(STATUS "Qt MultimediaWidgets found - camera viewfinder UI available")
else()
    message(STATUS "Qt MultimediaWidgets NOT found - reverse camera keeps placeholder")
endif()

find_package(Qt5 OPTIONAL_COMPONENTS WebEngineWidgets)
if(Qt5WebEngineWidgets_FOUND)
    message(STATUS "Qt WebEngineWidgets found - YouTube/Navigation WebView enabled")
    add_compile_definitions(HU_WEBENGINE_AVAILABLE)
else()
    message(STATUS "Qt WebEngineWidgets NOT found - stub screens only")
endif()

# ── 모든 실행파일을 한 디렉토리에 모아서 ModuleController가 찾을 수 있게 ──
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# ── 빌드 순서: core → common → 모듈 → shell ──────────────────────────
add_subdirectory(core)
add_subdirectory(modules/common)
add_subdirectory(modules/media)
add_subdirectory(modules/youtube)
add_subdirectory(modules/call)
add_subdirectory(modules/navigation)
add_subdirectory(modules/ambient)
add_subdirectory(modules/settings)
add_subdirectory(shell)

# ── 설치: 모두 bin/ 에 모아서 ModuleController가 찾을 수 있게 ──────────
install(DIRECTORY config/ DESTINATION bin/config FILES_MATCHING PATTERN "*.json")
if(EXISTS "${CMAKE_SOURCE_DIR}/python")
    install(DIRECTORY python/ DESTINATION bin/python FILES_MATCHING PATTERN "*.py")
endif()
# hu_motor_control.py: scripts/ 사용 (게임패드 → 스로틀/스티어링, 기어는 /tmp/piracer_drive_mode.json)
if(EXISTS "${CMAKE_SOURCE_DIR}/../scripts/hu_motor_control.py")
    install(FILES "${CMAKE_SOURCE_DIR}/../scripts/hu_motor_control.py" DESTINATION bin/scripts)
endif()
