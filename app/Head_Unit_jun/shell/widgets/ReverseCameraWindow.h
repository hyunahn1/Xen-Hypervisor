#ifndef REVERSECAMERAWINDOW_H
#define REVERSECAMERAWINDOW_H

#include "PdcTypes.h"

#include <QWidget>
#include <QPixmap>
#include <QImage>
#include <QString>
#include <QTimer>

class GearPanel;
class GearStateManager;

class ReverseCameraWindow : public QWidget
{
    Q_OBJECT

public:
    explicit ReverseCameraWindow(GearStateManager *gearState = nullptr, QWidget *parent = nullptr);
    ~ReverseCameraWindow() override;

public slots:
    void setPdcState(const PdcState &state);

protected:
    void paintEvent(QPaintEvent *event) override;

private slots:
    void onPullFrame();

private:
    void buildPlaceholderPixmap();
    bool startCameraPreview();
    void stopCameraPreview();
    void handlePipelineMessages();
    void fallbackToPlaceholder(const QString &reason);

    QPixmap m_placeholder;
    GearPanel *m_gearPanel = nullptr;
    bool    m_showPlaceholder = true;
    QImage  m_frame;
    PdcState m_pdcState;
    QTimer *m_frameTimer  = nullptr;
    int     m_noFrameCount = 0;

    // GstElement* stored as void* to keep GStreamer headers out of .h
    void *m_pipeline = nullptr;
    void *m_appsink  = nullptr;
};

#endif // REVERSECAMERAWINDOW_H
