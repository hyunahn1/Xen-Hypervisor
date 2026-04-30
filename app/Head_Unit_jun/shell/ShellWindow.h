/**
 * @file ShellWindow.h
 * @brief Head Unit Shell — QtWayland Compositor 기반 멀티프로세스 아키텍처
 *
 * hu_shell은 Wayland 컴포지터로 동작합니다.
 * 각 모듈(media, youtube, call, navigation, ambient, settings)은
 * 별도 프로세스로 실행되어 HUCompositor에 Wayland 클라이언트로 연결됩니다.
 *
 * 프로세스 격리: YouTube 크래시 시 hu_shell과 다른 모듈은 계속 동작.
 * HU ↔ IC 통신: VSomeIP (별도 프로세스/VM 격리 유지).
 */

#ifndef SHELLWINDOW_H
#define SHELLWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include <QEvent>
#include <QMouseEvent>
#include <functional>

class TabBar;
class StatusBar;
class GlowOverlay;
class SplashScreen;
class ReverseCameraWindow;
class IVehicleDataProvider;
class VSomeIPClient;
class GearStateManager;
class ILedController;
class HUCompositor;
class ModuleSurfaceWidget;
class ClusterOutputWindow;
class ModuleController;
class ModuleBridge;
class PdcController;
class PdcBeepController;

#ifdef HU_WAYLAND_COMPOSITOR
class QWaylandSurface;
#endif

enum class GearState : quint8;

class ShellWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit ShellWindow(QWidget *parent = nullptr);
    ~ShellWindow() override = default;

private slots:
    void onTabChanged(int index);
    void onGearChanged(GearState gear, const QString &source);
    void onAmbientColorChanged(quint8 r, quint8 g, quint8 b, quint8 brightness);
    void onAmbientOff();
    void pollGamepadGear();
#ifdef HU_WAYLAND_COMPOSITOR
    void onModuleSurfaceCreated(const QString &moduleName, QWaylandSurface *surface);
    void onModuleSurfaceDestroyed(const QString &moduleName);
    void onClusterSurfaceCreated(QWaylandSurface *surface);
#endif

    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    void setupUI();
    void setupModules();
    void setupConnections();
    void setupClusterWindow();
    void switchToModule(int index);
    void broadcastToAllModules(std::function<void(ModuleBridge *)> fn);

    // ── UI 컴포넌트 ───────────────────────────────────────────────────
    TabBar               *m_tabBar        = nullptr;
    StatusBar            *m_statusBar     = nullptr;
    GlowOverlay          *m_ambientGlow   = nullptr;
    ReverseCameraWindow  *m_reverseCamera = nullptr;
    ModuleSurfaceWidget  *m_surfaceWidget = nullptr;

    // ── 차량 데이터 ───────────────────────────────────────────────────
    IVehicleDataProvider *m_vehicleData      = nullptr;
    VSomeIPClient        *m_vsomeipClient    = nullptr;
    GearStateManager     *m_gearStateManager = nullptr;
    ILedController       *m_ledController    = nullptr;
    PdcController        *m_pdcController    = nullptr;
    PdcBeepController    *m_pdcBeep          = nullptr;

    // ── Wayland 컴포지터 ──────────────────────────────────────────────
#ifdef HU_WAYLAND_COMPOSITOR
    HUCompositor         *m_compositor     = nullptr;
    ClusterOutputWindow  *m_clusterWindow  = nullptr;
#endif

    // ── 멀티프로세스 모듈 관리 (ModuleController + ModuleBridge 쌍) ──
    static constexpr int MODULE_COUNT  = 6;
    static constexpr int IDX_MEDIA     = 0;
    static constexpr int IDX_YOUTUBE   = 1;
    static constexpr int IDX_CALL      = 2;
    static constexpr int IDX_NAVIGATION= 3;
    static constexpr int IDX_AMBIENT   = 4;
    static constexpr int IDX_SETTINGS  = 5;

    ModuleController *m_controllers[MODULE_COUNT] = {};
    ModuleBridge     *m_bridges[MODULE_COUNT]     = {};

    int     m_activeIndex    = 0;
    bool    m_lastIpcStatus  = false;
    QTimer *m_ipcPollTimer      = nullptr;
    QString m_lastFileGearDir;
    QTimer *m_gearFilePollTimer = nullptr;

    int m_screenW = 1024;
    int m_screenH = 600;

    static constexpr int TAB_H    = 40;
    static constexpr int STATUS_H = 32;
};

#endif // SHELLWINDOW_H
