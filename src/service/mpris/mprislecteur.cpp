#include "mprislecteur.h"
#include "../pulse/volumeProxy.h"
#include "../artwork.h"
#include "../../virtualfiles/VFservice.h"
#include <QUrl>

MprisLecteur::MprisLecteur(lecteurService *s): QDBusAbstractAdaptor(s),service(s),taille(0) {
    connect(s,SIGNAL(titreChanged(MetaData*)),this, SLOT(changementTitre(MetaData*)));
    connect(s,SIGNAL(listeChanged()),this, SLOT(changementPlaylist()));
    connect(s,SIGNAL(modeChanged()),this, SLOT(changementMode()));
    connect(s,SIGNAL(etatChanged(bool)),this, SLOT(changementEtat(bool)));
    connect(&ArtWorkService::getInstance(), SIGNAL(artFound(QString,QString)),
            this, SLOT(changementPochette(QString,QString)));
}

/**
 * @brief MprisLecteur::Next
 */
void  MprisLecteur::Next(){
    qDebug() << "!!! MprisLecteur::Next";

    service->suivant();
}

/**
 * @brief MprisLecteur::Previous
 */
void MprisLecteur:: Previous(){
    qDebug() << "!!! MprisLecteur::Previous";

    service->precedent();
}

/**
 * @brief MprisLecteur::Pause
 */
void  MprisLecteur::Pause(){
    qDebug() << "!!! MprisLecteur::Pause";

    if( service->getEtat()==false) {
        return;
    }

    service->lire();
}

/**
 * @brief MprisLecteur::PlayPause
 */
void  MprisLecteur::PlayPause(){
    qDebug() << "!!! MprisLecteur::PlayPause";

    service->lire();
}

/**
 * @brief MprisLecteur::Stop
 */
void  MprisLecteur::Stop(){
    qDebug() << "!!! MprisLecteur::Stop";

    service->stopper();
}

/**
 * @brief MprisLecteur::Play
 */
void  MprisLecteur::Play(){
    qDebug() << "!!! MprisLecteur::Play";
    if( service->getEtat() == true) {
        return;
    }

    service->lire();
}

/**
 * @brief MprisLecteur::Seek
 * @param offset in microseconds (relative)
 */
