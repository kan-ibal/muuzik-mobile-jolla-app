#include "mprisplayer.h"
#include "../pulse/volumeProxy.h"
#include "../artwork.h"
#include "../../virtualfiles/VFservice.h"
#include <QFileInfo>
#include <QUrl>

#define MPRIS_TRACK_PREFIX "/org/mpris/MediaPlayer2/Track/"

MprisPlayer::MprisPlayer(playerService *s)
    : QDBusAbstractAdaptor(s)
    , service(s)
    , lastPlaybackStatus(QStringLiteral("Stopped"))
    , lastCanPlay(false)
    , lastCanPause(false)
    , lastCanSeek(false)
    , lastCanGoNext(false)
    , lastCanGoPrevious(false)
    , syncTimer(new QTimer(this)) {
    syncTimer->setSingleShot(true);
    syncTimer->setInterval(200);
    connect(syncTimer, SIGNAL(timeout()), this, SLOT(syncProperties()));

    connect(s,SIGNAL(titleChanged(MetaData*)),this, SLOT(changementTitre(MetaData*)));
    connect(s,SIGNAL(listChanged()),this, SLOT(changementPlaylist()));
    connect(s,SIGNAL(modeChanged()),this, SLOT(changementMode()));
    connect(s,SIGNAL(statusChanged(bool)),this, SLOT(changementEtat(bool)));
    connect(&ArtWorkService::getInstance(), SIGNAL(artFound(QString,QString)),
            this, SLOT(changementPochette(QString,QString)), Qt::QueuedConnection);
}

/**
 * @brief MprisPlayer::Next
 */
void  MprisPlayer::Next(){
    qDebug() << "!!! MprisPlayer::Next";

    if (!service) {
        return;
    }

    service->next();
}

/**
 * @brief MprisPlayer::Previous
 */
void MprisPlayer:: Previous(){
    qDebug() << "!!! MprisPlayer::Previous";

    if (!service) {
        return;
    }

    service->previous();
}

/**
 * @brief MprisPlayer::Pause
 */
void  MprisPlayer::Pause(){
    qDebug() << "!!! MprisPlayer::Pause";

    if(!service) {
        return;
    }

    service->pause();
}

/**
 * @brief MprisPlayer::PlayPause
 */
void  MprisPlayer::PlayPause(){
    qDebug() << "!!! MprisPlayer::PlayPause";

    if(!service) {
        return;
    }

    service->play();
}

/**
 * @brief MprisPlayer::Stop
 */
void  MprisPlayer::Stop(){
    qDebug() << "!!! MprisPlayer::Stop";

    if(!service) {
        return;
    }

    service->stop();
}

/**
 * @brief MprisPlayer::Play
 */
void  MprisPlayer::Play(){
    qDebug() << "!!! MprisPlayer::Play";
    if(!service || service->getStatus() == true) {
        return;
    }

    service->play();
}

/**
 * @brief MprisPlayer::Seek
 * @param offset in microseconds (relative)
 */
void MprisPlayer::Seek(qlonglong offset) {
    if (!service || !service->media) {
        return;
    }

    qint64 currentMs = service->getPositionMs();
    qint64 durationMs = service->media->duration();
    if (durationMs <= 0) {
        MetaData *t = service->getTitle();
        if (t && t->getDuree() > 0) {
            durationMs = static_cast<qint64>(t->getDuree()) * 1000LL;
        }
    }

    qint64 targetMs = currentMs + (offset / 1000LL);
    if (targetMs < 0) {
        targetMs = 0;
    }
    if (durationMs > 0 && targetMs > durationMs) {
        targetMs = durationMs;
    }

    if (service->media->state() == QMediaPlayer::StoppedState && service->media->duration() <= 0) {
        service->pendingRestorePositionMs = targetMs;
        emit service->dureeChanged(targetMs / 1000);
        emit Seeked(static_cast<qlonglong>(targetMs) * 1000LL);
        service->persistSession();
        return;
    }

    service->pendingRestorePositionMs = -1;
    service->media->setPosition(targetMs);
    emit Seeked(static_cast<qlonglong>(targetMs) * 1000LL);
    service->persistSession();
}

