/**
 * @file GearPanel.cpp
 * @brief Left-side gear panel
 *   Top    : [P] tap button
 *   Middle : drag-car widget — drag up = Drive, drag down = Reverse
 *   Bottom : [N] press-and-hold button (600 ms)
 * @author Ahn Hyunjun
 * @date 2026-02-23
 */

#include "GearPanel.h"
#include "GearStateManager.h"
#include "IVehicleDataProvider.h"

#include <QVBoxLayout>
#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include <QTimer>
#include <QLinearGradient>

// P=0 R=1 N=2 D=3
static constexpr const char *kGearColor[] = {
    "#8E8E93", "#FF3B30", "#FFD60A", "#30D158",
};
static constexpr const char *kGearDim[] = {
    "#2C2C2E", "#3D1210", "#3D3100", "#0F3D1B",
};

// =============================================================================
//  DragCarWidget — drag up = Drive, drag down = Reverse
// =============================================================================
class DragCarWidget : public QWidget
{
    Q_OBJECT
public:
    explicit DragCarWidget(GearStateManager *gs, QWidget *parent = nullptr)
        : QWidget(parent), m_gs(gs)
    {
        setFixedSize(72, 150);
        setCursor(Qt::OpenHandCursor);
        setAttribute(Qt::WA_OpaquePaintEvent, false);

        m_snapTimer = new QTimer(this);
        m_snapTimer->setInterval(16);
        connect(m_snapTimer, &QTimer::timeout, this, [this] {
            m_offset *= 0.72f;
            if (qAbs(m_offset) < 1.0f) { m_offset = 0.0f; m_snapTimer->stop(); }
            update();
        });
    }

    void refreshGear() { update(); }

protected:
    void mousePressEvent(QMouseEvent *e) override
    {
        m_pressY = e->pos().y();
        m_offset = 0.0f;
        m_dragging = true;
        m_snapTimer->stop();
        setCursor(Qt::ClosedHandCursor);
        update();
    }

    void mouseMoveEvent(QMouseEvent *e) override
    {
        if (!m_dragging) return;
        m_offset = qBound(-50.0f, float(e->pos().y() - m_pressY), 50.0f);
        update();
    }

    void mouseReleaseEvent(QMouseEvent *e) override
    {
        if (!m_dragging) return;
        m_dragging = false;
        setCursor(Qt::OpenHandCursor);
        const int delta = e->pos().y() - m_pressY;
        if      (delta < -THRESHOLD) m_gs->setGearFromTouch(GearState::D);
        else if (delta >  THRESHOLD) m_gs->setGearFromTouch(GearState::R);
        m_snapTimer->start();
    }

    void paintEvent(QPaintEvent *) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        p.setRenderHint(QPainter::SmoothPixmapTransform);

        const GearState g     = m_gs->gear();
        const QColor    color(kGearColor[static_cast<int>(g)]);
        const int cx = width() / 2;
        const int cy = 75 + static_cast<int>(m_offset);  // Fixed center

        // ── Zone 1: DRIVE ──
        {
            const bool active = (m_dragging && m_offset < -12) || g == GearState::D;
            QColor c(kGearColor[3]);
            c.setAlpha(active ? 230 : 45);
            drawChevron(p, cx, 18, true, c);
            QFont f; f.setPointSize(6); f.setBold(true); p.setFont(f);
            p.setPen(c);
            p.drawText(QRect(cx - 14, 2, 28, 8), Qt::AlignCenter, "D");
        }

        // ── Zone 2: Car ──
        drawCar(p, cx, cy, color);

        // ── Zone 3: REV ──
        {
            const bool active = (m_dragging && m_offset > 12) || g == GearState::R;
            QColor c(kGearColor[1]);
            c.setAlpha(active ? 230 : 45);
            drawChevron(p, cx, height() - 18, false, c);
            QFont f; f.setPointSize(6); f.setBold(true); p.setFont(f);
            p.setPen(c);
            p.drawText(QRect(cx - 14, height() - 16, 28, 8), Qt::AlignCenter, "R");
        }
    }

