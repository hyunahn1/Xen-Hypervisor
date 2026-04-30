/**
 * @file GearPanel.h
 * @brief Left-side gear panel: P button (top), drag-car (middle), hold-N (bottom)
 * @author Ahn Hyunjun
 * @date 2026-02-23
 */

#ifndef GEARPANEL_H
#define GEARPANEL_H

#include <QWidget>
#include <QPushButton>
#include <QTimer>

class GearStateManager;
class DragCarWidget;

class GearPanel : public QWidget
{
    Q_OBJECT

public:
    explicit GearPanel(GearStateManager *gearState, QWidget *parent = nullptr);

protected:
    void paintEvent(QPaintEvent *event) override;

private slots:
    void updateDisplay();

private:
    void setupUI();

    GearStateManager *m_gearState;
    DragCarWidget    *m_dragCar;
    QPushButton      *m_btnP;
    QPushButton      *m_btnN;
    QTimer           *m_holdTimer;
};

#endif // GEARPANEL_H
