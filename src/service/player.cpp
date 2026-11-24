#include "player.h"
#include <qregularexpression.h>
#include <QDir>
#include <QtAlgorithms>
#include <QFuture>
#include <QtConcurrent/QtConcurrentRun>
#include <qdebug.h>
#include <QStack>
#include <QStandardPaths>
#include <QCoreApplication>
#include <QQmlEngine>
#include <QMetaObject>
#include <QMediaMetaData>
#include <QFile>
#include <QFileInfo>
#include <QThread>

#include <taglib/fileref.h>
#include <taglib/audioproperties.h>

#include "config.h"
#include "../app.h"
#include "playlist.h"
#include "historique.h"
#include "metadataservice.h"

#include "../vi18n/vi18n.h"
#include "../virtualfiles/VFservice.h"
#include "../util/webradioutil.h"

#include <nemonotifications-qt5/notification.h>

#define MAX_TAILLE_PLAYLIST 250

namespace {

QString metaVariantToString(const QVariant &val) {
    if (!val.isValid() || val.isNull()) {
        return QString();
    }
    if (val.type() == QVariant::StringList) {
        QStringList cleaned;
        foreach (const QString &item, val.toStringList()) {
            const QString trimmed = item.trimmed();
            if (!trimmed.isEmpty()) {
                cleaned.append(trimmed);
            }
        }
        return cleaned.join(QStringLiteral(", "));
    }
    if (val.type() == QVariant::List) {
        QStringList cleaned;
        foreach (const QVariant &item, val.toList()) {
            const QString trimmed = item.toString().trimmed();
            if (!trimmed.isEmpty()) {
                cleaned.append(trimmed);
            }
        }
        return cleaned.join(QStringLiteral(", "));
    }
    return val.toString().trimmed();
}

QString firstMetaString(const QMediaPlayer *player, const QStringList &keys) {
    if (!player) {
        return QString();
    }
    foreach (const QString &key, keys) {
        const QString value = metaVariantToString(player->metaData(key));
        if (!value.isEmpty()) {
            return value;
        }
    }
    return QString();
}

int parseBitrateKbps(const QMediaPlayer *player, const QStringList &keys) {
    if (!player) {
        return 0;
    }
    foreach (const QString &key, keys) {
        const QVariant val = player->metaData(key);
        if (!val.isValid() || val.isNull()) {
            continue;
        }
        bool ok = false;
        const double raw = val.toDouble(&ok);
        if (ok && raw > 0.0) {
            return (raw >= 1000.0) ? qRound(raw / 1000.0) : qRound(raw);
        }
    }
    return 0;
}

void splitIcyArtistTitle(const QString &rawTitle, QString &outArtist, QString &outTitle) {
    const QString trimmed = rawTitle.trimmed();
    outArtist.clear();
    outTitle = trimmed;
    if (trimmed.isEmpty()) {
        return;
    }

    const QStringList separators = QStringList()
            << QStringLiteral(" - ")
            << QStringLiteral(" \u2013 ")
            << QStringLiteral(" \u2014 ");
    foreach (const QString &sep, separators) {
        const int idx = trimmed.indexOf(sep);
        if (idx > 0) {
            const QString left = trimmed.left(idx).trimmed();
            const QString right = trimmed.mid(idx + sep.length()).trimmed();
            if (!left.isEmpty() && !right.isEmpty()) {
                outArtist = left;
                outTitle = right;
                return;
            }
        }
    }
}

QString fallbackCodecFromSuffix(const QString &filePath) {
    const QString ext = QFileInfo(filePath).suffix().trimmed().toLower();
    if (ext == QLatin1String("mp3")) return QStringLiteral("MP3");
    if (ext == QLatin1String("flac")) return QStringLiteral("FLAC");
    if (ext == QLatin1String("opus")) return QStringLiteral("Opus");
    if (ext == QLatin1String("ogg") || ext == QLatin1String("oga")) return QStringLiteral("Vorbis");
    if (ext == QLatin1String("aac")) return QStringLiteral("AAC");
    if (ext == QLatin1String("m4a") || ext == QLatin1String("m4b")) return QStringLiteral("M4A");
    if (ext == QLatin1String("wav")) return QStringLiteral("PCM");
    if (ext == QLatin1String("aif") || ext == QLatin1String("aiff")) return QStringLiteral("AIFF");
    if (ext == QLatin1String("alac")) return QStringLiteral("ALAC");
    return QString();
}

} // namespace

/**
 * @brief playerService::playerService
 * @param parent
 */
