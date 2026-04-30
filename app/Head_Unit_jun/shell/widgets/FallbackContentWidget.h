/**
 * @file FallbackContentWidget.h
 * @brief Shown when module fails to embed — GearPanel + loading message
 */

#ifndef FALLBACKCONTENTWIDGET_H
#define FALLBACKCONTENTWIDGET_H

#include <QWidget>

class GearStateManager;

class FallbackContentWidget : public QWidget
{
    Q_OBJECT
public:
    explicit FallbackContentWidget(GearStateManager *gearState,
                                   const QString &message,
                                   QWidget *parent = nullptr);
};

#endif // FALLBACKCONTENTWIDGET_H
