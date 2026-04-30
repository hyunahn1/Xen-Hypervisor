/**
 * @file main.cpp (media module)
 *
 * Wayland 클라이언트로 hu_shell의 HUCompositor에 연결합니다.
 * 환경변수: QT_QPA_PLATFORM=wayland, WAYLAND_DISPLAY=wayland-hu (ModuleController가 설정)
 *
 * standalone 실행: ./hu_module_media
 * shell 관리 모드: ./hu_module_media /tmp/hu_shell_media.sock
 */

#include "MediaWindow.h"
#include "ShellClient.h"
#include "GearStateManager.h"

#include <QApplication>
#include <QDebug>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("HU Media");

    const bool standalone = (argc < 2);
    const QString socketPath = standalone ? QString() : QString(argv[1]);

    auto *gearManager = new GearStateManager(&app);
    auto *window      = new MediaWindow(gearManager);
    window->setWindowFlags(Qt::FramelessWindowHint | Qt::Window);
    window->setWindowTitle("media"); // HUCompositor 식별자

    if (standalone) {
        window->showFullScreen();
        qDebug() << "[hu_module_media] standalone mode";
    } else {
        auto *bridge = new ShellClient(socketPath, &app);

        QObject::connect(bridge, &ShellClient::gearStateUpdated,
                         gearManager, [gearManager](GearState g) {
            gearManager->setGear(g, "shell");
        });
        QObject::connect(gearManager, &GearStateManager::gearChanged,
                         bridge, [bridge](GearState g, const QString &src) {
            if (src != "shell")
                bridge->requestGearChange(g, src);
        });
        QObject::connect(bridge, &ShellClient::shellShutdown, &app, &QApplication::quit);

        // Wayland: 바로 show() — winId embedding 불필요
        QObject::connect(bridge, &ShellClient::connected, [window]() {
            window->showFullScreen();
        });

        bridge->connectToShell();
    }

    return app.exec();
}
