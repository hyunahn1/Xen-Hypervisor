SUMMARY = "PiRacer Python library (vehicles, gamepads, sensors)"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

SRC_URI = " \
    file://piracer/__init__.py \
    file://piracer/vehicles.py \
    file://piracer/gamepads.py \
    file://piracer/cameras.py \
"

S = "${WORKDIR}"

inherit allarch python3-dir

RDEPENDS:${PN} = "python3-core"

do_install() {
    install -d ${D}${PYTHON_SITEPACKAGES_DIR}/piracer
    install -m 0644 ${WORKDIR}/piracer/__init__.py  ${D}${PYTHON_SITEPACKAGES_DIR}/piracer/
    install -m 0644 ${WORKDIR}/piracer/vehicles.py  ${D}${PYTHON_SITEPACKAGES_DIR}/piracer/
    install -m 0644 ${WORKDIR}/piracer/gamepads.py  ${D}${PYTHON_SITEPACKAGES_DIR}/piracer/
    install -m 0644 ${WORKDIR}/piracer/cameras.py   ${D}${PYTHON_SITEPACKAGES_DIR}/piracer/
}

FILES:${PN} += "${PYTHON_SITEPACKAGES_DIR}/piracer"
