/**
 * @file ShellWindow.cpp
 * @brief Head Unit Shell — QtWayland Compositor 기반 멀티프로세스 아키텍처
 *
 * hu_shell = Wayland Compositor (HUCompositor)
 * 6개 모듈 = 독립 Wayland 클라이언트 프로세스 (ModuleController 감시)
 * IPC = ModuleBridge Unix 소켓 (gear/speed/battery/settings)
 * HU↔IC = VSomeIP (VSomeIPClient)
 */

#include "ShellWindow.h"
#include "widgets/TabBar.h"
#include "widgets/StatusBar.h"
#include "widgets/GlowOverlay.h"
#include "widgets/SplashScreen.h"
#include "widgets/ReverseCameraWindow.h"
#include "VSomeIPClient.h"
#include "MockLedController.h"
#include "MockPdcSensorProvider.h"
#include "SocketCanPdcProvider.h"
#include "IPdcSensorProvider.h"
#include "PdcController.h"
#include "PdcBeepController.h"
#include "GearStateManager.h"
#include "IVehicleDataProvider.h"
#ifdef HU_WAYLAND_COMPOSITOR
#include "HUCompositor.h"
#include "ModuleSurfaceWidget.h"
#include "ClusterOutputWindow.h"
#include <QWaylandSurface>
#include <QGuiApplication>
#endif
#include "ModuleController.h"
#include "ModuleBridge.h"

#include <QVBoxLayout>
#include <QWidget>
#include <QApplication>
#include <QScreen>
#include <QMouseEvent>
#include <QDebug>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDateTime>

// ── 모듈 메타데이터 ──────────────────────────────────────────────────────
namespace {
struct ModuleInfo {
    const char *waylandName;  // window title (Wayland 식별자)
    const char *execName;     // 실행파일명
    const char *socketSuffix; // /tmp/hu_shell_<suffix>.sock
};

constexpr ModuleInfo kModules[] = {
    { "media",      "hu_module_media",      "media"      },
    { "youtube",    "hu_module_youtube",    "youtube"    },
    { "call",       "hu_module_call",       "call"       },
    { "navigation", "hu_module_navigation", "navigation" },
    { "ambient",    "hu_module_ambient",    "ambient"    },
    { "settings",   "hu_module_settings",  "settings"   },
};

IPdcSensorProvider *createPdcProvider()
{
    const QString provider = qEnvironmentVariable("HU_PDC_PROVIDER").trimmed().toLower();
    if (provider == QStringLiteral("mock")) {
        qInfo() << "[PDC] Using mock sensor provider";
        return new MockPdcSensorProvider;
    }

    qInfo() << "[PDC] Using SocketCAN sensor provider on can0";
    return new SocketCanPdcProvider(QStringLiteral("can0"));
}
} // namespace

// ────────────────────────────────────────────────────────────────────────────

ShellWindow::ShellWindow(QWidget *parent)
    : QMainWindow(parent)
{
    if (QScreen *screen = QApplication::primaryScreen()) {
        const QRect geo = screen->geometry();
        m_screenW = geo.width();
        m_screenH = geo.height();
    }

    setWindowFlags(Qt::FramelessWindowHint | Qt::Window | Qt::WindowStaysOnTopHint);
    setGeometry(0, 0, m_screenW, m_screenH);
    setWindowTitle("PiRacer Head Unit");
    setStyleSheet("QMainWindow { background-color: #0D0D0F; }");

    m_vsomeipClient    = new VSomeIPClient(this);
    m_vehicleData      = m_vsomeipClient;
    m_gearStateManager = new GearStateManager(this);
    m_ledController    = new MockLedController(this);
    m_pdcController    = new PdcController(createPdcProvider(), this);
    m_pdcBeep          = new PdcBeepController(this);

    setupUI();
    setupModules();
    setupConnections();

    // Ensure window gets focus and stays on top
    show();
    raise();
    activateWindow();
    // Use QTimer to re-activate after event loop starts
    QTimer::singleShot(200, this, [this]{
        raise();
        activateWindow();
        qDebug() << "[Shell] activateWindow called, isActiveWindow=" << isActiveWindow();
    });

    // Debug: install event filter to track where mouse events go
    qApp->installEventFilter(this);
}

