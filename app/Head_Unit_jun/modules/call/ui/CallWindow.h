/**
 * @file CallScreen.h
 * @brief Call tab - phone call interface
 */

#ifndef CALLWINDOW_H
#define CALLWINDOW_H

#include <QWidget>

class GearStateManager;
class GearPanel;

class CallScreen : public QWidget
{
    Q_OBJECT

public:
    explicit CallScreen(GearStateManager *gearState, QWidget *parent = nullptr);

private slots:
    void onCallClicked();
    void onEndCallClicked();

private:
    void setupUI(GearStateManager *gearState);

    GearPanel *m_gearPanel;
    bool m_inCall = false;
};

#endif // CALLWINDOW_H
