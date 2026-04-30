/**
 * @file main.cpp
 * @brief Head Unit Shell 진입점
 */

#include "ShellWindow.h"
#include <QApplication>
#include <QByteArray>

int main(int argc, char *argv[])
{
    // Use eglfs for embedded RPi4 (no X11/Wayland available)
    if (qgetenv("QT_QPA_PLATFORM").isEmpty()) {
        qputenv("QT_QPA_PLATFORM", "eglfs");
    }

    QApplication app(argc, argv);
    app.setApplicationName("PiRacer Head Unit Shell");
    app.setApplicationVersion("2.0.0");

    ShellWindow w;
    w.show();

    return app.exec();
}