bool ShellWindow::eventFilter(QObject *obj, QEvent *event)
{
    switch (event->type()) {
    case QEvent::MouseButtonPress: {
        auto *me = static_cast<QMouseEvent*>(event);
        qDebug() << "[EventFilter] MousePress at" << me->globalPos()
                 << "on" << obj->metaObject()->className();
        break;
    }
    case QEvent::Enter:
        qDebug() << "[EventFilter] Enter:" << obj->metaObject()->className();
        break;
    case QEvent::WindowActivate:
        qDebug() << "[EventFilter] WindowActivate";
        break;
    case QEvent::FocusIn:
        qDebug() << "[EventFilter] FocusIn:" << obj->metaObject()->className();
        break;
    default:
        break;
    }
    return false; // don't consume
}

void ShellWindow::setupUI()
{
    QWidget *central = new QWidget(this);
    setCentralWidget(central);

    auto *layout = new QVBoxLayout(central);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    m_tabBar = new TabBar(this);
    m_tabBar->setFixedHeight(TAB_H);
    layout->addWidget(m_tabBar);

#ifdef HU_WAYLAND_COMPOSITOR
    // Wayland compositor 모드: OpenGL 렌더링 위젯
    m_surfaceWidget = new ModuleSurfaceWidget(this);
    layout->addWidget(m_surfaceWidget, 1);
#else
    // Wayland compositor 미설치 시 빈 위젯 (빌드 확인용)
    auto *placeholder = new QWidget(this);
    placeholder->setStyleSheet("background:#0D0D0F;");
    layout->addWidget(placeholder, 1);
#endif

    m_statusBar = new StatusBar(m_vehicleData, m_gearStateManager, this);
    m_statusBar->setFixedHeight(STATUS_H);
    layout->addWidget(m_statusBar);

    m_ambientGlow = new GlowOverlay(central);
    m_ambientGlow->setGeometry(0, 0, m_screenW, m_screenH);
    m_ambientGlow->raise();

    auto *splash = new SplashScreen(central);
    splash->setGeometry(0, 0, m_screenW, m_screenH);
    splash->raise();
    connect(splash, &SplashScreen::finished, splash, &QWidget::deleteLater);
    connect(splash, &SplashScreen::finished, this, [this] { m_ambientGlow->raise(); });
}

void ShellWindow::setupModules()
{
#ifdef HU_WAYLAND_COMPOSITOR
    // ── 1. Wayland Compositor 생성 ──────────────────────────────────────
    m_compositor = new HUCompositor(this);
    m_compositor->setupOutput(QSize(m_screenW, m_screenH));
    m_compositor->create();

    // Cluster window setup is deferred until after show() so that the eglfs
    // DRM master is fully established on the primary screen (HDMI-A-1) before
    // we attempt to set a mode on the secondary screen (DSI-1).
    QTimer::singleShot(500, this, &ShellWindow::setupClusterWindow);

    connect(m_compositor, &HUCompositor::moduleSurfaceCreated,
            this, &ShellWindow::onModuleSurfaceCreated);
    connect(m_compositor, &HUCompositor::moduleSurfaceDestroyed,
            this, &ShellWindow::onModuleSurfaceDestroyed);
    connect(m_compositor, &HUCompositor::clusterSurfaceCreated,
            this, &ShellWindow::onClusterSurfaceCreated);
#endif

    // ── 2. 각 모듈: ModuleBridge(IPC 서버) + ModuleController(프로세스 감시) ──
    for (int i = 0; i < MODULE_COUNT; ++i) {
        const QString socketPath =
            QString("/tmp/hu_shell_%1.sock").arg(kModules[i].socketSuffix);

        m_bridges[i] = new ModuleBridge(socketPath, this);
        if (!m_bridges[i]->listen()) {
            qWarning() << "[Shell] ModuleBridge failed to listen on" << socketPath;
        }

        m_controllers[i] = new ModuleController(
            kModules[i].waylandName,
            kModules[i].execName,
            socketPath,
            this
        );
        m_controllers[i]->launch();
    }
}

void ShellWindow::setupClusterWindow()
{
#ifdef HU_WAYLAND_COMPOSITOR
    QScreen *clusterScreen = nullptr;
    const auto screens = QGuiApplication::screens();
    for (QScreen *s : screens) {
        if (s->name().contains("DSI", Qt::CaseInsensitive)) {
            clusterScreen = s;
            break;
        }
    }
    if (!clusterScreen && screens.size() > 1)
        clusterScreen = screens.at(1);

    if (clusterScreen) {
        qDebug() << "[Shell] cluster screen found:" << clusterScreen->name()
                 << clusterScreen->size();
        m_clusterWindow = new ClusterOutputWindow(clusterScreen, this);
        m_compositor->setupClusterOutput(m_clusterWindow, clusterScreen->size());
        m_clusterWindow->setWaylandOutput(m_compositor->clusterOutput());
    } else {
        qWarning() << "[Shell] no DSI screen found — cluster display disabled";
    }
#endif
}

