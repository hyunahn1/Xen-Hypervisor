/**
 * @file main.cpp (youtube module)
 *
 * Wayland 클라이언트로 hu_shell의 HUCompositor에 연결합니다.
 * YouTube/WebEngine 크래시 시 이 프로세스만 종료, 다른 모듈은 영향 없음.
 */

#include "YouTubeService.h"
#include "YouTubeWindow.h"
#include "ShellClient.h"
#include "GearStateManager.h"

#include <QApplication>
#include <QDebug>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("HU YouTube");

    const bool standalone = (argc < 2);
    const QString socketPath = standalone ? QString() : QString(argv[1]);

    auto *gearManager = new GearStateManager(&app);
    auto *service     = new YouTubeService(&app);
    auto *window      = new YouTubeScreen(gearManager);
    window->setWindowFlags(Qt::FramelessWindowHint | Qt::Window);
    window->setWindowTitle("youtube"); // HUCompositor 식별자

    if (standalone) {
        window->showFullScreen();
        qDebug() << "[hu_module_youtube] standalone mode";
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
