#include "ReverseCameraWindow.h"
#include "GearPanel.h"

#include <QDebug>
#include <QFont>
#include <QLinearGradient>
#include <QPainter>

#ifdef HU_CAMERA_PREVIEW_AVAILABLE
#include <gst/gst.h>
#include <gst/app/gstappsink.h>
#include <gst/video/video.h>
#endif

static constexpr int kFrameIntervalMs = 33;   // ~30 fps polling
static constexpr int kNoFrameLimit    = 90;   // ~3 s with no frame → fallback
static constexpr int kScreenW         = 1024;
static constexpr int kScreenH         = 600;
static constexpr int kGearPanelW      = 96;
static constexpr int kCameraAreaW     = kScreenW - kGearPanelW;

ReverseCameraWindow::ReverseCameraWindow(GearStateManager *gearState, QWidget *parent)
    : QWidget(parent)
{
    setWindowTitle("Rear View");
    setWindowFlags(Qt::Window | Qt::FramelessWindowHint);
    setFixedSize(kScreenW, kScreenH);
    setStyleSheet("background-color: #0a0a0c;");

    buildPlaceholderPixmap();

    if (gearState) {
        m_gearPanel = new GearPanel(gearState, this);
        m_gearPanel->setGeometry(0, 0, kGearPanelW, kScreenH);
        m_gearPanel->raise();
    }

    if (startCameraPreview()) {
        m_showPlaceholder = false;
        m_frameTimer = new QTimer(this);
        connect(m_frameTimer, &QTimer::timeout, this, &ReverseCameraWindow::onPullFrame);
        m_frameTimer->start(kFrameIntervalMs);
    }
}

ReverseCameraWindow::~ReverseCameraWindow()
{
    if (m_frameTimer) m_frameTimer->stop();
    stopCameraPreview();
}

void ReverseCameraWindow::setPdcState(const PdcState &state)
{
    m_pdcState = state;
    update();
}

bool ReverseCameraWindow::startCameraPreview()
{
#ifdef HU_CAMERA_PREVIEW_AVAILABLE
    if (!gst_is_initialized())
        gst_init(nullptr, nullptr);

    GError *err = nullptr;
    const char *pipeStr =
        "libcamerasrc ! "
        "videoconvert ! "
        "videoscale ! "
        "video/x-raw,format=RGB,width=1024,height=600,framerate=30/1 ! "
        "appsink name=sink max-buffers=2 drop=true sync=false";

    GstElement *pipeline = gst_parse_launch(pipeStr, &err);
    if (!pipeline || err) {
        qWarning() << "[RearCamera] pipeline parse failed:"
                   << (err ? err->message : "unknown");
        if (err)      g_error_free(err);
        if (pipeline) gst_object_unref(pipeline);
        return false;
    }

    GstElement *appsink = gst_bin_get_by_name(GST_BIN(pipeline), "sink");
    if (!appsink) {
        qWarning() << "[RearCamera] appsink element not found";
        gst_object_unref(pipeline);
        return false;
    }

    GstStateChangeReturn ret = gst_element_set_state(pipeline, GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        qWarning() << "[RearCamera] failed to set pipeline PLAYING";
        gst_object_unref(appsink);
        gst_object_unref(pipeline);
        return false;
    }

    m_pipeline = pipeline;
    m_appsink  = appsink;
    qInfo() << "[RearCamera] libcamerasrc pipeline started";
    return true;
#else
    return false;
#endif
}

void ReverseCameraWindow::stopCameraPreview()
{
#ifdef HU_CAMERA_PREVIEW_AVAILABLE
    if (m_pipeline) {
        gst_element_set_state(reinterpret_cast<GstElement *>(m_pipeline), GST_STATE_NULL);
        if (m_appsink) {
            gst_object_unref(reinterpret_cast<GstElement *>(m_appsink));
            m_appsink = nullptr;
        }
        gst_object_unref(reinterpret_cast<GstElement *>(m_pipeline));
        m_pipeline = nullptr;
    }
#endif
}