private:
    void drawChevron(QPainter &p, int cx, int y, bool up, const QColor &c)
    {
        const int dir = up ? -1 : 1;
        const int aw  = 8;   // Smaller, stays in bounds
        p.setPen(QPen(c, 1.8, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        p.drawLine(cx - aw, y + dir * 10, cx, y + dir * 4);
        p.drawLine(cx, y + dir * 4, cx + aw, y + dir * 10);
    }

    void drawCar(QPainter &p, int cx, int cy, const QColor &color)
    {
        p.save();
        p.translate(cx, cy);
        p.scale(0.82f, 0.82f);  // Fit smaller widget

        // Modern EV-inspired silhouette — single fluid shape
        QPainterPath body;
        body.moveTo(-22, 6);
        body.quadTo(-24, -2, -20, -8);
        body.lineTo(-10, -12);
        body.quadTo(0, -14, 10, -12);
        body.lineTo(20, -8);
        body.quadTo(24, -2, 22, 6);
        body.closeSubpath();

        QLinearGradient grad(0, -14, 0, 8);
        grad.setColorAt(0, color.lighter(160));
        grad.setColorAt(0.4, color);
        grad.setColorAt(1, color.darker(190));
        p.setBrush(grad);
        p.setPen(Qt::NoPen);
        p.drawPath(body);

        // Cabin — minimal glass roof
        QPainterPath cabin;
        cabin.moveTo(-10, -10);
        cabin.quadTo(0, -16, 10, -10);
        cabin.lineTo(6, -6);
        cabin.lineTo(-6, -6);
        cabin.closeSubpath();

        p.setBrush(QColor(140, 200, 255, 90));
        p.drawPath(cabin);

        // Wheels — minimal circles
        for (int s : {-1, 1}) {
            QPointF w(s * 18.0, 8.0);
            p.setBrush(QColor("#252528"));
            p.drawEllipse(w, 5.0, 5.0);
            p.setBrush(QColor("#404048"));
            p.drawEllipse(w, 3.0, 3.0);
        }

        // Accent line
        p.setPen(QPen(color.lighter(200), 0.8));
        p.setBrush(Qt::NoBrush);
        p.drawPath(body);

        p.restore();
    }

    GearStateManager *m_gs;
    float             m_offset   = 0.0f;
    int               m_pressY   = 0;
    bool              m_dragging = false;
    QTimer           *m_snapTimer = nullptr;
    static constexpr int THRESHOLD = 35;
};

// =============================================================================
//  GearPanel
// =============================================================================
GearPanel::GearPanel(GearStateManager *gearState, QWidget *parent)
    : QWidget(parent)
    , m_gearState(gearState)
    , m_dragCar(nullptr)
    , m_btnP(nullptr)
    , m_btnN(nullptr)
    , m_holdTimer(nullptr)
{
    setFixedWidth(96);
    setAttribute(Qt::WA_StyledBackground, true);
    setupUI();
    connect(m_gearState, &GearStateManager::gearChanged,
            this, &GearPanel::updateDisplay);
}

void GearPanel::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(0x1C, 0x1C, 0x1E));
    p.drawRect(rect());
    // Teal right border — drawn with clear gap from content
    p.setBrush(QColor(0x00, 0xD4, 0xAA));
    p.drawRect(width() - 2, 0, 2, height());
}

void GearPanel::setupUI()
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(6, 12, 18, 12);  // Extra right margin = gap before green line
    layout->setSpacing(0);

    // ── P button ────────────────────────────────────────────────────
    m_btnP = new QPushButton("P", this);
    m_btnP->setFixedSize(56, 42);
    connect(m_btnP, &QPushButton::clicked, this,
            [this] { m_gearState->setGearFromTouch(GearState::P); });
    layout->addWidget(m_btnP, 0, Qt::AlignCenter);
    layout->addSpacing(12);

    auto makeSep = [this]() {
        auto *s = new QWidget(this);
        s->setFixedHeight(1);
        s->setStyleSheet("background-color: #2C2C2E;");
        return s;
    };

    layout->addWidget(makeSep());
    layout->addStretch(1);

    // ── Drag car ─────────────────────────────────────────────────────
    m_dragCar = new DragCarWidget(m_gearState, this);
    layout->addWidget(m_dragCar, 0, Qt::AlignCenter);
    layout->addStretch(1);
    layout->addWidget(makeSep());
    layout->addSpacing(12);

    // ── N hold button ────────────────────────────────────────────────
    m_btnN = new QPushButton(this);
    m_btnN->setFixedSize(56, 42);
    m_btnN->setToolTip("Press and hold for Neutral");

    m_holdTimer = new QTimer(this);
    m_holdTimer->setSingleShot(true);
    m_holdTimer->setInterval(600);

    connect(m_btnN, &QPushButton::pressed,  this, [this] { m_holdTimer->start(); });
    connect(m_btnN, &QPushButton::released, this, [this] { m_holdTimer->stop();  });
    connect(m_holdTimer, &QTimer::timeout,  this, [this] {
        m_gearState->setGearFromTouch(GearState::N);
    });

    layout->addWidget(m_btnN, 0, Qt::AlignCenter);
    updateDisplay();
}

void GearPanel::updateDisplay()
{
    if (m_dragCar) m_dragCar->refreshGear();

    const GearState g    = m_gearState->gear();
    const bool      selP = (g == GearState::P);
    const bool      selN = (g == GearState::N);

    // P button
    if (m_btnP) {
        m_btnP->setStyleSheet(selP ?
            "QPushButton {"
            "  background-color:#48484A; color:#FFFFFF;"
            "  font-size:16pt; font-weight:900;"
            "  border-radius:10px; border:none;"
            "}" :
            "QPushButton {"
            "  background-color:#2C2C2E; color:rgba(255,255,255,0.28);"
            "  font-size:16pt; font-weight:700;"
            "  border-radius:10px; border:none;"
            "}"
            "QPushButton:hover{"
            "  background-color:#3A3A3F; color:rgba(255,255,255,0.70);"
            "}"
        );
    }

    // N hold button
    if (m_btnN) {
        m_btnN->setText(selN ? "N" : "N\nhold");
        m_btnN->setStyleSheet(selN ?
            "QPushButton {"
            "  background-color:#3D3000; color:#FFD60A;"
            "  font-size:13pt; font-weight:900;"
            "  border-radius:10px; border:2px solid #FFD60A55;"
            "}" :
            "QPushButton {"
            "  background-color:#1C1C1E; color:rgba(255,214,10,0.32);"
            "  font-size:9pt; font-weight:700;"
            "  border-radius:10px; border:1px solid #2C2C2E;"
            "}"
            "QPushButton:hover{"
            "  background-color:#2A2A00; color:rgba(255,214,10,0.70);"
            "  border-color:#FFD60A44;"
            "}"
            "QPushButton:pressed{"
            "  background-color:#3D3000; color:#FFD60A;"
            "}"
        );
    }
}

#include "GearPanel.moc"
