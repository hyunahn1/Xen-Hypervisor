/**
 * @file HUCompositor.h
 * @brief Head Unit Wayland Compositor
 *
 * hu_shell 프로세스가 Wayland 컴포지터로 동작합니다.
 * 각 모듈 프로세스는 이 컴포지터에 Wayland 클라이언트로 연결됩니다.
 * 소켓: $XDG_RUNTIME_DIR/wayland-hu
 */

#ifndef HUCOMPOSITOR_H
#define HUCOMPOSITOR_H

#include <QWaylandCompositor>
#include <QMap>
#include <QString>
#include <QSize>
#include <QRect>

class QWaylandXdgShell;
class QWaylandXdgSurface;
class QWaylandXdgToplevel;
class QWaylandSurface;
class QWaylandOutput;
class QWindow;

class HUCompositor : public QWaylandCompositor
{
    Q_OBJECT

public:
    explicit HUCompositor(QObject *parent = nullptr);
    ~HUCompositor() override = default;

    void setupOutput(const QSize &screenSize);
    void setupClusterOutput(QWindow *window, const QSize &clusterSize);
    void create() override;

    QWaylandSurface *surfaceForModule(const QString &moduleName) const;
    QWaylandOutput  *mainOutput()    const { return m_output; }
    QWaylandOutput  *clusterOutput() const { return m_clusterOutput; }

signals:
    void moduleSurfaceCreated(const QString &moduleName, QWaylandSurface *surface);
    void moduleSurfaceDestroyed(const QString &moduleName);
    void clusterSurfaceCreated(QWaylandSurface *surface);
    void clusterSurfaceDestroyed();

private slots:
    void onXdgToplevelCreated(QWaylandXdgToplevel *toplevel,
                              QWaylandXdgSurface  *xdgSurface);

private:
    void registerSurface(const QString &name, QWaylandSurface *surface);
    void dispatchSurface(QWaylandXdgToplevel *toplevel,
                         QWaylandSurface     *surface,
                         const QString       &title);

    QWaylandXdgShell                 *m_xdgShell      = nullptr;
    QWaylandOutput                   *m_output        = nullptr;
    QWaylandOutput                   *m_clusterOutput = nullptr;
    QMap<QString, QWaylandSurface *>  m_surfaces;

    static const QString kClusterTitle; // "PiRacerDashboard"
};

#endif // HUCOMPOSITOR_H