void ReverseCameraWindow::onPullFrame()
{
#ifdef HU_CAMERA_PREVIEW_AVAILABLE
    handlePipelineMessages();

    if (!m_appsink) return;

    GstSample *sample = gst_app_sink_try_pull_sample(
        GST_APP_SINK(reinterpret_cast<GstElement *>(m_appsink)), 0);

    if (!sample) {
        if (++m_noFrameCount > kNoFrameLimit) {
            fallbackToPlaceholder(QStringLiteral("no frames received"));
        }
        return;
    }

    m_noFrameCount = 0;
    GstCaps *caps = gst_sample_get_caps(sample);
    GstBuffer *buf = gst_sample_get_buffer(sample);
    if (buf && caps) {
        GstVideoInfo info;
        if (gst_video_info_from_caps(&info, caps) && GST_VIDEO_INFO_FORMAT(&info) == GST_VIDEO_FORMAT_RGB) {
            GstVideoFrame frame;
            if (gst_video_frame_map(&frame, &info, buf, GST_MAP_READ)) {
                const int frameWidth = GST_VIDEO_INFO_WIDTH(&info);
                const int frameHeight = GST_VIDEO_INFO_HEIGHT(&info);
                const int stride = GST_VIDEO_FRAME_PLANE_STRIDE(&frame, 0);
                const uchar *data = static_cast<const uchar *>(GST_VIDEO_FRAME_PLANE_DATA(&frame, 0));
                m_frame = QImage(data, frameWidth, frameHeight, stride, QImage::Format_RGB888).copy();
                gst_video_frame_unmap(&frame);
            } else {
                qWarning() << "[RearCamera] failed to map video frame";
            }
        } else {
            gchar *capsText = caps ? gst_caps_to_string(caps) : nullptr;
            qWarning() << "[RearCamera] unexpected sample caps:"
                       << (capsText ? QString::fromUtf8(capsText) : QStringLiteral("none"));
            if (capsText) g_free(capsText);
        }
        if (!m_frame.isNull()) {
            m_showPlaceholder = false;
            update();
        }
    }
    gst_sample_unref(sample);
#endif
}

void ReverseCameraWindow::handlePipelineMessages()
{
#ifdef HU_CAMERA_PREVIEW_AVAILABLE
    if (!m_pipeline) return;

    GstBus *bus = gst_element_get_bus(reinterpret_cast<GstElement *>(m_pipeline));
    if (!bus) return;

    while (GstMessage *message = gst_bus_pop_filtered(
               bus, static_cast<GstMessageType>(GST_MESSAGE_ERROR | GST_MESSAGE_EOS))) {
        if (GST_MESSAGE_TYPE(message) == GST_MESSAGE_ERROR) {
            GError *error = nullptr;
            gchar *debug = nullptr;
            gst_message_parse_error(message, &error, &debug);
            const QString reason = QStringLiteral("pipeline error: %1")
                                       .arg(error ? QString::fromUtf8(error->message)
                                                  : QStringLiteral("unknown"));
            qWarning() << "[RearCamera]" << reason
                       << (debug ? QString::fromUtf8(debug) : QString());
            if (error) g_error_free(error);
            if (debug) g_free(debug);
            gst_message_unref(message);
            gst_object_unref(bus);
            fallbackToPlaceholder(reason);
            return;
        }

        if (GST_MESSAGE_TYPE(message) == GST_MESSAGE_EOS) {
            gst_message_unref(message);
            gst_object_unref(bus);
            fallbackToPlaceholder(QStringLiteral("pipeline reached EOS"));
            return;
        }

        gst_message_unref(message);
    }

    gst_object_unref(bus);
#endif
}

void ReverseCameraWindow::fallbackToPlaceholder(const QString &reason)
{
#ifdef HU_CAMERA_PREVIEW_AVAILABLE
    qWarning() << "[RearCamera]" << reason << "- falling back to placeholder";
    if (m_frameTimer) m_frameTimer->stop();
    stopCameraPreview();
#else
    Q_UNUSED(reason)
#endif
    m_showPlaceholder = true;
    update();
}