playerService::playerService(QObject *parent)
    : QObject(parent)
    , media(0)
    , playlist(0)
    , dureeTotale(0)
    , sessionRestaure(false)
    , sessionRestaurePlaylist(false)
    , raf(false)
    , pendingRestorePositionMs(-1)
    , lastCurrentIndex(-1)
    , navigatingHistory(false)
    , settings(QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation)
               + "/" + QCoreApplication::applicationName() + ".conf",
               QSettings::NativeFormat)
    , currentAudioBitrate(0) {

    // le "chargeur" de titre
    connect(this,
            SIGNAL(chargementFaitIntr(QList<MetaData*>*,qint64,bool)),
            this,
            SLOT(finChargementImp(QList<MetaData*>*,qint64,bool)),
            Qt::QueuedConnection
            );

    // le lecteur
    media = new QMediaPlayer(this);
    playlist = new QMediaPlaylist(this);
    media->setPlaylist(playlist);
    connect(media, SIGNAL(stateChanged(QMediaPlayer::State)), SLOT(changementEtat(QMediaPlayer::State)));
    connect(playlist, SIGNAL(currentIndexChanged(int)), SLOT(changementIndex(int)));
    connect(media, SIGNAL(positionChanged(qint64)), SLOT(changementPosition(qint64)));
    connect(media, SIGNAL(durationChanged(qint64)), SLOT(changementDureeMedia(qint64)));
    connect(media, SIGNAL(mediaStatusChanged(QMediaPlayer::MediaStatus)), SLOT(changementMediaStatus(QMediaPlayer::MediaStatus)));
    connect(media, SIGNAL(seekableChanged(bool)), SLOT(changementSeekable(bool)));
    connect(media, SIGNAL(metaDataChanged()), SLOT(changementMetaData()));
    connect(media, SIGNAL(metaDataAvailableChanged(bool)), SLOT(changementMetaData()));

    // dbus
    mpris = new Mpris(this);
    mprisplayer = new MprisPlayer(this);

    QDBusConnection session = QDBusConnection::sessionBus();
    session.registerObject(MPRIS_OBJ, this);
    mprisServiceName = QStringLiteral(MPRIS_SERVICE_ID);
    if (!session.registerService(mprisServiceName)) {
        QString instanceName = QString("%1.instance%2")
                .arg(MPRIS_SERVICE_ID)
                .arg(QCoreApplication::applicationPid());
        if (session.registerService(instanceName)) {
            mprisServiceName = instanceName;
        } else {
            mprisServiceName.clear();
        }
    }

    // nemo.voicecall
    session.connect("",
                    "/calls/active",
                    "org.nemomobile.voicecall.VoiceCall",
                    "lineIdChanged",
                    this, SLOT(appelRecu(const QDBusMessage&)));


    // Even if we didn't find any service, we still listen for metadataChanged signals
    // from every MPRIS-compatible player
    // If such a signal comes in, we will connect to the source service for that signal
    //    session .connect("", "/org/mpris/MediaPlayer2", "org.freedesktop.DBus.Properties", "PropertiesChanged",
    // this, SLOT(handleMprisPropertiesChanged(QString,QMap<QString,QVariant>,QStringList)));

    // les controles plein ecran
    this->showControl = configService::getInstance().getControle();
    connect(&configService::getInstance(), SIGNAL(controleChanged()), this, SLOT(ControlChanged_impl()));
    connect(&VFService::getInstance(), SIGNAL(moved(QString,QString)), this, SLOT(onMoved(QString,QString)));
    connect(&VFService::getInstance(), SIGNAL(renamed(QString,QString)), this, SLOT(onRenamed(QString,QString)));

    // les services
    connect(&VFService::getInstance(), SIGNAL(removed(QString)),this, SLOT(onRemoved(QString)));

}

/**
 * @brief playerService::arreter
 */
void playerService::arreter() {
    if(!media && !playlist) {
        return;
    }

    if(media) {
        media->stop();
        media->disconnect();
        media->setPlaylist(0);
    }
    if(playlist) {
        playlist->clear();
        playlist->disconnect();
    }

    QDBusConnection session = QDBusConnection::sessionBus();
    if (!mprisServiceName.isEmpty()) {
        session.unregisterService(mprisServiceName);
        mprisServiceName.clear();
    }
    session.unregisterObject(MPRIS_OBJ);

    session.disconnect("",
                       "/calls/active",
                       "org.nemomobile.voicecall.VoiceCall",
                       "lineIdChanged",
                       this, SLOT(appelRecu(const QDBusMessage&)));

    delete media;
    media = 0;
    delete playlist;
    playlist = 0;

    liste.clear();
    playHistory.clear();
    lastCurrentIndex = -1;
    navigatingHistory = false;
    resetStreamMetaData();
}

/**
 * @brief playerService::~playerService
 */
playerService::~playerService() {
    arreter();
}

/**
 * @brief playerService::getInstance
 * @return
 */
playerService& playerService::getInstance() {
    static playerService instance;
    return instance;
}

/**
 * @brief playerService::init
 * @param context
 * @param name
 */
void playerService::init(QQmlContext *context,QString name) {
    context->setContextProperty(name, &playerService::getInstance());
}

/**
 * @brief ajouter à la liste de lecture
 * si un audio, ajoute un seul fichier, si rep, ajoute tout les fichiers audio du rep
 * @param vurl la liste des vurl vers des "albums" ou des titres à ajouter
 * @param recursif : si true, ajouter les média récursivement
 */
