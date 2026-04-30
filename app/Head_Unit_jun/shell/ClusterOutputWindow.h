/**
 * @file ClusterOutputWindow.h
 * @brief Standalone OpenGL window that renders the instrument cluster
 *        Wayland surface on the DSI-1 screen.
 *
 * hu_shell is the Wayland compositor (eglfs, DRM master on card1).
 * As DRM master it owns both HDMI-A-1 and DSI-1.
 * PiRacerDashboard connects as a Wayland client; this window blits
 * its surface contents onto the DSI-1 physical display.
 */

#ifndef CLUSTEROUTPUTWINDOW_H
#define CLUSTEROUTPUTWINDOW_H

#include <QOpenGLWindow>
#include <QOpenGLFunctions>
#include <QOpenGLTextureBlitter>

class QWaylandSurface;
class QWaylandView;
class QWaylandOutput;
class QScreen;

class ClusterOutputWindow : public QOpenGLWindow, protected QOpenGLFunctions
{
    Q_OBJECT

public:
    explicit ClusterOutputWindow(QScreen *screen, QObject *parent = nullptr);
    ~ClusterOutputWindow() override;

    void setWaylandOutput(QWaylandOutput *output) { m_waylandOutput = output; }
    void setSurface(QWaylandSurface *surface);
    void clearSurface();

protected:
    void exposeEvent(QExposeEvent *event) override;
    void initializeGL() override;
    void paintGL()      override;
    void resizeGL(int w, int h) override;

private:
    QWaylandOutput        *m_waylandOutput = nullptr;
    QWaylandView          *m_view          = nullptr;
    QOpenGLTextureBlitter  m_blitter;
    bool                   m_blitterInit   = false;
};

#endif // CLUSTEROUTPUTWINDOW_H