void MprisLecteur::Seek(qlonglong offset) {
    if (!service || !service->media) {
        return;
    }

    qint64 currentMs = service->getPositionMs();
    qint64 durationMs = service->media->duration();
    if (durationMs <= 0) {
        MetaData *t = service->getTitre();
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
 * @brief MprisLecteur::SetPosition
 * @param TrackId
 * @param Position in microseconds (absolute)
 */
void MprisLecteur::SetPosition(const QDBusObjectPath &TrackId, qlonglong Position) {
    if (!service || !service->media) {
        return;
    }

    MetaData *t = service->getTitre();
    if (!t) {
        return;
    }

    QString expectedPath = QString("/org/mpris/MediaPlayer2/muuzik/%1").arg(qAbs(t->getUuid()));
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

void  MprisLecteur::OpenUri(QString uri){
    Q_UNUSED(uri);
}

/**
 * @brief MprisLecteur::getPlaybackStatus
 * @return
 */
QString MprisLecteur::getPlaybackStatus() {
    if (service->getEtat()==true) {
        return "Playing";
    }

    if(service->getTaille()==0) {
        return "Stopped";
    }

    return "Paused";
}

/**
 * @brief MprisLecteur::getLoopStatus
 * @return
 */
QString MprisLecteur::getLoopStatus() {
    if(service->getBoucle()) {
        return "Playlist";
    }
    return "None";
}

/**
 * @brief MprisLecteur::setLoopStatus
 * @param v
 */
void  MprisLecteur::setLoopStatus(QString v) {
    if( v.compare("None",Qt::CaseInsensitive)==0) {
        if(service->getBoucle()==true) {
            service->switchBoucle();
        }
        return;
    }

    if(v.compare("Playlist",Qt::CaseInsensitive)==0 || v.compare("Track",Qt::CaseInsensitive)==0) {
        if(service->getBoucle()==false) {
            service->switchBoucle();
        }
    }
}

/**
 * @brief MprisLecteur::getShuffle
 * @return
 */
bool MprisLecteur::getShuffle() {
    return service->getAleatoire();
}

void MprisLecteur::setShuffle(bool v) {
    if(service->getAleatoire() == v) {
        return;
    }

    service->switchAleatoire();
}

/**
 * @brief MprisLecteur::getPosition
 * @return position in microseconds
 */
qlonglong MprisLecteur::getPosition() {
    if (!service) {
        return 0;
    }
    return static_cast<qlonglong>(service->getPositionMs()) * 1000LL;
}

/**
 * @brief MprisLecteur::getMetadata
 * @return
 */
QVariantMap MprisLecteur::getMetadata() {
    return metadata;
}

double MprisLecteur::getVolume() {
    double m = VolumeProxy::getInstance().getStepCount();
    if(m==0) {
        return 0;
    }

    double v = VolumeProxy::getInstance().getCurrentStep();

    return v/m;
}

void MprisLecteur::setVolume(double v) {
    double m = VolumeProxy::getInstance().getStepCount();
    if(m==0) {
        return ;
    }

    VolumeProxy::getInstance().setCurrentStep(v*m);
}

/**
 * @brief MprisLecteur::getCanGoNext
 * @return
 */
bool MprisLecteur::getCanGoNext() {
    return service->aSuivant();
}

/**
 * @brief MprisLecteur::getCanGoPrevious
 * @return
 */
bool MprisLecteur::getCanGoPrevious() {
    return service->aPrecedent();
}

/**
 * @brief MprisLecteur::getCanPlay
 * @return
 */
bool MprisLecteur::getCanPlay() {
    return service->getTaille()>0;
}

/**
 * @brief MprisLecteur::buildMetadata
 * @param t
 */
void MprisLecteur::buildMetadata(MetaData *t) {
    metadata.clear();
    currentTrackChemin.clear();

    if( t != NULL ) {
        currentTrackChemin = t->getChemin();

        QString trackPath = QString("/org/mpris/MediaPlayer2/muuzik/%1").arg(qAbs(t->getUuid()));
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

        if(!t->getTitre().isEmpty()) {
            metadata.insert("xesam:title", t->getTitre());
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

/**
 * @brief MprisLecteur::changementTitre
 * @param t
 */
void MprisLecteur::changementTitre(MetaData *t) {
    buildMetadata(t);

    QVariantMap p;
    p.insert("Metadata", metadata);
    p.insert("CanGoNext", getCanGoNext());
    p.insert("CanGoPrevious", getCanGoPrevious());

    sendDBusProperty(p);
}

/**
 * @brief MprisLecteur::changementDuree
 * @param durationMs
 */
void MprisLecteur::changementDuree(qint64 durationMs) {
    if (durationMs <= 0 || metadata.isEmpty()) {
        return;
    }

    qlonglong newLengthUs = static_cast<qlonglong>(durationMs) * 1000LL;
    if (!metadata.contains("mpris:length") || metadata.value("mpris:length").toLongLong() <= 0) {
        metadata.insert("mpris:length", QVariant::fromValue(newLengthUs));
        QVariantMap p;
        p.insert("Metadata", metadata);
        sendDBusProperty(p);
    }
}

/**
 * @brief MprisLecteur::changementPochette
 * @param vurl
 * @param artPath
 */
void MprisLecteur::changementPochette(const QString &vurl, const QString &artPath) {
    if (artPath.isEmpty() || currentTrackChemin.isEmpty() || metadata.isEmpty()) {
        return;
    }

    if (vurl == currentTrackChemin || VFI.getParentVUrl(currentTrackChemin) == vurl) {
        QString fileUrl = QUrl::fromLocalFile(artPath).toString();
        if (metadata.value("mpris:artUrl").toString() != fileUrl) {
            metadata.insert("mpris:artUrl", fileUrl);
            QVariantMap p;
            p.insert("Metadata", metadata);
            sendDBusProperty(p);
        }
    }
}

/**
 * @brief MprisLecteur::changementPlaylist
 */
void MprisLecteur::changementPlaylist() {

    QVariantMap p;

    quint64 t=service->getTaille();

    if( t!=taille) {
        taille=t;
        bool b=getCanPlay();
        p.insert("CanPlay", b);
        p.insert("CanPause", b);
        p.insert("CanSeek", b);
    }

    p.insert("CanGoNext", getCanGoNext());
    p.insert("CanGoPrevious", getCanGoPrevious());

    sendDBusProperty(p);
}

/**
 * @brief MprisLecteur::changementEtat
 * @param s
 */
void MprisLecteur::changementEtat(bool s) {
    Q_UNUSED(s);
    QVariantMap p;
    p.insert("PlaybackStatus", getPlaybackStatus());

    MetaData *t = service->getTitre();
    if (t != NULL && metadata.isEmpty()) {
        buildMetadata(t);
        p.insert("Metadata", metadata);
    }

    sendDBusProperty(p);
}

/**
 * @brief MprisLecteur::changementMode
 */
void MprisLecteur::changementMode() {
    QVariantMap p;
    p.insert("Shuffle", getShuffle());
    p.insert("LoopStatus", getLoopStatus());
    sendDBusProperty(p);
}

/**
 * @brief MprisLecteur::sendDBusProperty
 * @param value
 */
void MprisLecteur::sendDBusProperty(const QVariantMap value) {
    if (value.isEmpty()) {
        return;
    }
    QDBusMessage signal = QDBusMessage::createSignal(MPRIS_OBJ,FREEDESKTOP_PROPERTIES,"PropertiesChanged" );
    signal << MPRIS_PLAYER_ID;
    signal << value;
    signal << QStringList();
    QDBusConnection::sessionBus().send(signal);
}