void ReverseCameraWindow::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)

    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setRenderHint(QPainter::SmoothPixmapTransform, true);

    // ---- Background: live camera or placeholder ----
    if (!m_showPlaceholder && !m_frame.isNull())
        p.drawImage(rect(), m_frame);
    else
        p.drawPixmap(rect(), m_placeholder);

    if (!m_pdcState.active)
        return;

    // ---- Distance from PdcState (-1 → unknown) ----
    const float rawCm = m_pdcState.nearestDistanceCm;
    const bool  hasDistance = (rawCm >= 0.0f);
    int distance = hasDistance ? static_cast<int>(rawCm) : 999;
    if (distance < 0)   distance = 0;
    if (distance > 999) distance = 999;

    const int W = width();
    const int H = height();
    const int gearPanelW = kGearPanelW;
    const int camX = gearPanelW;
    const int camW = W - gearPanelW;
    const int cx = camX + camW / 2;

    QLinearGradient panelGrad(0, 0, gearPanelW, H);
    panelGrad.setColorAt(0, QColor(8, 10, 12, 220));
    panelGrad.setColorAt(1, QColor(18, 20, 24, 235));
    p.fillRect(QRect(0, 0, gearPanelW, H), panelGrad);

    p.setPen(QPen(QColor(255, 255, 255, 20), 1));
    p.drawLine(gearPanelW, 0, gearPanelW, H);

    // ---- Distance color ----
    QColor dotColor;
    if (!hasDistance)
        dotColor = QColor(160, 160, 160);
    else if (distance > 40)
        dotColor = QColor(0, 255, 120);
    else if (distance > 20)
        dotColor = QColor(255, 210, 0);
    else
        dotColor = QColor(255, 0, 0);

    const QColor yellow(255, 230, 0);
    const QColor red(255, 0, 0);

    QFont font;
    if (hasDistance && distance < 100) {
        // ---- Distance box ----
        p.setBrush(QColor(0, 0, 0, 155));
        p.setPen(QPen(QColor(255, 255, 255, 80), 2));
        p.drawRoundedRect(cx - 115, 125, 230, 78, 16, 16);

        font.setPointSize(11);
        p.setFont(font);
        p.setPen(Qt::white);
        p.drawText(QRect(cx - 115, 136, 230, 18), Qt::AlignCenter, "DISTANCE");

        font.setPointSize(30);
        font.setBold(true);
        p.setFont(font);
        p.setPen(dotColor);
        p.drawText(QRect(cx - 115, 158, 230, 34),
                   Qt::AlignCenter,
                   QString::number(distance) + " cm");

        if (!m_pdcState.trackedSensorName.isEmpty()) {
            QString zone = m_pdcState.trackedSensorName;
            zone.replace(QStringLiteral("rear_"), QString());
            zone.replace(QStringLiteral("mid_"), QStringLiteral("M"));
            zone.replace(QLatin1Char('_'), QLatin1Char(' '));
            zone = zone.toUpper();

            QString confidenceText = QStringLiteral("%1  %2%")
                                         .arg(zone)
                                         .arg(m_pdcState.confidencePercent);
            if (m_pdcState.distanceHeld) {
                confidenceText += QStringLiteral("  HOLD");
            }

            font.setPointSize(8);
            font.setBold(false);
            p.setFont(font);
            p.setPen(Qt::NoPen);
            p.setBrush(QColor(0, 0, 0, 145));
            p.drawRoundedRect(cx - 82, 209, 164, 22, 7, 7);

            p.setPen(QColor(230, 255, 245, 220));
            p.drawText(QRect(cx - 78, 212, 156, 16),
                       Qt::AlignCenter,
                       confidenceText);

            const int barW = qBound(0, (m_pdcState.confidencePercent * 132) / 100, 132);
            p.setPen(QPen(QColor(255, 255, 255, 45), 2, Qt::SolidLine, Qt::RoundCap));
            p.drawLine(cx - 66, 228, cx + 66, 228);
            p.setPen(QPen(dotColor, 2, Qt::SolidLine, Qt::RoundCap));
            p.drawLine(cx - 66, 228, cx - 66 + barW, 228);
        }
    }

    // ---- Parking guide lines ----
    const int yBottom = 505;
    const int yMid = 365;
    const int yTop = 245;

    const int xBottomL = cx - 345;
    const int xBottomR = cx + 345;
    const int xMidL = cx - 227;
    const int xMidR = cx + 227;
    const int xTopL = cx - 135;
    const int xTopR = cx + 135;

    p.setPen(QPen(red, 8, Qt::SolidLine, Qt::RoundCap));
    p.drawLine(xBottomL, yBottom, xBottomR, yBottom);

    p.setPen(QPen(dotColor, 7, Qt::SolidLine, Qt::RoundCap));
    p.drawLine(xMidL, yMid, xMidR, yMid);
    p.drawLine(xTopL, yTop, xTopR, yTop);

    p.setPen(QPen(yellow, 7, Qt::SolidLine, Qt::RoundCap));
    p.drawLine(xBottomL, yBottom, xTopL, yTop);
    p.drawLine(xBottomR, yBottom, xTopR, yTop);

    // ---- 3 side sensor dots each side ----
    int activeDots = 1;
    if (distance > 100) activeDots = 1;
    else if (distance > 60) activeDots = 2;
    else activeDots = 3;

    for (int i = 0; i < 3; ++i) {
        const QColor sensorColor = (i < activeDots) ? dotColor : QColor(90, 90, 90, 130);
        p.setBrush(sensorColor);
        p.setPen(Qt::NoPen);

        const int y = 255 + i * 55;

        p.drawEllipse(camX + 18, y, 18, 18);
        p.drawEllipse(camX + camW - 36, y, 18, 18);
    }

    // ---- Footer ----
    font.setPointSize(10);
    font.setBold(false);
    p.setFont(font);
    p.setPen(Qt::white);
    p.drawText(QRect(camX, 572, camW, 20),
               Qt::AlignCenter,
               "Check surroundings before reversing");
}

