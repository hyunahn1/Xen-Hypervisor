/**
 * @file TabBar.cpp
 * @brief 4-tab bar implementation
 * @author Ahn Hyunjun
 * @date 2026-02-20
 */

#include "TabBar.h"
#include <QSignalMapper>
#include <QVariant>

TabBar::TabBar(QWidget *parent)
    : QWidget(parent)
    , m_currentIndex(0)
{
    setupUI();
}

void TabBar::setupUI()
{
    m_layout = new QHBoxLayout(this);
    m_layout->setContentsMargins(12, 0, 12, 0);
    m_layout->setSpacing(0);

    // Icons: Media ♪ | YouTube ▶ | Call ☎ | Navigation ⊕ | Lighting ★ | Settings ⚙
    static const char *icons[] = { "♪", "▶", "☎", "⊕", "★", "⚙" };
    static const char *tooltips[] = { "Media", "YouTube", "Call", "Navigation", "Lighting", "Settings" };

    m_layout->addStretch(1);
    for (int i = 0; i < 6; ++i) {
        QPushButton *btn = new QPushButton(QString::fromUtf8(icons[i]), this);
        btn->setFixedSize(40, 36);
        btn->setToolTip(QString::fromUtf8(tooltips[i]));
        btn->setProperty("tabIndex", i);
        btn->setCursor(Qt::PointingHandCursor);
        btn->setStyleSheet("font-size: 16pt;");

        connect(btn, &QPushButton::clicked, this, [this, i]() {
            setCurrentIndex(i);
            emit tabSelected(i);
        });

        m_tabs.append(btn);
        m_layout->addWidget(btn);
        if (i < 5) m_layout->addSpacing(6);
    }
    m_layout->addStretch(1);
    updateTabStyles();
}

void TabBar::setCurrentIndex(int index)
{
    if (index < 0 || index >= m_tabs.size()) return;
    m_currentIndex = index;
    updateTabStyles();
}

void TabBar::onTabClicked()
{
    QPushButton *btn = qobject_cast<QPushButton*>(sender());
    if (btn && btn->property("tabIndex").isValid()) {
        int idx = btn->property("tabIndex").toInt();
        setCurrentIndex(idx);
        emit tabSelected(idx);
    }
}

void TabBar::updateTabStyles()
{
    QString selectedStyle =
        "QPushButton {"
        "  color: #00D4AA;"
        "  background-color: #00D4AA18;"
        "  border: none;"
        "  border-radius: 10px;"
        "  font-size: 16pt;"
        "}"
        "QPushButton:hover { background-color: #00D4AA28; }";
    QString normalStyle =
        "QPushButton {"
        "  color: #6E6E73;"
        "  background: transparent;"
        "  border: none;"
        "  border-radius: 10px;"
        "  font-size: 16pt;"
        "}"
        "QPushButton:hover { background-color: #252529; color: #B3B3B7; }";

    for (int i = 0; i < m_tabs.size(); ++i) {
        m_tabs[i]->setStyleSheet(i == m_currentIndex ? selectedStyle : normalStyle);
    }
}
