#ifndef PDCBEEPCONTROLLER_H
#define PDCBEEPCONTROLLER_H

#include "PdcTypes.h"

#include <QObject>
#include <QTimer>

class PdcBeepController : public QObject
{
    Q_OBJECT

public:
    explicit PdcBeepController(QObject *parent = nullptr);

public slots:
    void setPdcState(const PdcState &state);

private slots:
    void beep();

private:
    int intervalForLevel(PdcWarningLevel level) const;

    QTimer m_timer;
};

#endif // PDCBEEPCONTROLLER_H