void playerService::add(const QList<QString> &vurl, bool recursif) {
    if(raf==true) {
        return; // scan déjà en cours
    }

    raf=true;
    emit enCoursChanged();

    // lancement du scan
    QFuture<void> f = QtConcurrent::run(doChargement,vurl,recursif,-1,false);
}

/**
 * @brief remplace à la liste de lecture
 * si un audio, ajoute un seul fichier, si rep, ajoute tout les fichiers audio du rep
 * @param vurl la liste des vurl vers des "albums" ou des titres à ajouter
 * @param recursif : si true, ajouter les média récursivement
 */
void playerService::replace(const QList<QString> &vurl, bool recursif) {
    if(raf==true) {
        return; // scan déjà en cours
    }

    raf=true;
    pendingRestorePositionMs = -1;
    emit enCoursChanged();

    // arrête le player et vide la liste
    if (media) media->stop();
    if (playlist) playlist->clear();
    liste.clear();
    playHistory.clear();
    lastCurrentIndex = -1;
    navigatingHistory = false;
    dureeTotale=0;
    resetStreamMetaData();
    emit listChanged();
    emit titleChanged(NULL);
    emit statusChanged(false);

    bool autoplay = configService::getInstance().getAutoStart() && (!media || media->state() != QMediaPlayer::PlayingState) ;

    // lancement du scan
    QFuture<void> f = QtConcurrent::run(doChargement,vurl,recursif,0,autoplay);
}

/**
 * @brief créé la liste de lecture à ajouter au player
 * la liste sera retourné au service par le signal chargementFaitIntr
 * @param vurl liste des sources : album (rep) ou audio (file)
 * @param recursif: true si doit passer les rep recursivement
 * @param position : utiliser lors de la restauration d'une session pour le titre a démmarrer
 * si -1 : n'est pas pris en compte
 */
void playerService::doChargement(const QList<QString> &vurls, bool recursif, qint64 position, bool autoplay) {

    QStringList resolvedVurls;
    foreach (QString source, vurls) {
        if (WebRadioUtil::isStreamUrl(source)) {
            resolvedVurls.append(source.trimmed());
            continue;
        }

        if(VFI.exist(source)==false) {
            continue;
        }

        // on n'ajoute à l'historique que les rep/album racines
        if( VFI.isDirectory(source)) {
            // on ajoute pas à l'historique si on est ne train de faire une restauration
            if(playerService::getInstance().isSessionRestaurePlaylist()==false) {
                QMetaObject::invokeMethod(&HistoriqueService::getInstance(),
                                          "ajouter",
                                          Qt::QueuedConnection,
                                          Q_ARG(QString, source));
            }

            QList<VFInfo *> res = VFI.getSynch(source, VFService::FILTERAUDIO,
                                               recursif ? VFService::RECURSIVE : VFService::FILE, VFService::URI );
            foreach (VFInfo *info, res) {
                if (info) {
                    resolvedVurls.append(info->getVUrl());
                }
            }
            qDeleteAll(res);
            continue;
        }

        // un fichier (audio) ajoute
        VFInfo *singleInfo = VFI.getInfo(source);
        if (singleInfo) {
            resolvedVurls.append(singleInfo->getVUrl());
            delete singleInfo;
        }
    }

    QList<MetaData *> *ll = new QList<MetaData *>();
    foreach(const QString &itemVurl, resolvedVurls) {
        MetaData *meta = MetaDataService::getInstance().getMetaData(itemVurl);
        if (!meta) {
            continue;
        }
        ll->append(meta);

        if(ll->length()>MAX_TAILLE_PLAYLIST) {
            QMetaObject::invokeMethod(&playerService::getInstance(),
                                      "notifierTailleMax",
                                      Qt::QueuedConnection);
            break;
        }
    }

    // on calcule la position dans la liste
    // @NOTA @FIXME si vurls est vide, le .at lance une exception sur qlist.h mais
    // elle n'est pas catché !!??
    if((position != -1) && (ll->length()>0)) {
        position = position >= vurls.length() ? vurls.length()-1 : position;

        try {
            QString str = vurls.at(position);
            if (position < ll->length() && ll->at(position)->getChemin().compare(str) == 0) {
                // Exact index preserved
            } else {
                qint64 index = 0;
                bool found = false;

                foreach(MetaData *m, *ll) {
                    if(m->getChemin().compare(str)==0) {
                        found = true;
                        break;
                    }
                    index++;
                }
                position = found ? index : -1;
            }

        } catch(const std::exception & e) {
            qDebug()<< "on the exception";
        }
    }

    // ok, passe le resutat au service par un signal/slot
    emit playerService::getInstance().chargementFaitIntr(ll,position,autoplay);
}

void playerService::notifierTailleMax() {
    Notification n;
    n.setPreviewBody(vI18n::getInstance().get( "lecteur.taillemax" ));
    n.setHintValue("x-nemo-icon", "image://theme/icon-s-high-importance");
    n.publish();
}

/**
 * @brief la liste de lecture est créé et disponible pour affichage
 * @param ll
 * @param pls
 * @param position
 */
