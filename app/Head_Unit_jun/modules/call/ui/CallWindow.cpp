/**
 * @file CallScreen.cpp
 * @brief Call tab - phone call interface
 */

#include "CallWindow.h"
#include "GearPanel.h"
#include "GearStateManager.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFrame>
#include <QLabel>
#include <QPushButton>

static const char *kScreenStyle = R"(
    QWidget { background-color: #0D0D0F; color: #FFFFFF; }
    QLabel  { background: transparent; }
)";

static const char *kCardStyle = "background-color: #1A1A1E; border-radius: 16px;";
static const char *kCallBtnStyle = R"(
    QPushButton {
        background-color: #00D4AA; color: #000000;
        border: none; border-radius: 30px; font-size: 14pt; font-weight: bold;
    }
    QPushButton:hover { background-color: #00EFBF; }
    QPushButton:pressed { background-color: #00A887; }
)";
static const char *kEndBtnStyle = R"(
    QPushButton {
        background-color: #E53935; color: #FFFFFF;
        border: none; border-radius: 30px; font-size: 14pt; font-weight: bold;
    }
    QPushButton:hover { background-color: #EF5350; }
    QPushButton:pressed { background-color: #C62828; }
)";

CallScreen::CallScreen(GearStateManager *gearState, QWidget *parent)
    : QWidget(parent)
    , m_gearPanel(nullptr)
{
    setStyleSheet(kScreenStyle);
    setupUI(gearState);
}

void CallScreen::setupUI(GearStateManager *gearState)
{
    QHBoxLayout *root = new QHBoxLayout(this);
    root->setContentsMargins(0, 8, 12, 8);
    root->setSpacing(20);

    QVBoxLayout *content = new QVBoxLayout();
    content->setSpacing(16);

    QLabel *heading = new QLabel("Call", this);
    heading->setStyleSheet("font-size: 16pt; font-weight: bold; color: #FFFFFF;");
    content->addWidget(heading);

    QFrame *callCard = new QFrame(this);
    callCard->setStyleSheet(kCardStyle);
    callCard->setMinimumHeight(200);
    QVBoxLayout *callLayout = new QVBoxLayout(callCard);
    callLayout->setContentsMargins(24, 24, 24, 24);
    callLayout->setSpacing(16);

    m_gearPanel = new GearPanel(gearState, this);

    QLabel *contactLabel = new QLabel("Contact", callCard);
    contactLabel->setStyleSheet("font-size: 11pt; color: #6E6E73;");
    callLayout->addWidget(contactLabel);

    QLabel *nameLabel = new QLabel("+1 234 567 8900", callCard);
    nameLabel->setObjectName("contactName");
    nameLabel->setStyleSheet("font-size: 18pt; font-weight: bold; color: #FFFFFF;");
    callLayout->addWidget(nameLabel);

    QHBoxLayout *btnLayout = new QHBoxLayout();
    btnLayout->setSpacing(20);

    QPushButton *callBtn = new QPushButton("📞 Call", this);
    callBtn->setFixedSize(120, 60);
    callBtn->setStyleSheet(kCallBtnStyle);
    callBtn->setCursor(Qt::PointingHandCursor);
    connect(callBtn, &QPushButton::clicked, this, &CallScreen::onCallClicked);

    QPushButton *endBtn = new QPushButton("✕ End", this);
    endBtn->setFixedSize(120, 60);
    endBtn->setStyleSheet(kEndBtnStyle);
    endBtn->setCursor(Qt::PointingHandCursor);
    connect(endBtn, &QPushButton::clicked, this, &CallScreen::onEndCallClicked);

    btnLayout->addStretch();
    btnLayout->addWidget(callBtn);
    btnLayout->addWidget(endBtn);
    btnLayout->addStretch();

    callLayout->addStretch();
    callLayout->addLayout(btnLayout);

    content->addWidget(callCard);
    content->addStretch();

    root->addWidget(m_gearPanel);
    root->addLayout(content, 1);
}

void CallScreen::onCallClicked()
{
    m_inCall = true;
}

void CallScreen::onEndCallClicked()
{
    m_inCall = false;
}