/**
 * @brief MprisPlayer::SetPosition
 * @param TrackId
 * @param Position in microseconds (absolute)
 */
void MprisPlayer::SetPosition(const QDBusObjectPath &TrackId, qlonglong Position) {
    if (!service || !service->media) {
        return;
    }

    MetaData *t = service->getTitle();
    if (!t) {
        return;
    }

    QString expectedPath = QString("%1%2").arg(MPRIS_TRACK_PREFIX).arg(qAbs(t->getUuid()));
    if (TrackId.path() != expectedPath) {
        return;
    }

    qint64 targetMs = Position / 1000LL;
    qint64 durationMs = service->media->duration();
    if (durationMs <= 0 && t->getDuree() > 0) {
        durationMs = static_cast<qint64>(t->getDuree()) * 1000LL;
    }
    if (targetMs < 0 || (durationMs > 0 && targetMs > durationMs)) {
        return;
    }

    if (service->media->state() == QMediaPlayer::StoppedState && service->media->duration() <= 0) {
        service->pendingRestorePositionMs = targetMs;
        emit service->dureeChanged(targetMs / 1000);
        emit Seeked(static_cast<qlonglong>(targetMs) * 1000LL);
        service->persistSession();
        return;
    }

    service->pendingRestorePositionMs = -1;
    service->media->setPosition(targetMs);
    emit Seeked(static_cast<qlonglong>(targetMs) * 1000LL);
    service->persistSession();
}

void  MprisPlayer::OpenUri(QString uri){
    Q_UNUSED(uri);
}

/**
 * @brief MprisPlayer::getPlaybackStatus
 * @return
 */
QString MprisPlayer::getPlaybackStatus() {
    if (!service) {
        return QStringLiteral("Stopped");
    }
    if (service->getStatus() == true) {
        return QStringLiteral("Playing");
    }

    if (service->getSize() == 0) {
        return QStringLiteral("Stopped");
    }

    return QStringLiteral("Paused");
}

/**
 * @brief MprisPlayer::getLoopStatus
 * @return
 */
QString MprisPlayer::getLoopStatus() {
    if(service && service->getLoop()) {
        return "Playlist";
    }
    return "None";
}

/**
 * @brief MprisPlayer::setLoopStatus
 * @param v
 */
void  MprisPlayer::setLoopStatus(QString v) {
    if (!service) {
        return;
    }

    if( v.compare("None",Qt::CaseInsensitive)==0) {
        if(service->getLoop()==true) {
            service->switchLoop();
        }
        return;
    }

    if(v.compare("Playlist",Qt::CaseInsensitive)==0 || v.compare("Track",Qt::CaseInsensitive)==0) {
        if(service->getLoop()==false) {
            service->switchLoop();
        }
    }
}

/**
 * @brief MprisPlayer::getShuffle
 * @return
 */
bool MprisPlayer::getShuffle() {
    return service && service->getRandom();
}

void MprisPlayer::setShuffle(bool v) {
    if(!service || service->getRandom() == v) {
        return;
    }

    service->switchRandom();
}

/**
 * @brief MprisPlayer::getPosition
 * @return position in microseconds
 */
qlonglong MprisPlayer::getPosition() {
    if (!service) {
        return 0;
    }
    return static_cast<qlonglong>(service->getPositionMs()) * 1000LL;
}

/**
 * @brief MprisPlayer::getMetadata
 * @return
 */
QVariantMap MprisPlayer::getMetadata() {
    return metadata;
}

double MprisPlayer::getVolume() {
    double m = VolumeProxy::getInstance().getStepCount();
    if(m==0) {
        return 0;
    }

    double v = VolumeProxy::getInstance().getCurrentStep();

    return v/m;
}

void MprisPlayer::setVolume(double v) {
    double m = VolumeProxy::getInstance().getStepCount();
    if(m==0) {
        return ;
    }

    VolumeProxy::getInstance().setCurrentStep(v*m);
}

