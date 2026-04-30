/**
 * @file SplashScreen.h
 * @brief Car-style boot splash — logo fade-in, progress bar, fade-out
 * @author Ahn Hyunjun
 * @date 2026-02-20
 */

#ifndef SPLASHSCREEN_H
#define SPLASHSCREEN_H

#include <QWidget>

class QVariantAnimation;

/**
 * @class SplashScreen
 * @brief Full-window overlay shown at startup.
 *        Phase 0: PiRacer logo fades in (700 ms)
 *        Phase 1: Loading progress bar fills  (1600 ms)
 *        Phase 2: Everything fades to transparent (550 ms)
 *        Then emits finished() → caller deletes it.
 */
class SplashScreen : public QWidget
{
    Q_OBJECT

public:
    explicit SplashScreen(QWidget *parent = nullptr);

signals:
    void finished();

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    void runPhase(int phase);

    int   m_phase = 0;
    float m_t     = 0.0f;   // 0→1 within the current phase
};

#endif // SPLASHSCREEN_H
