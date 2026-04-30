SUMMARY = "PiRacer Head Unit DomU image"
DESCRIPTION = "DomU image that contains the Head Unit application stack"
LICENSE = "MIT"

inherit core-image

IMAGE_FEATURES += "ssh-server-openssh debug-tweaks"

IMAGE_INSTALL:append = " \
    headunit \
    instrument-cluster \
    qtbase-plugins \
    qtwayland \
    qtwayland-plugins \
    vsomeip3 \
    ttf-dejavu-sans \
    ttf-dejavu-common \
    fontconfig \
    can-utils \
    python3 \
    python3-can \
    python3-pyserial \
    python3-core \
    python3-json \
    python3-misc \
    python3-piracer \
    python3-smbus2 \
    i2c-tools \
    wpa-supplicant \
    openssh-sftp-server \
"

# weston 제거: hu_shell이 QtWayland Compositor로 동작하므로 별도 Weston 불필요
# hu_shell은 eglfs로 DRM/KMS에 직접 접근하는 Wayland 컴포지터입니다.

# WiFi 설정 + network interfaces 후처리로 덮어쓰기
ROOTFS_POSTPROCESS_COMMAND:append = " setup_wifi_config; setup_ssh_config; setup_serial_console; setup_touch_driver_config; "

setup_wifi_config() {
    # wpa_supplicant.conf 작성
    printf 'ctrl_interface=/var/run/wpa_supplicant\nctrl_interface_group=0\nupdate_config=1\n\nnetwork={\n    ssid="SEA:ME WiFi Access"\n    psk="1fy0u534m3"\n}\n' > ${IMAGE_ROOTFS}/etc/wpa_supplicant.conf
    chmod 600 ${IMAGE_ROOTFS}/etc/wpa_supplicant.conf

    # /etc/network/interfaces 전체 덮어쓰기 (sed 방식은 기본 파일 형식에 따라 실패할 수 있음)
    mkdir -p ${IMAGE_ROOTFS}/etc/network
    printf 'auto lo\niface lo inet loopback\n\nauto wlan0\niface wlan0 inet dhcp\n\tauto-driver wext\n\twpa-conf /etc/wpa_supplicant.conf\n\nauto eth0\niface eth0 inet dhcp\n' > ${IMAGE_ROOTFS}/etc/network/interfaces

    # wpa_supplicant 서비스 활성화 (systemd)
    if [ -d ${IMAGE_ROOTFS}/lib/systemd/system ]; then
        mkdir -p ${IMAGE_ROOTFS}/etc/systemd/system/multi-user.target.wants
        ln -sf /lib/systemd/system/wpa_supplicant.service \
            ${IMAGE_ROOTFS}/etc/systemd/system/multi-user.target.wants/wpa_supplicant.service || true
    fi
}

setup_ssh_config() {
    # PasswordAuthentication 활성화
    sed -i 's/#PasswordAuthentication yes/PasswordAuthentication yes/' ${IMAGE_ROOTFS}/etc/ssh/sshd_config
    sed -i 's/#PasswordAuthentication no/PasswordAuthentication yes/'  ${IMAGE_ROOTFS}/etc/ssh/sshd_config
    sed -i 's/^PasswordAuthentication no/PasswordAuthentication yes/'  ${IMAGE_ROOTFS}/etc/ssh/sshd_config

    # PermitRootLogin 활성화 (없으면 비밀번호로 root 로그인 불가)
    sed -i 's/#PermitRootLogin.*/PermitRootLogin yes/' ${IMAGE_ROOTFS}/etc/ssh/sshd_config
    grep -q "^PermitRootLogin" ${IMAGE_ROOTFS}/etc/ssh/sshd_config \
        || echo "PermitRootLogin yes" >> ${IMAGE_ROOTFS}/etc/ssh/sshd_config
}

# UART 시리얼 콘솔 활성화
# eglfs가 DRM을 독점해서 Ctrl+Alt+F2 VT 전환이 안 되므로
# GPIO 14/15 (UART0) 또는 USB-TTL 어댑터로 터미널 접근 가능
setup_serial_console() {
    if [ -d ${IMAGE_ROOTFS}/lib/systemd/system ]; then
        mkdir -p ${IMAGE_ROOTFS}/etc/systemd/system/getty.target.wants
        ln -sf /lib/systemd/system/serial-getty@.service \
            ${IMAGE_ROOTFS}/etc/systemd/system/getty.target.wants/serial-getty@ttyAMA0.service || true
    fi
}

# The Waveshare 7.9" DSI overlay exposes both Goodix (0x14) and Ilitek
# (0x41) touch nodes. Our panel uses the Goodix path; probing the unused
# Ilitek node floods the console with ili210x I2C read failures.
setup_touch_driver_config() {
    mkdir -p ${IMAGE_ROOTFS}/etc/modprobe.d
    printf 'blacklist ili210x\n' > ${IMAGE_ROOTFS}/etc/modprobe.d/blacklist-ili210x.conf
}
