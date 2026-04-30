/**
 * @file MediaWindow.cpp
 * @brief Media tab - Now Playing, playlist, cover art, error handling
 * @author Ahn Hyunjun
 * @date 2026-02-20
 */

#include "MediaWindow.h"
#include "MediaPlayer.h"
#include "PlaylistModel.h"
#include "GearPanel.h"
#include "GearStateManager.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFrame>
#include <QFileDialog>
#include <QDir>
#include <QSizePolicy>
#include <QTimer>
#include <QImage>
#include <QPainter>
#include <QLinearGradient>

// ─────────────────────────────────────────────────────────────
static const char *kScreenStyle = R"(
    QWidget { background-color: #0D0D0F; color: #FFFFFF; }
    QLabel  { background: transparent; }
    QSlider::groove:horizontal {
        background: #2C2C30; height: 6px; border-radius: 3px;
    }
    QSlider::sub-page:horizontal {
        background: #00D4AA; height: 6px; border-radius: 3px;
    }
    QSlider::handle:horizontal {
        background: #FFFFFF; width: 14px; height: 14px;
        margin: -4px 0; border-radius: 7px;
    }
    QListView {
        background-color: #1A1A1E; border: none;
        border-radius: 12px; padding: 4px; outline: none;
    }
    QListView::item {
        color: #B3B3B7; padding: 4px 10px; border-radius: 6px; border: none;
        font-size: 9pt;
    }
    QListView::item:hover    { background: #252529; color: #FFFFFF; }
    QListView::item:selected { background: #00D4AA22; color: #00D4AA; }
    QScrollBar:vertical {
        background: #1A1A1E; width: 6px; border-radius: 3px;
    }
    QScrollBar::handle:vertical { background: #3A3A3F; border-radius: 3px; }
    QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }
)";

static const char *kCardStyle    = "background-color: #1A1A1E; border-radius: 16px;";
static const char *kCtrlBtnStyle = R"(
    QPushButton {
        background-color: #252529; color: #FFFFFF;
        border: none; border-radius: 14px; font-size: 11pt;
    }
    QPushButton:hover   { background-color: #3A3A3F; }
    QPushButton:pressed { background-color: #444448; }
)";
static const char *kPlayBtnStyle = R"(
    QPushButton {
        background-color: #00D4AA; color: #000000;
        border: none; border-radius: 16px; font-size: 12pt;
    }
    QPushButton:hover   { background-color: #00EFBF; }
    QPushButton:pressed { background-color: #00A887; }
)";
static const char *kAddBtnStyle  = R"(
    QPushButton {
        background-color: transparent; color: #00D4AA;
        border: 1px solid #00D4AA33; border-radius: 6px;
        font-size: 9pt; padding: 2px 8px;
    }
    QPushButton:hover { background-color: #00D4AA18; border-color: #00D4AA; }
)";

// ─────────────────────────────────────────────────────────────
MediaWindow::MediaWindow(GearStateManager *gearState, QWidget *parent)
    : QWidget(parent)
    , m_player(new MediaPlayer(this))
    , m_playlist(new PlaylistModel(this))
    , m_gearPanel(nullptr)
{
    setStyleSheet(kScreenStyle);

    QHBoxLayout *root = new QHBoxLayout(this);
    root->setContentsMargins(0, 6, 10, 6);
    root->setSpacing(14);

    QVBoxLayout *content = new QVBoxLayout();
    content->setSpacing(8);

    // ── Now Playing Card ─────────────────────────────────────
    QFrame *card = new QFrame(this);
    card->setStyleSheet(kCardStyle);
    card->setMinimumHeight(120);
    card->setMaximumHeight(160);
    card->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    QHBoxLayout *cardLayout = new QHBoxLayout(card);
    cardLayout->setContentsMargins(10, 8, 10, 8);
    cardLayout->setSpacing(10);

    // Album art - shows cover if embedded, otherwise ♪ placeholder
    m_albumArt = new QLabel(card);
    m_albumArt->setFixedSize(80, 80);
    m_albumArt->setAlignment(Qt::AlignCenter);
    showPlaceholderArt();

    // Right info column
    QVBoxLayout *info = new QVBoxLayout();
    info->setSpacing(4);

    m_trackTitle = new QLabel("No Track Selected", card);
    m_trackTitle->setStyleSheet("font-size: 11pt; font-weight: bold; color: #FFFFFF;");
    m_trackTitle->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    m_trackTitle->setWordWrap(false);

    m_artistName = new QLabel("", card);
    m_artistName->setStyleSheet("font-size: 9pt; color: #B3B3B7;");

    // Controls
    QHBoxLayout *controls = new QHBoxLayout();
    controls->setSpacing(6);
    m_prevBtn      = new QPushButton("◀◀", card);
    m_playPauseBtn = new QPushButton("▶", card);
    m_nextBtn      = new QPushButton("▶▶", card);
    m_prevBtn->setFixedSize(28, 28);
    m_nextBtn->setFixedSize(28, 28);
    m_playPauseBtn->setFixedSize(32, 32);
    m_prevBtn->setStyleSheet(kCtrlBtnStyle);
    m_nextBtn->setStyleSheet(kCtrlBtnStyle);
    m_playPauseBtn->setStyleSheet(kPlayBtnStyle);
    controls->addStretch();
    controls->addWidget(m_prevBtn);
    controls->addWidget(m_playPauseBtn);
    controls->addWidget(m_nextBtn);
    controls->addStretch();

    // Progress bar
    QHBoxLayout *progressRow = new QHBoxLayout();
    progressRow->setSpacing(8);
    m_posLabel = new QLabel("0:00", card);
    m_posLabel->setStyleSheet("font-size: 8pt; color: #6E6E73; min-width: 26px;");
    m_progressSlider = new QSlider(Qt::Horizontal, card);
    m_progressSlider->setRange(0, 0);
    m_progressSlider->setTracking(false);
    m_durLabel = new QLabel("0:00", card);
    m_durLabel->setStyleSheet("font-size: 8pt; color: #6E6E73; min-width: 26px;");
    m_durLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    progressRow->addWidget(m_posLabel);
    progressRow->addWidget(m_progressSlider, 1);
    progressRow->addWidget(m_durLabel);

    // Volume
    QHBoxLayout *volRow = new QHBoxLayout();
    volRow->setSpacing(8);
    QLabel *volIcon = new QLabel("🔊", card);
    volIcon->setStyleSheet("font-size: 9pt;");
    m_volumeSlider = new QSlider(Qt::Horizontal, card);
    m_volumeSlider->setRange(0, 100);
    m_volumeSlider->setValue(50);
    m_volumeSlider->setMaximumWidth(100);
    volRow->addStretch();
    volRow->addWidget(volIcon);
    volRow->addWidget(m_volumeSlider);

    info->addWidget(m_trackTitle);
    info->addWidget(m_artistName);
    info->addStretch();
    info->addLayout(controls);
    info->addLayout(progressRow);
    info->addLayout(volRow);

    cardLayout->addWidget(m_albumArt);
    cardLayout->addLayout(info, 1);

    // ── Playlist section ─────────────────────────────────────
    QHBoxLayout *listHeader = new QHBoxLayout();
    listHeader->setContentsMargins(4, 0, 8, 0);
    QLabel *listTitle = new QLabel("Playlist", this);
    listTitle->setStyleSheet("font-size: 10pt; font-weight: bold; color: #FFFFFF;");
    QPushButton *addBtn = new QPushButton("+ Add Folder", this);
    addBtn->setStyleSheet(kAddBtnStyle);
    listHeader->addWidget(listTitle);
    listHeader->addStretch();
    listHeader->addWidget(addBtn);

    m_playlistView = new QListView(this);
    m_playlistView->setModel(m_playlist);
    m_playlistView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_playlistView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    content->addWidget(card);
    content->addLayout(listHeader);
    content->addWidget(m_playlistView, 1);

    m_gearPanel = new GearPanel(gearState, this);
    root->addWidget(m_gearPanel);
    root->addLayout(content, 1);

    // ── Connections ───────────────────────────────────────────
    connect(m_prevBtn,      &QPushButton::clicked, this, &MediaWindow::onPrev);
    connect(m_playPauseBtn, &QPushButton::clicked, this, &MediaWindow::onPlayPause);
    connect(m_nextBtn,      &QPushButton::clicked, this, &MediaWindow::onNext);
    connect(addBtn,         &QPushButton::clicked, this, &MediaWindow::onAddFolder);

    connect(m_player, &MediaPlayer::positionChanged, this, &MediaWindow::onPositionChanged);
    connect(m_player, &MediaPlayer::durationChanged, this, &MediaWindow::onDurationChanged);
    connect(m_player, &MediaPlayer::stateChanged,    this, &MediaWindow::onStateChanged);
    connect(m_player, &MediaPlayer::trackFinished,   this, &MediaWindow::onTrackFinished);
    connect(m_player, &MediaPlayer::coverArtChanged, this, &MediaWindow::onCoverArtChanged);
    connect(m_player, &MediaPlayer::errorOccurred,   this, &MediaWindow::onError);

    connect(m_progressSlider, &QSlider::sliderPressed,  this, [this] { m_seeking = true; });
    connect(m_progressSlider, &QSlider::sliderReleased, this, [this] {
        m_seeking = false;
        m_player->seekTo((qint64)m_progressSlider->value() * 1000);
    });
    connect(m_progressSlider, &QSlider::valueChanged,
            this, &MediaWindow::onProgressSliderMoved);
    connect(m_volumeSlider, &QSlider::valueChanged,
            this, &MediaWindow::onVolumeChanged);
    connect(m_playlistView, &QListView::clicked,
            this, &MediaWindow::onPlaylistItemActivated);
    connect(m_playlistView, &QListView::activated,
            this, &MediaWindow::onPlaylistItemActivated);
}

// ─────────────────────────────────────────────────────────────
// Deterministically maps a track title to a vivid HSV colour.
// Same title → same hue every time (no randomness).
static QColor colorFromTitle(const QString &title)
{
    const uint h  = qHash(title, 42u);
    const int  hue = int(h % 360);
    return QColor::fromHsv(hue, 200, 220);
}

// ─────────────────────────────────────────────────────────────
// Extracts the most vibrant/saturated colour from an album art pixmap.
// Skips near-black and near-white pixels, weights remaining pixels by
// saturation so the dominant vivid hue wins over dull backgrounds.
QColor MediaWindow::extractVibrantColor(const QPixmap &pixmap)
{
    if (pixmap.isNull()) return QColor(0, 212, 170); // default teal

    // Downsample for speed (64×64 is plenty for colour analysis)
    const QImage img = pixmap.scaled(64, 64, Qt::IgnoreAspectRatio,
                                     Qt::FastTransformation).toImage();

    double rSum = 0, gSum = 0, bSum = 0, wSum = 0;
    for (int y = 0; y < img.height(); ++y) {
        for (int x = 0; x < img.width(); ++x) {
            const QColor c = img.pixelColor(x, y);
            int h, s, v;
            c.getHsv(&h, &s, &v);
            if (v < 40 || v > 220) continue; // skip too dark / too bright
            if (s < 40)            continue; // skip desaturated (grays)
            const double w = s / 255.0;      // weight by saturation
            rSum += c.red()   * w;
            gSum += c.green() * w;
            bSum += c.blue()  * w;
            wSum += w;
        }
    }

    if (wSum < 1.0) return QColor(0, 212, 170); // fallback: mostly grayscale art

    QColor result(int(rSum / wSum), int(gSum / wSum), int(bSum / wSum));

    // Clamp to a vibrant, readable range
    int h, s, v;
    result.getHsv(&h, &s, &v);
    s = qMin(255, qMax(160, s)); // at least s=160 (vivid)
    v = qMin(255, qMax(190, v)); // bright enough to read against dark UI
    result.setHsv(h, s, v);

    return result;
}

// Applies accent colour to play button, progress bar and playlist highlight.
void MediaWindow::applyAccentColor(const QColor &color)
{
    m_accentColor = color;
    // Guard: widgets may not yet be constructed (called from constructor)
    if (!m_playPauseBtn || !m_progressSlider || !m_playlistView) return;

    const QString hex  = color.name();                   // e.g. "#ff6b35"
    const QString hexH = color.lighter(115).name();      // hover
    const QString hexP = color.darker(120).name();       // pressed
    const QString hexA = hex + "30";                     // ~19% alpha for selection bg

    m_playPauseBtn->setStyleSheet(QString(R"(
        QPushButton {
            background-color: %1; color: #000000;
            border: none; border-radius: 16px; font-size: 12pt;
        }
        QPushButton:hover   { background-color: %2; }
        QPushButton:pressed { background-color: %3; }
    )").arg(hex, hexH, hexP));

    m_progressSlider->setStyleSheet(QString(R"(
        QSlider::groove:horizontal {
            background: #2C2C30; height: 6px; border-radius: 3px;
        }
        QSlider::sub-page:horizontal {
            background: %1; height: 6px; border-radius: 3px;
        }
        QSlider::handle:horizontal {
            background: #FFFFFF; width: 14px; height: 14px;
            margin: -4px 0; border-radius: 7px;
        }
    )").arg(hex));

    m_playlistView->setStyleSheet(QString(R"(
        QListView {
            background-color: #1A1A1E; border: none;
            border-radius: 12px; padding: 4px; outline: none;
        }
        QListView::item {
            color: #B3B3B7; padding: 4px 10px; border-radius: 6px; border: none;
            font-size: 9pt;
        }
        QListView::item:hover    { background: #252529; color: #FFFFFF; }
        QListView::item:selected { background: %1; color: %2; }
        QScrollBar:vertical {
            background: #1A1A1E; width: 6px; border-radius: 3px;
        }
        QScrollBar::handle:vertical { background: #3A3A3F; border-radius: 3px; }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }
    )").arg(hexA, hex));

    emit accentColorChanged(color);
}

void MediaWindow::showPlaceholderArt()
{
    m_albumArt->clear();
    m_albumArt->setText("♪");
    m_albumArt->setStyleSheet(
        "background-color: #252529; border-radius: 10px;"
        "color: #6E6E73; font-size: 28pt;");
    applyAccentColor(QColor(0, 212, 170)); // reset to default teal
}

void MediaWindow::onCoverArtChanged(const QPixmap &art)
{
    if (art.isNull()) {
        if (m_currentTrack >= 0) {
            // No embedded art → generate a gradient placeholder from track title
            const QColor c = colorFromTitle(m_playlist->titleAt(m_currentTrack));
        QPixmap pm(80, 80);
        QPainter p(&pm);
        p.setRenderHint(QPainter::Antialiasing);
        QLinearGradient grad(0, 0, 80, 80);
        grad.setColorAt(0, c.lighter(130));
        grad.setColorAt(1, c.darker(160));
        p.fillRect(pm.rect(), grad);
        p.setPen(QColor(255, 255, 255, 160));
        QFont f = p.font();
        f.setPointSize(28);
        p.setFont(f);
        p.drawText(pm.rect(), Qt::AlignCenter, "♪");
        p.end();
            m_albumArt->setPixmap(pm);
            m_albumArt->setText("");
            m_albumArt->setStyleSheet("border-radius: 12px;");
            applyAccentColor(c);
        } else {
            showPlaceholderArt();
        }
    } else {
        // Scale to fit 96x96 with rounded corners via stylesheet
        const QPixmap scaled = art.scaled(
            80, 80, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
        // Crop to exact 80x80 from center
        const QPixmap cropped = scaled.copy(
            (scaled.width()  - 80) / 2,
            (scaled.height() - 80) / 2,
            80, 80);
        m_albumArt->setPixmap(cropped);
        m_albumArt->setText("");
        m_albumArt->setStyleSheet("border-radius: 8px;");

        // Sync UI accent colour to the album art palette
        applyAccentColor(extractVibrantColor(cropped));
    }
}

void MediaWindow::onError(const QString &msg)
{
    Q_UNUSED(msg)
    // Mark the track as unplayable in the UI
    m_trackTitle->setStyleSheet(
        "font-size: 11pt; font-weight: bold; color: #FF4757;");
    m_artistName->setText("Cannot play this file — skipping...");
    m_artistName->setStyleSheet("font-size: 9pt; color: #FF475770;");
    showPlaceholderArt();

    // Auto-skip to next after 1.5s
    QTimer::singleShot(1500, this, [this] {
        // Restore normal styles before playing next
        m_trackTitle->setStyleSheet(
            "font-size: 11pt; font-weight: bold; color: #FFFFFF;");
        m_artistName->setStyleSheet("font-size: 9pt; color: #B3B3B7;");
        onNext();
    });
}

// ─────────────────────────────────────────────────────────────
void MediaWindow::onPlayPause()
{
    if (m_player->isPlaying()) {
        m_player->pause();
    } else {
        if (m_currentTrack < 0 && m_playlist->rowCount() > 0)
            playTrack(0);
        else
            m_player->play();
    }
}

void MediaWindow::onPrev()
{
    if (m_currentTrack > 0)
        playTrack(m_currentTrack - 1);
}

void MediaWindow::onNext()
{
    if (m_currentTrack < m_playlist->rowCount() - 1)
        playTrack(m_currentTrack + 1);
}

void MediaWindow::onTrackFinished()
{
    // Defer to next event-loop tick so Qt6 can finish processing
    // the EndOfMedia status change before we load a new source.
    QTimer::singleShot(0, this, [this] { onNext(); });
}

void MediaWindow::onPlaylistItemActivated(const QModelIndex &index)
{
    playTrack(index.row());
}

void MediaWindow::onAddFolder()
{
    const QString dir = QFileDialog::getExistingDirectory(
        this, "Select Music Folder", QDir::homePath());
    if (dir.isEmpty()) return;
    m_playlist->loadFromDirectory(dir);
    if (m_playlist->rowCount() > 0) playTrack(0);
}

void MediaWindow::playTrack(int row)
{
    if (row < 0 || row >= m_playlist->rowCount()) return;
    m_currentTrack = row;
    // Restore normal title style in case previous track had an error
    m_trackTitle->setStyleSheet(
        "font-size: 11pt; font-weight: bold; color: #FFFFFF;");
    m_artistName->setStyleSheet("font-size: 9pt; color: #B3B3B7;");
    m_player->stop();
    m_player->setSource(m_playlist->urlAt(row), true);
    m_trackTitle->setText(m_playlist->titleAt(row));
    m_artistName->setText("");
    m_playlistView->setCurrentIndex(m_playlist->index(row));
}

void MediaWindow::onPositionChanged(qint64 ms)
{
    if (m_seeking) return;
    m_progressSlider->blockSignals(true);
    m_progressSlider->setValue(int(ms / 1000));
    m_progressSlider->blockSignals(false);
    m_posLabel->setText(formatTime(ms));
}

void MediaWindow::onDurationChanged(qint64 ms)
{
    m_progressSlider->setRange(0, int(ms / 1000));
    m_durLabel->setText(formatTime(ms));
}

void MediaWindow::onStateChanged(bool playing)
{
    m_playPauseBtn->setText(playing ? "||" : "▶");
}

void MediaWindow::onProgressSliderMoved(int value)
{
    if (m_seeking)
        m_posLabel->setText(formatTime((qint64)value * 1000));
}

void MediaWindow::onVolumeChanged(int value) { m_player->setVolume(value); }

QString MediaWindow::formatTime(qint64 ms) const
{
    const int secs = int(ms / 1000);
    return QString("%1:%2")
        .arg(secs / 60)
        .arg(secs % 60, 2, 10, QLatin1Char('0'));
}
