/**
 * @file ModuleSurfaceWidget.cpp
 */

#include "ModuleSurfaceWidget.h"

#include <QWaylandView>
#include <QWaylandSurface>
#include <QWaylandBufferRef>
#include <QWaylandCompositor>
#include <QWaylandSeat>
#include <QOpenGLTexture>
#include <QPainter>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QKeyEvent>
#include <QDebug>

ModuleSurfaceWidget::ModuleSurfaceWidget(QWidget *parent)
    : QOpenGLWidget(parent)
{
    setStyleSheet("background:#0D0D0F;");
    setFocusPolicy(Qt::StrongFocus);   // accept keyboard focus on click
    setMouseTracking(true);             // receive mouseMoveEvent without button held
}

ModuleSurfaceWidget::~ModuleSurfaceWidget()
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

void ModuleSurfaceWidget::setSurface(QWaylandSurface *surface)
{
    // Clean up previous view
    if (m_view) {
        if (m_view->surface())
            disconnect(m_view->surface(), nullptr, this, nullptr);
        delete m_view;
        m_view = nullptr;
    }

    if (surface) {
        m_view = new QWaylandView();
        m_view->setSurface(surface);

        // Trigger repaint every time the surface commits a new frame
        connect(surface, &QWaylandSurface::redraw,
                this, QOverload<>::of(&QOpenGLWidget::update));
        connect(surface, &QWaylandSurface::hasContentChanged,
                this, QOverload<>::of(&QOpenGLWidget::update));

        // Auto-clear if the surface is destroyed externally
        connect(surface, &QWaylandSurface::destroyed,
                this, &ModuleSurfaceWidget::clearSurface);

        qDebug() << "[ModuleSurfaceWidget] setSurface: hasContent=" << surface->hasContent();
    }
    update();
}

void ModuleSurfaceWidget::clearSurface()
{
    if (m_view) {
        delete m_view;
        m_view = nullptr;
    }
    update();
}

QWaylandSurface *ModuleSurfaceWidget::currentSurface() const
{
    return m_view ? m_view->surface() : nullptr;
}

void ModuleSurfaceWidget::initializeGL()
{
    initializeOpenGLFunctions();
}

void ModuleSurfaceWidget::paintGL()
{
    if (!m_blitterInit) {
        m_blitter.create();
        m_blitterInit = true;
    }

    glClearColor(0.051f, 0.051f, 0.059f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    if (!m_view || !m_view->surface()) {
        // Fallback: draw loading text via QPainter
        QPainter p(this);
        p.setPen(QColor(180, 180, 180));
        p.setFont(QFont("DejaVu Sans", 14));
        p.drawText(rect(), Qt::AlignCenter, tr("모듈 로딩 중..."));
        return;
    }

    QWaylandSurface *surface = m_view->surface();
    surface->frameStarted();

    // advance() moves nextBuffer → currentBuffer; must be called before currentBuffer()
    m_view->advance();

    QWaylandBufferRef buf = m_view->currentBuffer();
    qDebug() << "[ModuleSurfaceWidget] paintGL: hasContent=" << buf.hasContent()
             << "size=" << surface->destinationSize();

    if (buf.hasContent()) {
        QOpenGLTexture *tex = buf.toOpenGLTexture();
        if (tex) {
            m_blitter.bind(tex->target());
            const QMatrix4x4 target = QOpenGLTextureBlitter::targetTransform(
                QRectF(0, 0, width(), height()),
                QRect(0, 0, width(), height())
            );
            // OriginTopLeft: V is flipped to match xcomposite-glx convention on X11.
            // On RPi (eglfs + wayland-egl), test and adjust if needed.
            m_blitter.blit(tex->textureId(), target,
                           QOpenGLTextureBlitter::OriginTopLeft);
            m_blitter.release();
        }
    }

    surface->sendFrameCallbacks();
}

void ModuleSurfaceWidget::resizeGL(int w, int h)
{
    glViewport(0, 0, w, h);
}

// ── Input forwarding ─────────────────────────────────────────────────────────

QPointF ModuleSurfaceWidget::mapToSurface(const QPointF &widgetPos) const
{
    if (!m_view || !m_view->surface()) return widgetPos;
    const QSize surfSize = m_view->surface()->destinationSize();
    if (surfSize.isEmpty()) return widgetPos;

    // OriginTopLeft renders the surface correctly (top-left = top-left).
    // Input mapping is the same: widget (0,0) → surface (0,0), no Y inversion needed.
    const float sx = (float(widgetPos.x()) / float(width()))  * float(surfSize.width());
    const float sy = (float(widgetPos.y()) / float(height())) * float(surfSize.height());
    return QPointF(sx, sy);
}

static QWaylandSeat *seatFor(QWaylandSurface *surface) {
    return surface ? surface->compositor()->defaultSeat() : nullptr;
}

void ModuleSurfaceWidget::mousePressEvent(QMouseEvent *event)
{
    setFocus();
    if (!m_view || !m_view->surface()) return;
    QWaylandSeat *seat = seatFor(m_view->surface());
    if (!seat) return;

    const QPointF surfPos = mapToSurface(event->localPos());
    qDebug() << "[Input] press widget=" << event->localPos() << "→ surf=" << surfPos
             << "surfSize=" << m_view->surface()->destinationSize();
    // Establish mouse focus on this view so the surface receives the event
    seat->sendMouseMoveEvent(m_view, surfPos, event->screenPos());
    seat->sendMousePressEvent(event->button());

    // Also set keyboard focus so key events reach the module
    seat->setKeyboardFocus(m_view->surface());
    event->accept();
}

void ModuleSurfaceWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (!m_view || !m_view->surface()) return;
    QWaylandSeat *seat = seatFor(m_view->surface());
    if (!seat) return;
    seat->sendMouseMoveEvent(m_view, mapToSurface(event->localPos()), event->screenPos());
    seat->sendMouseReleaseEvent(event->button());
    event->accept();
}

void ModuleSurfaceWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (!m_view || !m_view->surface()) return;
    QWaylandSeat *seat = seatFor(m_view->surface());
    if (!seat) return;
    seat->sendMouseMoveEvent(m_view, mapToSurface(event->localPos()), event->screenPos());
    event->accept();
}

void ModuleSurfaceWidget::wheelEvent(QWheelEvent *event)
{
    if (!m_view || !m_view->surface()) return;
    QWaylandSeat *seat = seatFor(m_view->surface());
    if (!seat) return;
    const QPoint delta = event->angleDelta();
    if (delta.y() != 0)
        seat->sendMouseWheelEvent(Qt::Vertical, delta.y());
    if (delta.x() != 0)
        seat->sendMouseWheelEvent(Qt::Horizontal, delta.x());
    event->accept();
}

void ModuleSurfaceWidget::keyPressEvent(QKeyEvent *event)
{
    if (!m_view || !m_view->surface()) return;
    QWaylandSeat *seat = seatFor(m_view->surface());
    if (!seat) return;
    seat->sendFullKeyEvent(event);
    event->accept();
}

void ModuleSurfaceWidget::keyReleaseEvent(QKeyEvent *event)
{
    if (!m_view || !m_view->surface()) return;
    QWaylandSeat *seat = seatFor(m_view->surface());
    if (!seat) return;
    seat->sendFullKeyEvent(event);
    event->accept();
}