void playerService::finChargementImp(QList<MetaData *>*ll, const qint64 position,bool autoplay) {

    bool restoring = sessionRestaurePlaylist;

    raf=false;
    emit  enCoursChanged();

    QList<MetaData *> validTracks;
    foreach (MetaData *t, *ll) {
        if (t) {
            QQmlEngine::setObjectOwnership(t, QQmlEngine::CppOwnership);
            validTracks.append(t);
        }
    }

    if(validTracks.isEmpty()) {
        sessionRestaurePlaylist=false;
        pendingRestorePositionMs = -1;
        emit listChanged();
        emit titleChanged(NULL);
        emit statusChanged(false);
        delete ll;
        return;
    }

    liste.append(validTracks);

    dureeTotale = 0;
    foreach (MetaData *t, liste) {
        if (t) {
            dureeTotale += t->getDuree();
        }
    }

    if(playlist) {
        playlist->addMedia( MetaData::versMediaContent(&validTracks) );
    }

    // debut de liste : ne peut venir que d'une restauration
    if(position > -1 && playlist) {
        int p = position < playlist->mediaCount() ? static_cast<int>(position) : 0;
        playlist->setCurrentIndex(p);
    } else {
        if (restoring) {
            pendingRestorePositionMs = -1;
        }
        if(playlist && playlist->currentIndex() == -1 && playlist->mediaCount() > 0) {
            playlist->setCurrentIndex(0);
        }
    }

    // on s'assure de ne plus être dans le cas d'une restauration (pas de play, pas d'historique)
    sessionRestaurePlaylist=false;

    if (restoring && pendingRestorePositionMs > 0) {
        MetaData *t = getTitle();
        if (t && t->getDuree() > 0 && pendingRestorePositionMs > static_cast<qint64>(t->getDuree()) * 1000LL) {
            pendingRestorePositionMs = static_cast<qint64>(t->getDuree()) * 1000LL;
        }
    } else if (!restoring) {
        pendingRestorePositionMs = -1;
    }

    emit listChanged();

    if (restoring && pendingRestorePositionMs > 0) {
        emit dureeChanged(pendingRestorePositionMs / 1000);
        applyPendingRestorePosition();
    } else if (!restoring) {
        persistSession();
    }

    if(autoplay) {
        play();
    }

    delete ll;
}

/**
  * recharge la precedente liste de lecture
  */
void playerService::restoreSession() {
    if(sessionRestaure==true) {
        return;
    }
    sessionRestaure=true;


    QList<QString> vurl = settings.value(LECTURE_LISTE).value<QList<QString> >();
    if(vurl.size()==0) {
        return;
    }


    int pos = settings.value(LECTURE_LISTE_INDEX, 0).toInt();
    if (pos < 0) {
        pos = 0;
    }

    qint64 savedPosMs = settings.value(LECTURE_POSITION, 0).toLongLong();
    pendingRestorePositionMs = savedPosMs > 0 ? savedPosMs : -1;

    nom = settings.value(LECTURE_NOM).toString();
    int savedMode = settings.value(LECTURE_MODE, static_cast<int>(QMediaPlaylist::Sequential)).toInt();
    if (playlist && (savedMode == QMediaPlaylist::Sequential ||
                     savedMode == QMediaPlaylist::Random ||
                     savedMode == QMediaPlaylist::Loop)) {
        playlist->setPlaybackMode(static_cast<QMediaPlaylist::PlaybackMode>(savedMode));
        emit modeChanged();
    }

    raf=true;
    emit enCoursChanged();

    // lancement du scan
    sessionRestaurePlaylist=true;   // on recharger depuis la session, pas de lecture automatique
    QFuture<void> f = QtConcurrent::run(doChargement, vurl, false, pos, false);
}

/**
 * @brief Persiste l'état de la file de lecture et la position du titre courant
 */
void playerService::persistSession() {
    if (!sessionRestaure || !media || !playlist || raf) {
        return;
    }

    QList<QString> l;
    int count = 0;
    foreach (MetaData *t, liste) {
        if (!t) {
            continue;
        }
        l.append(t->getChemin());
        if (++count >= MAX_TAILLE_PLAYLIST) {
            break;
        }
    }

    settings.setValue(LECTURE_LISTE, QVariant::fromValue(l));
    int idx = playlist->currentIndex();
    settings.setValue(LECTURE_LISTE_INDEX, idx >= 0 ? idx : 0);
    settings.setValue(LECTURE_POSITION, l.isEmpty() ? 0LL : getPositionMs());
    settings.setValue(LECTURE_MODE, static_cast<int>(playlist->playbackMode()));
    settings.setValue(LECTURE_NOM, nom);
    settings.sync();
}

/**
 * @brief playerService::saveSession
 */
void playerService::saveSession() {
    qDebug() << "saveSession start";
    persistSession();
    qDebug() << "saveSession stop";

    arreter();
}

/**
 * @brief Applique la position restaurée dès que le média est prêt et seekable
 */
