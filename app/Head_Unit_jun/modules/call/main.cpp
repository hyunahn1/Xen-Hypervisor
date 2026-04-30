/**
 * @file main.cpp (call module)
 *
 * Wayland 클라이언트로 hu_shell의 HUCompositor에 연결합니다.
 */

#include "CallService.h"
#include "CallWindow.h"
#include "ShellClient.h"
#include "GearStateManager.h"

#include <QApplication>
#include <QDebug>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("HU Call");

    const bool standalone = (argc < 2);
    const QString socketPath = standalone ? QString() : QString(argv[1]);

    auto *gearManager = new GearStateManager(&app);
    auto *service     = new CallService(&app);
    auto *window      = new CallScreen(gearManager);
    window->setWindowFlags(Qt::FramelessWindowHint | Qt::Window);
    window->setWindowTitle("call"); // HUCompositor 식별자

    if (standalone) {
        window->showFullScreen();
        qDebug() << "[hu_module_call] standalone mode";
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

        QObject::connect(bridge, &ShellClient::connected, [window]() {
            window->showFullScreen();
        });

        bridge->connectToShell();
    }

    return app.exec();
}
