/**
 * @file main.cpp (settings module)
 *
 * Wayland 클라이언트로 hu_shell의 HUCompositor에 연결합니다.
 * 속도/기어/IPC 상태는 ModuleBridge를 통해 수신합니다.
 */

#include "SettingsService.h"
#include "SettingsWindow.h"
#include "ShellClient.h"
#include "GearStateManager.h"

#include <QApplication>
#include <QDebug>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("HU Settings");

    const bool standalone = (argc < 2);
    const QString socketPath = standalone ? QString() : QString(argv[1]);

    auto *gearManager = new GearStateManager(&app);
    auto *service     = new SettingsService(&app);
    auto *window      = new SettingsScreen(gearManager);
    window->setWindowFlags(Qt::FramelessWindowHint | Qt::Window);
    window->setWindowTitle("settings"); // HUCompositor 식별자

    if (standalone) {
        window->showFullScreen();
        qDebug() << "[hu_module_settings] standalone mode";
    } else {
        auto *bridge = new ShellClient(socketPath, &app);

        // 설정 변경 → shell 전달
        QObject::connect(service, &SettingsService::settingChanged,
                         bridge, [bridge](const QString &key, const QVariant &val) {
            bridge->sendSettingsChanged({{key, val}});
        });
        QObject::connect(bridge, &ShellClient::gearStateUpdated,
                         gearManager, [gearManager](GearState g) {
            gearManager->setGear(g, "shell");
        });
        QObject::connect(bridge, &ShellClient::gearStateUpdated,
                         window, &SettingsScreen::updateGear);
        QObject::connect(bridge, &ShellClient::vehicleSpeedUpdated,
                         window, &SettingsScreen::updateSpeed);
        QObject::connect(bridge, &ShellClient::ipcStatusUpdated,
                         window, &SettingsScreen::updateIpcStatus);
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
