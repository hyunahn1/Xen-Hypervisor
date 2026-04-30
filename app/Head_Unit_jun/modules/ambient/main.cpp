/**
 * @file main.cpp (ambient module)
 *
 * Wayland 클라이언트로 hu_shell의 HUCompositor에 연결합니다.
 * LED 색상 변경은 ModuleBridge를 통해 hu_shell의 GlowOverlay에 전달됩니다.
 */

#include "AmbientService.h"
#include "AmbientWindow.h"
#include "ShellClient.h"
#include "GearStateManager.h"
#include "MockLedController.h"

#include <QApplication>
#include <QDebug>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("HU Ambient");

    const bool standalone = (argc < 2);
    const QString socketPath = standalone ? QString() : QString(argv[1]);

    auto *gearManager = new GearStateManager(&app);
    auto *led         = new MockLedController(&app);
    auto *service     = new AmbientService(led, &app);
    auto *window      = new AmbientScreen(led, gearManager);
    window->setWindowFlags(Qt::FramelessWindowHint | Qt::Window);
    window->setWindowTitle("ambient"); // HUCompositor 식별자

    if (standalone) {
        window->showFullScreen();
        qDebug() << "[hu_module_ambient] standalone mode";
    } else {
        auto *bridge = new ShellClient(socketPath, &app);

        // LED 색상 → shell 전달 (GlowOverlay 업데이트)
        QObject::connect(window, &AmbientScreen::ambientColorChanged,
                         bridge, [bridge](uint8_t r, uint8_t g, uint8_t b, int brightness) {
            bridge->sendAmbientColor(r, g, b, static_cast<quint8>(brightness));
        });
        QObject::connect(window, &AmbientScreen::ambientOff,
                         bridge, &ShellClient::sendAmbientOff);
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