void ShellWindow::setupConnections()
{
    connect(m_tabBar, &TabBar::tabSelected, this, &ShellWindow::onTabChanged);
    connect(m_gearStateManager, &GearStateManager::gearChanged,
            this, &ShellWindow::onGearChanged);
    connect(m_pdcController, &PdcController::stateChanged,
            m_pdcBeep, &PdcBeepController::setPdcState);

    // ── 차량 속도 → 모든 모듈에 브로드캐스트 ──────────────────────────
    connect(m_vehicleData, &IVehicleDataProvider::speedChanged,
            this, [this](float speed) {
        broadcastToAllModules([speed](ModuleBridge *b) {
            b->sendVehicleSpeed(speed);
        });
        if (m_pdcController) {
            m_pdcController->setVehicleSpeed(speed);
        }
    });

    // ── Ambient 모듈 신호 → GlowOverlay ──────────────────────────────
    connect(m_bridges[IDX_AMBIENT], &ModuleBridge::ambientColorChanged,
            this, [this](quint8 r, quint8 g, quint8 b, quint8 brightness) {
        onAmbientColorChanged(r, g, b, brightness);
    });
    connect(m_bridges[IDX_AMBIENT], &ModuleBridge::ambientOff,
            this, &ShellWindow::onAmbientOff);

    // ── 각 모듈의 기어 변경 요청 → GearStateManager ──────────────────
    for (int i = 0; i < MODULE_COUNT; ++i) {
        connect(m_bridges[i], &ModuleBridge::gearChangeRequested,
                this, [this](GearState gear, const QString &source) {
            m_gearStateManager->setGear(gear, source);
        });
    }

    // ── IPC 연결 상태 폴링 (1초마다) → 모든 모듈에 브로드캐스트 ────────
    m_ipcPollTimer = new QTimer(this);
    m_ipcPollTimer->setInterval(1000);
    connect(m_ipcPollTimer, &QTimer::timeout, this, [this] {
        const bool connected = m_vsomeipClient && m_vsomeipClient->isConnected();
        if (connected != m_lastIpcStatus) {
            m_lastIpcStatus = connected;
            broadcastToAllModules([connected](ModuleBridge *b) {
                b->sendIpcStatus(connected);
            });
        }
    });
    m_ipcPollTimer->start();

    // ── 게임패드 기어 폴링 (500ms마다 파일 읽기) ─────────────────────
    m_gearFilePollTimer = new QTimer(this);
    m_gearFilePollTimer->setInterval(500);
    connect(m_gearFilePollTimer, &QTimer::timeout,
            this, &ShellWindow::pollGamepadGear);
    m_gearFilePollTimer->start();
}

// ── 탭 전환 ──────────────────────────────────────────────────────────────

void ShellWindow::onTabChanged(int index)
{
    qDebug() << "[Shell] onTabChanged index=" << index;
    if (index == m_activeIndex) return;
    switchToModule(index);
}

void ShellWindow::switchToModule(int index)
{
    if (index < 0 || index >= MODULE_COUNT) return;
    m_activeIndex = index;
    m_tabBar->setCurrentIndex(index);

#ifdef HU_WAYLAND_COMPOSITOR
    // 현재 탭에 해당하는 Wayland surface를 렌더 위젯에 설정
    const QString name = kModules[index].waylandName;
    QWaylandSurface *surface = m_compositor ? m_compositor->surfaceForModule(name) : nullptr;
    if (m_surfaceWidget) m_surfaceWidget->setSurface(surface);
#endif
}

#ifdef HU_WAYLAND_COMPOSITOR
// ── Wayland Surface 이벤트 ────────────────────────────────────────────────

void ShellWindow::onModuleSurfaceCreated(const QString &moduleName,
                                          QWaylandSurface *surface)
{
    qDebug() << "[Shell] module surface created:" << moduleName;
    if (moduleName == kModules[m_activeIndex].waylandName && m_surfaceWidget) {
        m_surfaceWidget->setSurface(surface);
    }
}

