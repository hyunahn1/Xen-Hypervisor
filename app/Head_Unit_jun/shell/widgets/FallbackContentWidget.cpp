/**
 * @file FallbackContentWidget.cpp
 */

#include "FallbackContentWidget.h"
#include "GearPanel.h"
#include "GearStateManager.h"

#include <QHBoxLayout>
#include <QLabel>

FallbackContentWidget::FallbackContentWidget(GearStateManager *gearState,
                                             const QString &message,
                                             QWidget *parent)
    : QWidget(parent)
{
    setStyleSheet("background-color: #0D0D0F;");
    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    auto *panel = new GearPanel(gearState, this);
    layout->addWidget(panel);

    auto *msgLabel = new QLabel(message, this);
    msgLabel->setAlignment(Qt::AlignCenter);
    msgLabel->setStyleSheet(
        "color: #6E6E73; font-size: 14pt; background: transparent;"
    );
    layout->addWidget(msgLabel, 1);
}
