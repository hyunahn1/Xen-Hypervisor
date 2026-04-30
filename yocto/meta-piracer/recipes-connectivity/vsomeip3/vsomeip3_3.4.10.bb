SUMMARY = "vSomeIP - AUTOSAR SOME/IP implementation by COVESA"
DESCRIPTION = "vsomeip3 is the open-source SOME/IP implementation used for \
               service-oriented communication between Head Unit and Instrument Cluster."
HOMEPAGE = "https://github.com/COVESA/vsomeip"
LICENSE = "MPL-2.0"
LIC_FILES_CHKSUM = "file://LICENSE;md5=9741c346eef56131163e13b9db1241b3"

DEPENDS = "boost"

SRC_URI = "git://github.com/COVESA/vsomeip.git;protocol=https;branch=master;nobranch=1"
SRCREV = "01a0df9e9b993297367e843b75291d9aa06909e5"

S = "${WORKDIR}/git"

inherit cmake pkgconfig

EXTRA_OECMAKE = "\
    -DCMAKE_BUILD_TYPE=Release \
    -DENABLE_SIGNAL_HANDLING=1 \
    -DDIAGNOSIS_ADDRESS=0x10 \
    -DENABLE_TESTS=OFF \
    -DBUILD_TESTS=OFF \
    -DENABLE_MULTIPLE_ROUTING_MANAGERS=OFF \
"

do_configure:prepend() {
    # test/CMakeLists.txt를 비워서 googletest 의존성 완전 제거
    echo "# tests disabled" > ${S}/test/CMakeLists.txt
    rm -rf ${B}/_deps ${B}/CMakeCache.txt
}

# boost::asio가 필요하므로 boost-dev 확인
PACKAGECONFIG ??= ""

FILES:${PN}       = " \
    ${libdir}/libvsomeip3*.so.* \
    ${sysconfdir}/vsomeip/ \
    /usr/etc/vsomeip/ \
"
FILES:${PN}-dev   = " \
    ${libdir}/libvsomeip3*.so \
    ${includedir}/vsomeip/ \
    ${includedir}/compat/ \
    ${libdir}/cmake/vsomeip3/ \
    ${libdir}/pkgconfig/ \
"
FILES:${PN}-dbg   = "${libdir}/.debug/"

RDEPENDS:${PN} = "boost"