void ShellWindow::onModuleSurfaceDestroyed(const QString &moduleName)
{
    qWarning() << "[Shell] module surface destroyed (crash?):" << moduleName;
    if (moduleName == kModules[m_activeIndex].waylandName && m_surfaceWidget) {
        m_surfaceWidget->clearSurface();
    }
}

void ShellWindow::onClusterSurfaceCreated(QWaylandSurface *surface)
{
    qDebug() << "[Shell] cluster surface connected (PiRacerDashboard on DSI-1)";
    if (m_clusterWindow)
        m_clusterWindow->setSurface(surface);
}
#endif

// ── 기어 변경 ─────────────────────────────────────────────────────────────

void ShellWindow::onGearChanged(GearState gear, const QString &source)
{
    Q_UNUSED(source)

    // VSomeIP를 통해 IC에 기어 상태 게시
    if (m_vsomeipClient)
        m_vsomeipClient->publishGear(gear);

    // 모든 모듈에 기어 상태 브로드캐스트 (GearPanel 업데이트용)
    broadcastToAllModules([gear](ModuleBridge *b) {
        b->sendGearState(gear);
    });

    // 후진 기어 → 카메라 창 표시
    const bool isReverse = (static_cast<quint8>(gear) == 1); // GearState::R
    if (m_pdcController) {
        m_pdcController->setActive(isReverse);
    }
    if (isReverse) {
        if (!m_reverseCamera) {
            m_reverseCamera = new ReverseCameraWindow(m_gearStateManager, this);
            m_reverseCamera->setAttribute(Qt::WA_DeleteOnClose);
            if (m_pdcController) {
                connect(m_pdcController, &PdcController::stateChanged,
                        m_reverseCamera, &ReverseCameraWindow::setPdcState);
                m_reverseCamera->setPdcState(m_pdcController->state());
            }
            connect(m_reverseCamera, &QWidget::destroyed, this, [this] {
                m_reverseCamera = nullptr;
            });
            if (QScreen *screen = QApplication::primaryScreen()) {
                const QRect geo = screen->availableGeometry();
                m_reverseCamera->move(
                    geo.x() + (geo.width()  - m_reverseCamera->width())  / 2,
                    geo.y() + (geo.height() - m_reverseCamera->height()) / 2 - 80
                );
            }
        }
        m_reverseCamera->show();
        m_reverseCamera->raise();
    } else {
        if (m_reverseCamera) m_reverseCamera->close();
    }
}

// ── Ambient ───────────────────────────────────────────────────────────────

void ShellWindow::onAmbientColorChanged(quint8 r, quint8 g, quint8 b, quint8 brightness)
{
    m_ambientGlow->setGlow(r, g, b, brightness);
}

void ShellWindow::onAmbientOff()
{
    m_ambientGlow->clearGlow();
}

// ── 게임패드 기어 폴링 ───────────────────────────────────────────────────

static QString gearToDirection(GearState g)
{
    switch (static_cast<quint8>(g)) {
    case 3: return "F";   // GearState::D
    case 1: return "R";   // GearState::R
    default: return "N";  // GearState::P (0), GearState::N (2)
    }
}

void ShellWindow::pollGamepadGear()
{
    static const QString kPath = "/tmp/piracer_drive_mode.json";

    QFileInfo fi(kPath);
    if (!fi.exists()) return;
    if (fi.lastModified().msecsTo(QDateTime::currentDateTime()) > 3000) return;

    QFile f(kPath);
    if (!f.open(QIODevice::ReadOnly)) return;
    const QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
    f.close();
    if (!doc.isObject()) return;

    const QString dir = doc.object().value("direction").toString().trimmed().toUpper();
    if (dir.isEmpty() || dir == m_lastFileGearDir) return;
    m_lastFileGearDir = dir;

    const GearState currentGear = m_gearStateManager->gear();
    if (dir == gearToDirection(currentGear)) return;

    GearState newGear;
    if (dir == "F")      newGear = GearState::D;
    else if (dir == "R") newGear = GearState::R;
    else                 newGear = GearState::N;

    qDebug() << "[Shell] gamepad gear from file:" << dir;
    m_gearStateManager->setGear(newGear, "gamepad");
}

// ── 유틸리티 ─────────────────────────────────────────────────────────────

void ShellWindow::broadcastToAllModules(std::function<void(ModuleBridge *)> fn)
{
    for (int i = 0; i < MODULE_COUNT; ++i) {
        if (m_bridges[i] && m_bridges[i]->isModuleConnected())
            fn(m_bridges[i]);
    }
}
