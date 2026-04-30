SUMMARY = "PiRacer Dom0 base image"
DESCRIPTION = "Dom0 base image skeleton for Xen management and backend services"
LICENSE = "MIT"

inherit core-image

IMAGE_FEATURES += "ssh-server-openssh"

# Keep Dom0 minimal by default; add Xen/driver/backend packages later.
IMAGE_INSTALL:append = " \
    packagegroup-core-boot \
"