void playerService::applyPendingRestorePosition() {
    if (pendingRestorePositionMs <= 0 || !media) {
        return;
    }
    if (media->state() == QMediaPlayer::StoppedState || !media->isSeekable()) {
        return;
    }

    qint64 dur = media->duration();
    if (dur <= 0) {
        return;
    }

    qint64 target = qMin(pendingRestorePositionMs, dur);
    pendingRestorePositionMs = -1;
    media->setPosition(target);
    emit dureeChanged(target / 1000);
    if (mprisplayer) {
        emit mprisplayer->Seeked(static_cast<qlonglong>(target) * 1000LL);
    }
}

qint64 playerService::getPositionMs() const {
    if (pendingRestorePositionMs > 0) {
        return pendingRestorePositionMs;
    }
    return media ? media->position() : 0;
}

int playerService::getPosition() const {
    return static_cast<int>(getPositionMs() / 1000);
}

/**
 * @brief retire le titre qui a l'uuid uuid
 * @param uuid
 */
void playerService::retirerUuid(qint64 uuid) {
    qint64 index = trouverIndex(uuid);
    if(index < 0 || index >= liste.size()) {
        return;
    }

    int currentIdx = playlist ? playlist->currentIndex() : -1;
    bool removedCurrent = (index == currentIdx);

    MetaData *removed = liste.takeAt(index);
    if (removed) {
        dureeTotale = qMax<qint64>(0, dureeTotale - removed->getDuree());
    }

    if(playlist && index < playlist->mediaCount()) {
        playlist->removeMedia(index);
    }

    playHistory.clear();
    lastCurrentIndex = playlist ? playlist->currentIndex() : -1;

    persistSession();
    emit listChanged();
    if (removedCurrent) {
        emit titleChanged(getTitle());
    }
}

/**
 * @brief deplace vers le haut (debut de liste) le titre à l'index index
 * @param position
 */
void playerService::monterUuid(const qint64 uuid) {
    qint64 position = trouverIndex(uuid);
    if(position <= 0 || position >= liste.size() || !playlist || playlist->mediaCount() != liste.size()) {
        return;
    }

    liste.swap(position, position-1);

    if(position==playlist->currentIndex()) {
        QMediaContent m = playlist->media(position-1);
        playlist->insertMedia(position+1, m);
        playlist->removeMedia(position-1);
        persistSession();
        emit listChanged();
        return;
    }

    QMediaContent m = playlist->media(position);
    playlist->insertMedia(position-1, m);
    playlist->removeMedia(position+1);
    persistSession();
    emit listChanged();
}

/**
 * @brief deplace vers le bas le titre à l'index index
 * @param index
 */
void playerService::descendreUuid(const qint64 uuid) {
    qint64 position = trouverIndex(uuid);
    if(position < 0 || position + 1 >= liste.size() || !playlist || playlist->mediaCount() != liste.size()) {
        return;
    }

    liste.swap(position, position+1);

    if(position==playlist->currentIndex()) {
        QMediaContent m = playlist->media(position+1);
        playlist->insertMedia(position, m);
        playlist->removeMedia(position+2);
        persistSession();
        emit listChanged();
        return;
    }

    QMediaContent m = playlist->media(position);
    playlist->insertMedia(position+2, m);
    playlist->removeMedia(position);
    persistSession();
    emit listChanged();
}

/**
 * @brief retourne le titre en cours de lecture ou null si aucun
 * @return
 */
MetaData *playerService::getTitle() const {
    if(!playlist || playlist->mediaCount()<=0) {
        return NULL;
    }
    int idx = playlist->currentIndex();
    if( idx < 0 || idx >= liste.size()) {
        return NULL;
    }

    MetaData *t = liste.at(idx);
    if (t) {
        QQmlEngine::setObjectOwnership(t, QQmlEngine::CppOwnership);
    }
    return t;
}

/**
 * @brief effacer la liste et eventuellement arreter la lecture
 * @return
 */
void playerService::effacer() {
    if(media && media->state() != QMediaPlayer::StoppedState) {
        stop();
    }

    if(playlist) {
        playlist->clear();
    }
    liste.clear();
    playHistory.clear();
    lastCurrentIndex = -1;
    navigatingHistory = false;
    dureeTotale=0;
    nom=QString();
    pendingRestorePositionMs = -1;
    resetStreamMetaData();

    persistSession();
    emit listChanged();
    emit titleChanged(NULL);
    emit statusChanged(false);
}

/**
 * @brief lire ou mettre en pause
 */
void playerService::play(qint64 index) {
    if(!media || !playlist || playlist->mediaCount()<=0) {
        return;
    }

    if( index > playlist->mediaCount()-1) {
        return;
    }

    if( index >=0) {
        pendingRestorePositionMs = -1;
        playlist->setCurrentIndex(index);
        media->play();
        return;
    }

    if( media->state() == QMediaPlayer::PlayingState) {
        media->pause();
        return;
    }

    if (playlist->currentIndex() < 0) {
        playlist->setCurrentIndex(0);
    }

    media->play();
}

/**
 * @brief mettre en pause la lecture si en cours
 */
