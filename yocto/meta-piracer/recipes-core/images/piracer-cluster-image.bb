SUMMARY = "PiRacer Instrument Cluster DomU image"
DESCRIPTION = "DomU image that contains the Instrument Cluster stack"
LICENSE = "MIT"

inherit core-image

IMAGE_FEATURES += "ssh-server-openssh"

IMAGE_INSTALL:append = " \
    instrument-cluster \
    qtbase-plugins \
    vsomeip3 \
"