void ReverseCameraWindow::buildPlaceholderPixmap()
{
    m_placeholder = QPixmap(kScreenW, kScreenH);

    QPainter p(&m_placeholder);
    p.setRenderHint(QPainter::Antialiasing);
    p.setRenderHint(QPainter::SmoothPixmapTransform, true);

    QLinearGradient grad(0, 0, kScreenW, kScreenH);
    grad.setColorAt(0,   QColor(15, 18, 22));
    grad.setColorAt(0.5, QColor(25, 30, 35));
    grad.setColorAt(1,   QColor(12, 15, 18));
    p.fillRect(m_placeholder.rect(), grad);

    QLinearGradient panelGrad(0, 0, kGearPanelW, kScreenH);
    panelGrad.setColorAt(0, QColor(8, 10, 12, 220));
    panelGrad.setColorAt(1, QColor(18, 20, 24, 235));
    p.fillRect(QRect(0, 0, kGearPanelW, kScreenH), panelGrad);
    p.setPen(QPen(QColor(255, 255, 255, 20), 1));
    p.drawLine(kGearPanelW, 0, kGearPanelW, kScreenH);

    p.setPen(QPen(QColor(0, 180, 120), 1.5));
    const int cx = kGearPanelW + kCameraAreaW / 2;
    const int cy = kScreenH / 2;
    const int gw = 300;
    const int gh = 220;
    for (int i = -2; i <= 2; ++i) {
        int x = cx + i * gw / 2;
        p.drawLine(x, 120, x, 520);
    }
    for (int i = -2; i <= 2; ++i) {
        int y = cy + i * gh / 2;
        p.drawLine(kGearPanelW + 40, y, kScreenW - 40, y);
    }

    p.setPen(QPen(QColor(0, 212, 170), 2));
    p.setBrush(Qt::NoBrush);
    p.drawRect(cx - 36, cy - 28, 72, 56);
    p.drawLine(cx - 48, cy, cx + 48, cy);
    p.drawLine(cx, cy - 40, cx, cy + 40);

    QFont font;
    font.setPointSize(18);
    font.setBold(true);
    p.setFont(font);
    p.setPen(QColor(0, 212, 170));
    p.drawText(QRect(0, 20, kScreenW, 40), Qt::AlignCenter, "REAR VIEW");

    font.setPointSize(10);
    font.setBold(false);
    p.setFont(font);
    p.setPen(QColor(100, 110, 120));
    p.drawText(QRect(0, 58, kScreenW, 24), Qt::AlignCenter, "Placeholder - No camera connected");

    p.setPen(Qt::NoPen);
    p.setBrush(QColor(40, 45, 50));
    p.drawRect(0, 500, kScreenW, 100);
    p.end();
}