void playerService::pause() {
    if (!media || media->state() != QMediaPlayer::PlayingState) {
        return;
    }
    media->pause();
}

/**
 * @brief stopper la lecture
 */
void playerService::stop() {
    if (media) {
        media->stop();
    }
    //@fix utile ou deja fait par le signal positionChanged ???
    //dureetitre=0;
    // emit dureeChanged(dureetitre);
}

/**
* @brief passe à la piste suivant si possible.
*/
void playerService::next() {
    if(!playlist || playlist->nextIndex() == -1) {
        return;
    }
    pendingRestorePositionMs = -1;
    playlist->next();
    if (media) {
        media->play();
    }
}

/**
* @brief passe à la piste precedente si possible.
*/
void playerService::previous() {
    if(!playlist || playlist->mediaCount() <= 0) {
        return;
    }

    pendingRestorePositionMs = -1;

    if (playlist->playbackMode() == QMediaPlaylist::Random) {
        int prevIdx = -1;
        while (!playHistory.isEmpty()) {
            int candidate = playHistory.takeLast();
            if (candidate >= 0 && candidate < playlist->mediaCount() && candidate != playlist->currentIndex()) {
                prevIdx = candidate;
                break;
            }
        }
        if (prevIdx == -1 && playlist->mediaCount() > 1) {
            int cur = playlist->currentIndex();
            prevIdx = (cur > 0) ? (cur - 1) : (playlist->mediaCount() - 1);
        }
        if (prevIdx >= 0) {
            navigatingHistory = true;
            playlist->setCurrentIndex(prevIdx);
            navigatingHistory = false;
        } else if (media) {
            media->setPosition(0);
            emit dureeChanged(0);
            if (mprisplayer) {
                emit mprisplayer->Seeked(0);
            }
        }
    } else if (playlist->previousIndex() != -1) {
        navigatingHistory = true;
        playlist->previous();
        navigatingHistory = false;
    } else if (media) {
        media->setPosition(0);
        emit dureeChanged(0);
        if (mprisplayer) {
            emit mprisplayer->Seeked(0);
        }
    }

    if (media) {
        media->play();
    }
}

/**
 * @brief avance la position de la lecture dans le morceau en cours
 */
void playerService::avancer(qint64 pos) {
    if (!media) return;
    qint64 posMs = pos * 1000LL;
    if (posMs < 0) {
        return;
    }

    if (media->state() == QMediaPlayer::StoppedState && media->duration() <= 0) {
        MetaData *t = getTitle();
        qint64 maxMs = t ? static_cast<qint64>(t->getDuree()) * 1000LL : 0;
        if (maxMs > 0 && posMs > maxMs) {
            posMs = maxMs;
        }
        pendingRestorePositionMs = posMs;
        emit dureeChanged(posMs / 1000);
        if (mprisplayer) {
            emit mprisplayer->Seeked(static_cast<qlonglong>(posMs) * 1000LL);
        }
        persistSession();
        return;
    }

    if (posMs > media->duration()) {
        return;
    }

    pendingRestorePositionMs = -1;
    media->setPosition(posMs);
    if (mprisplayer) {
        emit mprisplayer->Seeked(static_cast<qlonglong>(posMs) * 1000LL);
    }
    persistSession();
}

/**
  * @brief switchRandom
  * @param v
  */
void playerService::switchRandom() {
    if(!playlist) return;
    playlist->setPlaybackMode( playlist->playbackMode() != QMediaPlaylist::Random
            ? QMediaPlaylist::Random : QMediaPlaylist::Sequential);

    Notification n;
    n.setPreviewBody(vI18n::getInstance().get( playlist->playbackMode() == QMediaPlaylist::Random  ? "lecteur.aleatoire.on" :"lecteur.aleatoire.off") );
    n.setHintValue("x-nemo-icon", "image://theme/icon-s-attach");
    n.publish();

    persistSession();
    emit modeChanged();
}

/**
  * @brief switchLoop
  * @param v
  */
void playerService::switchLoop(){
    if(!playlist) return;
    playlist->setPlaybackMode( playlist->playbackMode() != QMediaPlaylist:: Loop
            ? QMediaPlaylist::Loop : QMediaPlaylist::Sequential);

    Notification n;
    n.setPreviewBody(vI18n::getInstance().get( playlist->playbackMode() == QMediaPlaylist::Loop  ? "lecteur.boucle.on" :"lecteur.boucle.off") );
    n.setHintValue("x-nemo-icon", "image://theme/icon-s-attach");
    n.publish();

    persistSession();
    emit modeChanged();
}

/**
 * @brief playerService::changementEtat
 * @param state
 */
void playerService::changementEtat(QMediaPlayer::State state) {
    emit statusChanged(state == QMediaPlayer::PlayingState);
    applyPendingRestorePosition();
    if (state == QMediaPlayer::PlayingState) {
        updateStreamMetaData(false);
    } else if (state == QMediaPlayer::PausedState || state == QMediaPlayer::StoppedState) {
        persistSession();
    }
}

