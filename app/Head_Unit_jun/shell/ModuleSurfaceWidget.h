/**
 * @file ModuleSurfaceWidget.h
 * @brief Wayland 모듈 surface를 OpenGL로 렌더링하는 위젯
 *
 * QStackedWidget 없이 단일 QOpenGLWidget이 활성 모듈 surface를 표시합니다.
 * 탭 전환 시 setSurface()를 호출하여 렌더링할 surface를 교체합니다.
 */

#ifndef MODULESURFACEWIDGET_H
#define MODULESURFACEWIDGET_H

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLTextureBlitter>
#include <QPointF>

class QWaylandSurface;
class QWaylandView;
class QMouseEvent;
class QWheelEvent;
class QKeyEvent;

class ModuleSurfaceWidget : public QOpenGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT

public:
    explicit ModuleSurfaceWidget(QWidget *parent = nullptr);
    ~ModuleSurfaceWidget() override;

    void setSurface(QWaylandSurface *surface);
    void clearSurface();
    QWaylandSurface *currentSurface() const;

protected:
    void initializeGL() override;
    void paintGL()      override;
    void resizeGL(int w, int h) override;

    // Input forwarding to Wayland surface
    void mousePressEvent(QMouseEvent *event)   override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event)    override;
    void wheelEvent(QWheelEvent *event)        override;
    void keyPressEvent(QKeyEvent *event)       override;
    void keyReleaseEvent(QKeyEvent *event)     override;

private:
    // Map widget coordinates to surface coordinates (accounts for render flip)
    QPointF mapToSurface(const QPointF &widgetPos) const;

    QWaylandView          *m_view        = nullptr;
    QOpenGLTextureBlitter  m_blitter;
    bool                   m_blitterInit = false;
};

#endif // MODULESURFACEWIDGET_H
