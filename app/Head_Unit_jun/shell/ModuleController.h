/**
 * @file ModuleController.h
 * @brief 모듈 프로세스 실행 / 감시 / 자동 재시작
 */

#ifndef MODULECONTROLLER_H
#define MODULECONTROLLER_H

#include <QObject>
#include <QProcess>
#include <QTimer>

class ModuleController : public QObject
{
    Q_OBJECT

public:
    explicit ModuleController(const QString &moduleName,
                              const QString &executableName,
                              const QString &socketPath,
                              QObject *parent = nullptr);

    void launch();
    void terminate();
    bool isRunning() const;
    QString moduleName() const { return m_name; }

signals:
    void moduleStarted(const QString &name);
    void moduleExited(const QString &name, int exitCode);

private slots:
    void onProcessFinished(int exitCode, QProcess::ExitStatus status);

private:
    QString    m_name;
    QString    m_execName;
    QString    m_socketPath;
    QProcess  *m_process = nullptr;
    QTimer    *m_restartTimer;
    int        m_restartCount = 0;

    static constexpr int MAX_RESTARTS    = 5;
    static constexpr int RESTART_DELAY_MS = 2000;
};

#endif // MODULECONTROLLER_H