/**
 * @brief playerService::changementPosition
 * @param position
 */
void playerService::changementPosition(qint64 positionMs) {
    if (pendingRestorePositionMs > 0) {
        return;
    }
    emit dureeChanged(positionMs / 1000);
    if (!lastSaveTimer.isValid() || lastSaveTimer.elapsed() >= 5000) {
        persistSession();
        lastSaveTimer.restart();
    }
}

/**
 * @brief playerService::changementDureeMedia
 * @param duration
 */
void playerService::changementDureeMedia(qint64 duration) {
    if (mprisplayer) {
        mprisplayer->changementDuree(duration);
    }
    applyPendingRestorePosition();
}

void playerService::changementMediaStatus(QMediaPlayer::MediaStatus status) {
    if (status == QMediaPlayer::InvalidMedia || status == QMediaPlayer::EndOfMedia) {
        pendingRestorePositionMs = -1;
        if (status == QMediaPlayer::InvalidMedia) {
            resetStreamMetaData();
        }
        return;
    }
    if (status == QMediaPlayer::LoadedMedia ||
            status == QMediaPlayer::BufferedMedia ||
            status == QMediaPlayer::BufferingMedia) {
        updateStreamMetaData(false);
    }
    applyPendingRestorePosition();
}

void playerService::changementSeekable(bool seekable) {
    if (seekable) {
        applyPendingRestorePosition();
    }
}

void playerService::changementMetaData() {
    updateStreamMetaData(false);
}

bool playerService::isCurrentStream() const {
    MetaData *track = getTitle();
    return track && WebRadioUtil::isStreamUrl(track->getChemin());
}

void playerService::resetStreamMetaData() {
    if (currentStreamArtist.isEmpty() &&
            currentStreamTitle.isEmpty() &&
            currentAudioCodec.isEmpty() &&
            currentAudioBitrate == 0) {
        return;
    }
    currentStreamArtist.clear();
    currentStreamTitle.clear();
    currentAudioCodec.clear();
    currentAudioBitrate = 0;
    emit streamMetaDataChanged();
}

void playerService::updateStreamMetaData(bool clearFirst) {
    MetaData *track = getTitle();
    if (!media || !track) {
        resetStreamMetaData();
        return;
    }

    const bool isStream = WebRadioUtil::isStreamUrl(track->getChemin());
    const bool metaAvailable = media->isMetaDataAvailable() || !media->availableMetaData().isEmpty();

    QString newArtist = clearFirst ? QString() : currentStreamArtist;
    QString newTitle = clearFirst ? QString() : currentStreamTitle;
    QString newCodec = clearFirst ? QString() : currentAudioCodec;
    int newBitrate = clearFirst ? 0 : currentAudioBitrate;

    if (metaAvailable) {
        const QStringList artistKeys = QStringList()
                << QMediaMetaData::ContributingArtist
                << QMediaMetaData::AlbumArtist
                << QMediaMetaData::Author
                << QStringLiteral("Artist")
                << QStringLiteral("artist");
        const QStringList titleKeys = QStringList()
                << QMediaMetaData::Title
                << QStringLiteral("Title")
                << QStringLiteral("title")
                << QStringLiteral("StreamTitle")
                << QStringLiteral("icy-title");
        const QStringList codecKeys = QStringList()
                << QMediaMetaData::AudioCodec
                << QStringLiteral("audio-codec")
                << QStringLiteral("AudioCodec")
                << QStringLiteral("codec")
                << QStringLiteral("container-format");
        const QStringList bitrateKeys = QStringList()
                << QMediaMetaData::AudioBitRate
                << QStringLiteral("nominal-bitrate")
                << QStringLiteral("AudioBitRate")
                << QStringLiteral("bitrate")
                << QStringLiteral("icy-br")
                << QStringLiteral("maximum-bitrate")
                << QStringLiteral("minimum-bitrate");

        QString rawArtist = firstMetaString(media, artistKeys);
        QString rawTitle = firstMetaString(media, titleKeys);

        if (isStream && rawArtist.isEmpty() && !rawTitle.isEmpty()) {
            QString parsedArtist;
            QString parsedTitle;
            splitIcyArtistTitle(rawTitle, parsedArtist, parsedTitle);
            rawArtist = parsedArtist;
            rawTitle = parsedTitle;
        }

        newArtist = rawArtist;
        newTitle = rawTitle;

        const QString detectedCodec = firstMetaString(media, codecKeys);
        if (!detectedCodec.isEmpty() || clearFirst) {
            newCodec = detectedCodec;
        }

        const int detectedBitrate = parseBitrateKbps(media, bitrateKeys);
        if (detectedBitrate > 0 || clearFirst) {
            newBitrate = detectedBitrate;
        }
    }

    if (!isStream && (newBitrate <= 0 || newCodec.isEmpty())) {
        const QString localPath = VFI.getPathFromVUrl(track->getChemin());
        if (!localPath.isEmpty()) {
            if (newCodec.isEmpty()) {
                newCodec = fallbackCodecFromSuffix(localPath);
            }
            if (newBitrate <= 0) {
                TagLib::FileRef fr(QFile::encodeName(localPath).constData(), true, TagLib::AudioProperties::Fast);
                if (!fr.isNull() && fr.audioProperties() != NULL && fr.audioProperties()->bitrate() > 0) {
                    newBitrate = fr.audioProperties()->bitrate();
                }
            }
        }
    }

    if (newArtist == currentStreamArtist &&
            newTitle == currentStreamTitle &&
            newCodec == currentAudioCodec &&
            newBitrate == currentAudioBitrate) {
        return;
    }

    currentStreamArtist = newArtist;
    currentStreamTitle = newTitle;
    currentAudioCodec = newCodec;
    currentAudioBitrate = newBitrate;
    emit streamMetaDataChanged();
}

