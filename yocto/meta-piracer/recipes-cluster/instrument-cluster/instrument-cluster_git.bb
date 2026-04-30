SUMMARY = "PiRacer Instrument Cluster"
DESCRIPTION = "Qt-based PiRacer instrument cluster dashboard"
LICENSE = "CLOSED"

SRC_URI = "file://piracer-cluster.init"

inherit cmake_qt5 pkgconfig update-rc.d externalsrc

# Use the repository's local source tree during development.
EXTERNALSRC = "${TOPDIR}/../../app/Head_Unit_jun/instrument_cluster"
S = "${EXTERNALSRC}"
B = "${WORKDIR}/build"
# Suppress checksum warnings for externalsrc helper symlinks (oe-workdir/oe-logs).
EXTERNALSRC_SYMLINKS = ""

DEPENDS += "qtbase qtserialport vsomeip3 boost"

EXTRA_OECMAKE += " \
    -DCMAKE_BUILD_TYPE=Release \
    -DIC_HAS_VSOMEIP=ON \
"

# The generated CMake toolchain file lives in WORKDIR, which can be recreated
# while BitBake still has an older do_generate_toolchain_file stamp.
do_configure[prefuncs] += "cmake_do_generate_toolchain_file"

# SysVinit: start at priority 99 (after hu-shell at 98).
# hu-shell must start first to expose the Wayland compositor socket.
# PiRacerDashboard is the vSomeIP routing manager, but vSomeIP clients
# retry automatically, so boot order for vSomeIP is not critical.
INITSCRIPT_NAME = "piracer-cluster"
INITSCRIPT_PARAMS = "defaults 99"

do_install:append() {
    install -d ${D}${sysconfdir}/init.d
    install -m 0755 ${THISDIR}/files/piracer-cluster.init ${D}${sysconfdir}/init.d/piracer-cluster
}

FILES:${PN} += " \
    ${bindir}/PiRacerDashboard \
    ${bindir}/config/ \
    ${bindir}/python/ \
    ${sysconfdir}/init.d/piracer-cluster \
"

# Python bridge runtime dependencies
RDEPENDS:${PN} += "python3 python3-can python3-pyserial python3-core python3-json"
