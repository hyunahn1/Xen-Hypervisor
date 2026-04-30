/**
 * @file HUCompositor.cpp
 */

#include "HUCompositor.h"

#include <QWaylandOutput>
#include <QWaylandXdgShell>
#include <QWaylandXdgSurface>
#include <QWaylandXdgToplevel>
#include <QWaylandSurface>
#include <QDebug>

const QString HUCompositor::kClusterTitle = QStringLiteral("PiRacer Dashboard");

HUCompositor::HUCompositor(QObject *parent)
    : QWaylandCompositor(parent)
{
    // Socket name is relative to XDG_RUNTIME_DIR
    // Module processes must have WAYLAND_DISPLAY=wayland-hu
    setSocketName("wayland-hu");
}

void HUCompositor::setupOutput(const QSize &screenSize)
{
    if (!m_output) {
        m_output = new QWaylandOutput(this, nullptr);
    }
    QRect geo(QPoint(0, 0), screenSize);
    m_output->setAvailableGeometry(geo);

    // Advertise a mode so clients know the screen resolution
    QWaylandOutputMode mode(screenSize, 60000); // 60 Hz
    m_output->addMode(mode, true /*preferred*/);
    m_output->setCurrentMode(mode);
}

void HUCompositor::setupClusterOutput(QWindow *window, const QSize &clusterSize)
{
    // Physical DSI is portrait (400x1280) but cluster app expects landscape.
    // Report swapped dimensions so the cluster renders in landscape orientation.
    const QSize landscapeSize(clusterSize.height(), clusterSize.width());

    if (!m_clusterOutput) {
        m_clusterOutput = new QWaylandOutput(this, window);
    }
    QRect geo(QPoint(0, 0), landscapeSize);
    m_clusterOutput->setAvailableGeometry(geo);

    QWaylandOutputMode mode(landscapeSize, 60000);
    m_clusterOutput->addMode(mode, true /*preferred*/);
    m_clusterOutput->setCurrentMode(mode);

    qDebug() << "[HUCompositor] cluster output configured (landscape):" << landscapeSize;
}

void HUCompositor::create()
{
    m_xdgShell = new QWaylandXdgShell(this);
    connect(m_xdgShell, &QWaylandXdgShell::toplevelCreated,
            this, &HUCompositor::onXdgToplevelCreated);

    QWaylandCompositor::create();
    qDebug() << "[HUCompositor] created, socket=" << socketName()
             << "XDG_RUNTIME_DIR=" << qgetenv("XDG_RUNTIME_DIR")
             << "clusterOutput=" << (m_clusterOutput ? "yes" : "no");
}

void HUCompositor::onXdgToplevelCreated(QWaylandXdgToplevel *toplevel,
                                         QWaylandXdgSurface  *xdgSurface)
{
    QWaylandSurface *surface = xdgSurface->surface();
    const QString title = toplevel->title();

    if (!title.isEmpty()) {
        dispatchSurface(toplevel, surface, title);
    } else {
        connect(toplevel, &QWaylandXdgToplevel::titleChanged,
                this, [this, toplevel, surface]() {
            const QString t = toplevel->title();
            if (!t.isEmpty())
                dispatchSurface(toplevel, surface, t);
        });
    }
}

void HUCompositor::dispatchSurface(QWaylandXdgToplevel *toplevel,
                                    QWaylandSurface     *surface,
                                    const QString       &title)
{
    if (title == kClusterTitle) {
        // Instrument cluster connects as a Wayland client → render on DSI-1
        if (m_clusterOutput) {
            toplevel->sendFullscreen(m_clusterOutput->geometry().size());
            qDebug() << "[HUCompositor] cluster surface registered, size="
                     << m_clusterOutput->geometry().size();
        } else {
            // No cluster output configured — fall back to main output size
            if (m_output)
                toplevel->sendFullscreen(m_output->geometry().size());
            qWarning() << "[HUCompositor] cluster surface arrived but no cluster output configured";
        }

        if (m_surfaces.contains(title)) return;
        m_surfaces.insert(title, surface);
        connect(surface, &QWaylandSurface::destroyed, this, [this, title]() {
            m_surfaces.remove(title);
            emit clusterSurfaceDestroyed();
        });
        emit clusterSurfaceCreated(surface);
    } else {
        // Regular HU module → main HDMI output
        if (m_output)
            toplevel->sendFullscreen(m_output->geometry().size());
        registerSurface(title, surface);
    }
}

void HUCompositor::registerSurface(const QString &name, QWaylandSurface *surface)
{
    if (m_surfaces.contains(name)) return;

    m_surfaces.insert(name, surface);
    qDebug() << "[HUCompositor] module surface registered:" << name
             << "pid=" << surface->client()->processId();

    connect(surface, &QWaylandSurface::destroyed, this, [this, name]() {
        m_surfaces.remove(name);
        qDebug() << "[HUCompositor] module surface destroyed:" << name;
        emit moduleSurfaceDestroyed(name);
    });

    emit moduleSurfaceCreated(name, surface);
}

QWaylandSurface *HUCompositor::surfaceForModule(const QString &moduleName) const
{
    return m_surfaces.value(moduleName, nullptr);
}