/**
 * @brief playerService::changementIndex
 * @param position
 */
void playerService::changementIndex(int position) {
    if(position==-1) {
        pendingRestorePositionMs = -1;
        lastCurrentIndex = -1;
        resetStreamMetaData();
        if (media) media->stop();
        if (playlist && playlist->mediaCount() > 0) {
            playlist->setCurrentIndex(0);
        } else {
            emit titleChanged(NULL);
        }
        return;
    }

    if (!navigatingHistory && !sessionRestaurePlaylist &&
            lastCurrentIndex >= 0 && lastCurrentIndex != position &&
            playlist && lastCurrentIndex < playlist->mediaCount()) {
        playHistory.append(lastCurrentIndex);
        if (playHistory.size() > 100) {
            playHistory.removeFirst();
        }
    }
    lastCurrentIndex = position;

    if (!sessionRestaurePlaylist) {
        pendingRestorePositionMs = -1;
    }

    emit indexChanged(position);
    emit titleChanged(getTitle());
    updateStreamMetaData(true);

    if (!sessionRestaurePlaylist) {
        persistSession();
    }
}

/**
 * @brief parcours la liste et retourne l'index du titre qui a l'uuid ; -1 si trouve pas
 * @param uuid
 * @return
 */
qint64 playerService::trouverIndex( const qint64 uuid) const {
    qint64 index = 0;

    foreach (MetaData *t , liste) {
        if (t && t->getUuid()==uuid ) {
            return index;
        }
        index++;
    }

    return -1;
}

/**
 * @brief playerService::appelRecu
 * @param a
 */
void playerService::appelRecu(const QDBusMessage &a) {

    QDBusInterface direction("org.nemomobile.voicecall",
                             "/calls/active",
                             "org.nemomobile.voicecall.VoiceCall",
                             QDBusConnection::sessionBus());

    if(direction.property("isIncoming").toBool()==false) {
        return;
    }

    if( media && media->state() == QMediaPlayer::PlayingState) {
        media->pause();
    }
}

/**
 * @brief playerService::setShowControl
 * @param val
 */
void playerService::setShowControl(const bool val) {
    if( this->showControl == val) {
        return;
    }
    this->showControl = val;
    emit showControlChanged();
}

/**
 * @brief si change l'affichage des controle en plein ecran dans la config
 */
void playerService::ControlChanged_impl() {
    this->setShowControl(configService::getInstance().getControle());
}

/**
 * @brief playerService::handleMprisPropertiesChanged
 * @param interface
 * @param changed
 * @param invalidated
 */
void playerService::handleMprisPropertiesChanged(const QString &interface, const QMap<QString, QVariant> &changed, const QStringList &invalidated) {
    qDebug() << "handleMprisPropertiesChanged ! " << interface << " " ;
}

/**
 * @brief favorisService::onRemoved
 * @param vurl
 */
void playerService::onRemoved(QString vurl) {
    if (vurl.isEmpty()) {
        return;
    }

    QStack<qint64> stack;

    foreach (MetaData *m, liste) {
        if(m && m->getChemin().startsWith(vurl)) {
            stack.push(m->getUuid());
        }
    }

    // on supprime en lifo
    while( stack.isEmpty() == false) {
        qint64 val = stack.pop();

        retirerUuid(val);
    }
}

/**
 * @brief playerService::onMoved
 * @param vurl
 * @param as
 */
void playerService::onMoved(QString vurl, QString as) {
    if (vurl.isEmpty()) {
        return;
    }

    for (int i = 0 ; i < liste.size() ; i++) {
        MetaData *m = liste.at(i);
        if (!m) {
            continue;
        }

        if ( m->getChemin().startsWith(vurl) ) {
            QString target = m->getChemin().replace(vurl, as);
            m->setChemin(target);

            // media
            if (playlist) {
                playlist->removeMedia(i);
                playlist->insertMedia(i, MetaData::versMediaContent(m) );
            }
        }
    }
    persistSession();
}

/**
 * @brief playerService::onRenamed
 * @param vurl
 * @param name
 */
void playerService::onRenamed(QString vurl, QString name) {
    QString p = VFI.getParentVUrl(vurl);
    QString as  = p + (VFI.isRoot(p) ? "" : "/") + name;
    onMoved(vurl, as);
}

