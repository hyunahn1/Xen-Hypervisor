/**
 * @file NavigationScreen.cpp
 * @brief Navigation tab - OpenStreetMap WebView
 */

#include "NavigationWindow.h"
#include "GearPanel.h"
#include "GearStateManager.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>

#ifdef HU_WEBENGINE_AVAILABLE
#include <QWebEngineView>
#include <QWebEngineSettings>
#endif

static const char *kScreenStyle = R"(
    QWidget { background-color: #0D0D0F; color: #FFFFFF; }
    QLabel  { background: transparent; }
)";

NavigationScreen::NavigationScreen(GearStateManager *gearState, QWidget *parent)
    : QWidget(parent)
    , m_gearPanel(nullptr)
#ifdef HU_WEBENGINE_AVAILABLE
    , m_webView(nullptr)
#endif
{
    setStyleSheet(kScreenStyle);
    setupUI(gearState);
}

void NavigationScreen::setupUI(GearStateManager *gearState)
{
    QHBoxLayout *root = new QHBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    m_gearPanel = new GearPanel(gearState, this);
    m_gearPanel->setFixedWidth(m_gearPanel->sizeHint().width());

    QVBoxLayout *content = new QVBoxLayout();
    content->setContentsMargins(0, 0, 0, 0);
    content->setSpacing(0);

#ifdef HU_WEBENGINE_AVAILABLE
    m_webView = new QWebEngineView(this);
    m_webView->settings()->setAttribute(QWebEngineSettings::JavascriptEnabled, true);
    // OpenStreetMap centered on Seoul
    m_webView->load(QUrl("https://www.openstreetmap.org/#map=13/37.5665/126.9780"));
    content->addWidget(m_webView, 1);
#else
    QLabel *placeholder = new QLabel("지도 기능은 준비 중입니다.", this);
    placeholder->setAlignment(Qt::AlignCenter);
    placeholder->setStyleSheet("font-size: 13pt; color: #6E6E73;");
    content->addWidget(placeholder, 1);
#endif

    root->addWidget(m_gearPanel);
    root->addLayout(content, 1);
}