/**
 * @brief MprisPlayer::getCanGoNext
 * @return
 */
bool MprisPlayer::getCanGoNext() {
    return service && service->aSuivant();
}

/**
 * @brief MprisPlayer::getCanGoPrevious
 * @return
 */
bool MprisPlayer::getCanGoPrevious() {
    return service && service->aPrecedent();
}

/**
 * @brief MprisPlayer::getCanPlay
 * @return
 */
bool MprisPlayer::getCanPlay() {
    return service && service->getSize() > 0;
}

bool MprisPlayer::getCanPause() {
    return service && service->getSize() > 0;
}

bool MprisPlayer::getCanSeek() {
    return service && service->getSize() > 0;
}

/**
 * @brief MprisPlayer::buildMetadata
 * @param t
 */
void MprisPlayer::buildMetadata(MetaData *t) {
    metadata.clear();
    currentTrackChemin.clear();

    if( t != NULL ) {
        currentTrackChemin = t->getChemin();

        QString trackPath = QString("%1%2").arg(MPRIS_TRACK_PREFIX).arg(qAbs(t->getUuid()));
        metadata.insert("mpris:trackid", QVariant::fromValue(QDBusObjectPath(trackPath)));

        qlonglong lengthUs = 0;
        if(t->getDuree() > 0) {
            lengthUs = static_cast<qlonglong>(t->getDuree()) * 1000000LL;
        } else if(service && service->media && service->media->duration() > 0) {
            lengthUs = static_cast<qlonglong>(service->media->duration()) * 1000LL;
        }
        if(lengthUs > 0) {
            metadata.insert("mpris:length", QVariant::fromValue(lengthUs));
        }

        QString title = t->getTitre();
        if (title.isEmpty() && !currentTrackChemin.isEmpty()) {
            title = QFileInfo(currentTrackChemin).completeBaseName();
            if (title.isEmpty()) {
                title = currentTrackChemin;
            }
        }
        if (!title.isEmpty()) {
            metadata.insert("xesam:title", title);
        }

        if(!t->getAlbum().isEmpty()) {
            metadata.insert("xesam:album", t->getAlbum());
        }
        if(!t->getArtiste().isEmpty()) {
            metadata.insert("xesam:artist", QStringList() << t->getArtiste());
        }
        if(t->getAnnee() > 0) {
            metadata.insert("xesam:contentCreated",
                            QString("%1-01-01T00:00:00Z").arg(t->getAnnee(), 4, 10, QChar('0')));
        }
        if(!currentTrackChemin.isEmpty()) {
            ArtWorkService::getInstance().getArtAsync(currentTrackChemin);
        }
    }
}

void MprisPlayer::appendCapabilityChanges(QVariantMap &p, bool forceAll) {
    QString status = getPlaybackStatus();
    if (forceAll || status != lastPlaybackStatus) {
        lastPlaybackStatus = status;
        p.insert("PlaybackStatus", status);
    }

    if (forceAll) {
        p.insert("CanControl", getCanControl());
    }

    bool canPlay = getCanPlay();
    if (forceAll || canPlay != lastCanPlay) {
        lastCanPlay = canPlay;
        p.insert("CanPlay", canPlay);
    }

    bool canPause = getCanPause();
    if (forceAll || canPause != lastCanPause) {
        lastCanPause = canPause;
        p.insert("CanPause", canPause);
    }

    bool canSeek = getCanSeek();
    if (forceAll || canSeek != lastCanSeek) {
        lastCanSeek = canSeek;
        p.insert("CanSeek", canSeek);
    }

    bool canGoNext = getCanGoNext();
    if (forceAll || canGoNext != lastCanGoNext) {
        lastCanGoNext = canGoNext;
        p.insert("CanGoNext", canGoNext);
    }

    bool canGoPrevious = getCanGoPrevious();
    if (forceAll || canGoPrevious != lastCanGoPrevious) {
        lastCanGoPrevious = canGoPrevious;
        p.insert("CanGoPrevious", canGoPrevious);
    }
}

