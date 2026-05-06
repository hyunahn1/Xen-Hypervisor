SUMMARY = "PiRacer Dom0 base image"
DESCRIPTION = "Dom0 base image skeleton for Xen management and backend services"
LICENSE = "MIT"

inherit core-image

IMAGE_FEATURES += "ssh-server-openssh debug-tweaks"

XEN_KERNEL_MODULES ?= " \
    kernel-module-xen-blkback \
    kernel-module-xen-gntalloc \
    kernel-module-xen-gntdev \
    kernel-module-xen-netback \
    kernel-module-xen-wdt \
"

IMAGE_INSTALL:append = " \
    packagegroup-core-boot \
    ${XEN_KERNEL_MODULES} \
    xen \
    xen-tools \
    bridge-utils \
    iproute2 \
    iptables \
    kernel-image \
    kernel-vmlinux \
    openssh-sftp-server \
    procps \
    util-linux \
"

do_build[depends] += "xen:do_deploy"

do_check_xen_state() {
    if [ "${@bb.utils.contains('DISTRO_FEATURES', 'xen', 'yes', 'no', d)}" = "no" ]; then
        die "DISTRO_FEATURES does not contain 'xen'"
    fi
}

addtask check_xen_state before do_rootfs
