# VSOMEIP (optional) - Head Unit ↔ Instrument Cluster IPC
find_path(VSOMEIP_INCLUDE_DIR NAMES vsomeip/vsomeip.hpp)
find_library(VSOMEIP_LIBRARY NAMES vsomeip3)
if(VSOMEIP_INCLUDE_DIR AND VSOMEIP_LIBRARY)
    message(STATUS "VSOMEIP found - gear IPC enabled")
    add_compile_definitions(HU_HAS_VSOMEIP)
endif()

add_library(hu_core STATIC
    interfaces/IVehicleDataProvider.h
    interfaces/ILedController.h
    interfaces/IPdcSensorProvider.h
    ipc/MockVehicleDataProvider.h
    ipc/MockVehicleDataProvider.cpp
    ipc/VSomeIPClient.h
    ipc/VSomeIPClient.cpp
    ipc/MockPdcSensorProvider.h
    ipc/MockPdcSensorProvider.cpp
    ipc/SocketCanPdcProvider.h
    ipc/SocketCanPdcProvider.cpp
    ipc/MockLedController.h
    ipc/MockLedController.cpp
    models/PdcTypes.h
    models/GearStateManager.h
    models/GearStateManager.cpp
    protocol/ShellProtocol.h
    protocol/ShellProtocol.cpp
    protocol/ShellClient.h
    protocol/ShellClient.cpp
)

# PUBLIC: 모든 링크 대상이 이 include 경로를 상속
target_include_directories(hu_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/interfaces
    ${CMAKE_CURRENT_SOURCE_DIR}/ipc
    ${CMAKE_CURRENT_SOURCE_DIR}/models
    ${CMAKE_CURRENT_SOURCE_DIR}/protocol
)

target_link_libraries(hu_core PUBLIC
    Qt5::Core
    Qt5::Network    # QLocalSocket / QLocalServer
)
if(VSOMEIP_INCLUDE_DIR AND VSOMEIP_LIBRARY)
    target_include_directories(hu_core PUBLIC ${VSOMEIP_INCLUDE_DIR})
    target_link_libraries(hu_core PUBLIC ${VSOMEIP_LIBRARY})
endif()