void MprisPlayer::scheduleSync() {
    if (syncTimer) {
        syncTimer->start();
    }
}

void MprisPlayer::syncProperties() {
    QVariantMap p;
    if (service) {
        MetaData *t = service->getTitle();
        if (t != NULL && metadata.isEmpty()) {
            buildMetadata(t);
        }
    }
    if (!metadata.isEmpty()) {
        p.insert("Metadata", metadata);
    }
    appendCapabilityChanges(p, true);
    p.insert("Shuffle", getShuffle());
    p.insert("LoopStatus", getLoopStatus());
    sendDBusProperty(p);
}

/**
 * @brief MprisPlayer::changementTitre
 * @param t
 */
void MprisPlayer::changementTitre(MetaData *t) {
    buildMetadata(t);

    QVariantMap p;
    p.insert("Metadata", metadata);
    appendCapabilityChanges(p, true);

    sendDBusProperty(p);
    scheduleSync();
}

/**
 * @brief MprisPlayer::changementDuree
 * @param durationMs
 */
void MprisPlayer::changementDuree(qint64 durationMs) {
    if (durationMs <= 0 || metadata.isEmpty()) {
        return;
    }

    qlonglong newLengthUs = static_cast<qlonglong>(durationMs) * 1000LL;
    if (!metadata.contains("mpris:length") || metadata.value("mpris:length").toLongLong() <= 0) {
        metadata.insert("mpris:length", QVariant::fromValue(newLengthUs));
        QVariantMap p;
        p.insert("Metadata", metadata);
        appendCapabilityChanges(p, true);
        sendDBusProperty(p);
    }
}

/**
 * @brief MprisPlayer::changementPochette
 * @param vurl
 * @param artPath
 */
void MprisPlayer::changementPochette(const QString &vurl, const QString &artPath) {
    if (artPath.isEmpty() || currentTrackChemin.isEmpty() || metadata.isEmpty()) {
        return;
    }

    if (vurl == currentTrackChemin || VFI.getParentVUrl(currentTrackChemin) == vurl) {
        QString fileUrl = (artPath.startsWith("file://") || artPath.startsWith("http://") || artPath.startsWith("https://"))
                ? artPath
                : QUrl::fromLocalFile(artPath).toString();
        if (metadata.value("mpris:artUrl").toString() != fileUrl) {
            metadata.insert("mpris:artUrl", fileUrl);
            QVariantMap p;
            p.insert("Metadata", metadata);
            appendCapabilityChanges(p, true);
            sendDBusProperty(p);
        }
    }
}

/**
 * @brief MprisPlayer::changementPlaylist
 */
void MprisPlayer::changementPlaylist() {
    QVariantMap p;
    appendCapabilityChanges(p, true);
    sendDBusProperty(p);
    scheduleSync();
}

/**
 * @brief MprisPlayer::changementEtat
 * @param s
 */
void MprisPlayer::changementEtat(bool s) {
    Q_UNUSED(s);
    QVariantMap p;

    MetaData *t = service ? service->getTitle() : NULL;
    if (t != NULL && metadata.isEmpty()) {
        buildMetadata(t);
        p.insert("Metadata", metadata);
    }

    appendCapabilityChanges(p, true);

    sendDBusProperty(p);
}

/**
 * @brief MprisPlayer::changementMode
 */
void MprisPlayer::changementMode() {
    QVariantMap p;
    p.insert("Shuffle", getShuffle());
    p.insert("LoopStatus", getLoopStatus());
    appendCapabilityChanges(p, true);
    sendDBusProperty(p);
}

/**
 * @brief MprisPlayer::sendDBusProperty
 * @param value
 */
void MprisPlayer::sendDBusProperty(const QVariantMap &value) {
    if (value.isEmpty()) {
        return;
    }
    QDBusMessage signal = QDBusMessage::createSignal(MPRIS_OBJ,FREEDESKTOP_PROPERTIES,"PropertiesChanged" );
    signal << MPRIS_PLAYER_ID;
    signal << value;
    signal << QStringList();
    QDBusConnection::sessionBus().send(signal);
}


