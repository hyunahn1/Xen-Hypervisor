/**
 * @file NavigationScreen.h
 * @brief Navigation tab - OpenStreetMap WebView
 */

#ifndef NAVIGATIONWINDOW_H
#define NAVIGATIONWINDOW_H

#include <QWidget>

class GearStateManager;
class GearPanel;

#ifdef HU_WEBENGINE_AVAILABLE
class QWebEngineView;
#endif

class NavigationScreen : public QWidget
{
    Q_OBJECT

public:
    explicit NavigationScreen(GearStateManager *gearState, QWidget *parent = nullptr);

private:
    void setupUI(GearStateManager *gearState);

    GearPanel *m_gearPanel;
#ifdef HU_WEBENGINE_AVAILABLE
    QWebEngineView *m_webView;
#endif
};

#endif // NAVIGATIONWINDOW_H
