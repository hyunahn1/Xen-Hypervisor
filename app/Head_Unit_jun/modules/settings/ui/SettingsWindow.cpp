/**
 * @file SettingsScreen.cpp
 * @brief Settings tab - speed unit toggle, about info
 * @author Ahn Hyunjun
 * @date 2026-02-20
 */

#include "SettingsWindow.h"
#include "GearPanel.h"
#include "GearStateManager.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFrame>
#include <QLabel>
#include <QPushButton>

// ─────────────────────────────────────────────────────────────
static const char *kScreenStyle = R"(
    QWidget { background-color: #0D0D0F; color: #FFFFFF; }
    QLabel  { background: transparent; }
)";

static const char *kRowStyle =
    "background-color: #1A1A1E; border-radius: 12px;";

static QLabel *sectionLabel(const QString &text, QWidget *parent)
{
    QLabel *l = new QLabel(text, parent);
    l->setStyleSheet("font-size: 11pt; color: #6E6E73; margin-top: 6px;");
    return l;
}

static QFrame *rowCard(QWidget *parent)
{
    QFrame *f = new QFrame(parent);
    f->setStyleSheet(kRowStyle);
    return f;
}

// ─────────────────────────────────────────────────────────────
SettingsScreen::SettingsScreen(GearStateManager *gearState, QWidget *parent)
    : QWidget(parent)
    , m_gearPanel(nullptr)
{
    setStyleSheet(kScreenStyle);

    QHBoxLayout *root = new QHBoxLayout(this);
    root->setContentsMargins(0, 8, 12, 8);
    root->setSpacing(20);  // Gap between gear panel green line and main content

    QVBoxLayout *content = new QVBoxLayout();
    content->setSpacing(10);

    // ── Heading ──────────────────────────────────────────────
    QLabel *heading = new QLabel("Settings", this);
    heading->setStyleSheet("font-size: 16pt; font-weight: bold; color: #FFFFFF;");
    content->addWidget(heading);

    // ── Speed Unit ───────────────────────────────────────────
    content->addWidget(sectionLabel("Display", this));

    QFrame *unitCard = rowCard(this);
    unitCard->setFixedHeight(50);
    QHBoxLayout *unitLayout = new QHBoxLayout(unitCard);
    unitLayout->setContentsMargins(14, 0, 14, 0);
    QLabel *unitText = new QLabel("Speed Unit", unitCard);
    unitText->setStyleSheet("font-size: 11pt; color: #FFFFFF;");

    QPushButton *kmhBtn = new QPushButton("km/h", unitCard);
    QPushButton *mphBtn = new QPushButton("mph",  unitCard);
    for (auto *btn : {kmhBtn, mphBtn}) {
        btn->setFixedSize(58, 28);
        btn->setCheckable(true);
    }
    const auto updateUnitStyle = [kmhBtn, mphBtn]() {
        const char *activeStyle = R"(
            QPushButton {
                background-color: #00D4AA; color: #000000;
                border: none; border-radius: 6px; font-size: 10pt; font-weight: bold;
            }
        )";
        const char *inactiveStyle = R"(
            QPushButton {
                background-color: #252529; color: #B3B3B7;
                border: none; border-radius: 6px; font-size: 10pt;
            }
            QPushButton:hover { background-color: #3A3A3F; color: #FFFFFF; }
        )";
        kmhBtn->setStyleSheet(kmhBtn->isChecked() ? activeStyle : inactiveStyle);
        mphBtn->setStyleSheet(mphBtn->isChecked() ? activeStyle : inactiveStyle);
    };

    kmhBtn->setChecked(true);
    updateUnitStyle();

    connect(kmhBtn, &QPushButton::clicked, this, [=] {
        kmhBtn->setChecked(true);
        mphBtn->setChecked(false);
        updateUnitStyle();
        m_useMetric = true;
        emit speedUnitChanged(true);
    });
    connect(mphBtn, &QPushButton::clicked, this, [=] {
        mphBtn->setChecked(true);
        kmhBtn->setChecked(false);
        updateUnitStyle();
        m_useMetric = false;
        emit speedUnitChanged(false);
    });

    QHBoxLayout *toggleLayout = new QHBoxLayout();
    toggleLayout->setSpacing(4);
    toggleLayout->addWidget(kmhBtn);
    toggleLayout->addWidget(mphBtn);
    unitLayout->addWidget(unitText);
    unitLayout->addStretch();
    unitLayout->addLayout(toggleLayout);
    content->addWidget(unitCard);

    // ── About ────────────────────────────────────────────────
    content->addWidget(sectionLabel("About", this));

    struct InfoRow { QString key; QString value; };
    const QList<InfoRow> infoRows = {
        { "Application",  "PiRacer Head Unit" },
        { "Version",      "1.0.0" },
        { "Build Date",   "2026-02-20" },
        { "IPC",          "VSOMEIP (peer-to-peer)" },
        { "Platform",     "Raspberry Pi 4 / Yocto Linux" },
    };

    QFrame *aboutCard = rowCard(this);
    QVBoxLayout *aboutLayout = new QVBoxLayout(aboutCard);
    aboutLayout->setContentsMargins(14, 10, 14, 10);
    aboutLayout->setSpacing(6);

    for (int i = 0; i < infoRows.size(); ++i) {
        const auto &row = infoRows[i];
        QHBoxLayout *rowLayout = new QHBoxLayout();
        QLabel *key = new QLabel(row.key, aboutCard);
        key->setStyleSheet("font-size: 10pt; color: #6E6E73;");
        key->setFixedWidth(130);
        QLabel *val = new QLabel(row.value, aboutCard);
        val->setStyleSheet("font-size: 11pt; color: #FFFFFF;");
        rowLayout->addWidget(key);
        rowLayout->addWidget(val);
        rowLayout->addStretch();
        aboutLayout->addLayout(rowLayout);

        if (i < infoRows.size() - 1) {
            QFrame *divider = new QFrame(aboutCard);
            divider->setFrameShape(QFrame::HLine);
            divider->setStyleSheet("color: #252529;");
            aboutLayout->addWidget(divider);
        }
    }
    content->addWidget(aboutCard);

    // ── Vehicle Status ────────────────────────────────────────
    content->addWidget(sectionLabel("Vehicle Status", this));

    QFrame *statusCard = rowCard(this);
    QVBoxLayout *statusLayout = new QVBoxLayout(statusCard);
    statusLayout->setContentsMargins(14, 10, 14, 10);
    statusLayout->setSpacing(6);

    auto makeStatusRow = [&](const QString &label, QLabel *&valueOut) {
        QHBoxLayout *row = new QHBoxLayout();
        QLabel *key = new QLabel(label, statusCard);
        key->setStyleSheet("font-size: 10pt; color: #6E6E73;");
        key->setFixedWidth(130);
        valueOut = new QLabel("—", statusCard);
        valueOut->setStyleSheet("font-size: 11pt; color: #FFFFFF;");
        row->addWidget(key);
        row->addWidget(valueOut);
        row->addStretch();
        statusLayout->addLayout(row);
    };

    makeStatusRow("IC IPC",       m_ipcStatusLabel);
    statusLayout->addWidget([statusCard]() {
        QFrame *d = new QFrame(statusCard);
        d->setFrameShape(QFrame::HLine);
        d->setStyleSheet("color: #252529;");
        return d;
    }());
    makeStatusRow("Speed",        m_speedLabel);
    statusLayout->addWidget([statusCard]() {
        QFrame *d = new QFrame(statusCard);
        d->setFrameShape(QFrame::HLine);
        d->setStyleSheet("color: #252529;");
        return d;
    }());
    makeStatusRow("Gear",         m_gearLabel);

    content->addWidget(statusCard);
    content->addStretch();

    m_gearPanel = new GearPanel(gearState, this);
    root->addWidget(m_gearPanel);
    root->addLayout(content, 1);
}

// ── 상태 업데이트 슬롯 ─────────────────────────────────────────────

void SettingsScreen::updateIpcStatus(bool connected)
{
    if (!m_ipcStatusLabel) return;
    if (connected) {
        m_ipcStatusLabel->setText("Connected");
        m_ipcStatusLabel->setStyleSheet("font-size: 11pt; color: #00D4AA;");
    } else {
        m_ipcStatusLabel->setText("Disconnected");
        m_ipcStatusLabel->setStyleSheet("font-size: 11pt; color: #FF453A;");
    }
}

void SettingsScreen::updateSpeed(float kmh)
{
    if (!m_speedLabel) return;
    if (m_useMetric)
        m_speedLabel->setText(QString::number(qRound(kmh)) + " km/h");
    else
        m_speedLabel->setText(QString::number(qRound(kmh * 0.621371f)) + " mph");
}

void SettingsScreen::updateGear(GearState gear)
{
    if (!m_gearLabel) return;
    static const char *kGearNames[] = { "P", "R", "N", "D" };
    const int idx = static_cast<int>(gear);
    m_gearLabel->setText((idx >= 0 && idx < 4) ? kGearNames[idx] : "?");
}
