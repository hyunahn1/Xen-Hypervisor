/**
 * @file ClusterOutputWindow.cpp
 */

#include "ClusterOutputWindow.h"

#include <QWaylandSurface>
#include <QWaylandView>
#include <QWaylandOutput>
#include <QWaylandBufferRef>
#include <QOpenGLTexture>
#include <QScreen>
#include <QExposeEvent>

ClusterOutputWindow::ClusterOutputWindow(QScreen *screen, QObject *parent)
    : QOpenGLWindow(QOpenGLWindow::NoPartialUpdate, nullptr)
{
    Q_UNUSED(parent)
    if (screen)
        setScreen(screen);
    setTitle("ClusterOutput");
    showFullScreen();
}

ClusterOutputWindow::~ClusterOutputWindow()
{
    makeCurrent();
    if (m_blitterInit)
        m_blitter.destroy();
    if (m_view) {
        if (m_view->surface())
            disconnect(m_view->surface(), nullptr, this, nullptr);
        delete m_view;
    }
    doneCurrent();
}

void ClusterOutputWindow::setSurface(QWaylandSurface *surface)
{
    if (m_view) {
        if (m_view->surface())
            disconnect(m_view->surface(), nullptr, this, nullptr);
        delete m_view;
        m_view = nullptr;
    }

    if (surface) {
        m_view = new QWaylandView();
        m_view->setSurface(surface);
        if (m_waylandOutput)
            m_view->setOutput(m_waylandOutput);

        connect(surface, &QWaylandSurface::redraw,
                this, QOverload<>::of(&QOpenGLWindow::update));
        connect(surface, &QWaylandSurface::hasContentChanged,
                this, QOverload<>::of(&QOpenGLWindow::update));
        connect(surface, &QWaylandSurface::destroyed,
                this, &ClusterOutputWindow::clearSurface);
    }
    update();
}

void ClusterOutputWindow::clearSurface()
{
    if (m_view) {
        delete m_view;
        m_view = nullptr;
    }
    update();
}

void ClusterOutputWindow::exposeEvent(QExposeEvent *event)
{
    QOpenGLWindow::exposeEvent(event);
    if (isExposed() && m_view)
        update();
}

void ClusterOutputWindow::initializeGL()
{
    initializeOpenGLFunctions();
}

void ClusterOutputWindow::paintGL()
{
    if (!m_blitterInit) {
        m_blitter.create();
        m_blitterInit = true;
    }

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    if (!m_view || !m_view->surface())
        return;

    QWaylandSurface *surface = m_view->surface();
    surface->frameStarted();
    m_view->advance();

    QWaylandBufferRef buf = m_view->currentBuffer();
    if (buf.hasContent()) {
        QOpenGLTexture *tex = buf.toOpenGLTexture();
        if (tex) {
            m_blitter.bind(tex->target());
            const QMatrix4x4 target = QOpenGLTextureBlitter::targetTransform(
                QRectF(0, 0, width(), height()),
                QRect(0, 0, width(), height())
            );
            QMatrix4x4 rot;
            rot.rotate(90.0f, 0.0f, 0.0f, 1.0f);
            m_blitter.blit(tex->textureId(), rot * target,
                           QOpenGLTextureBlitter::OriginTopLeft);
            m_blitter.release();
        }
    }

    surface->sendFrameCallbacks();
}

void ClusterOutputWindow::resizeGL(int w, int h)
{
    glViewport(0, 0, w, h);
}
