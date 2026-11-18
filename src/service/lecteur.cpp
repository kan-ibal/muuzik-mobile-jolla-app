#include "lecteur.h"
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
#include <QThread>

#include "config.h"
#include "../app.h"
#include "playlist.h"
#include "historique.h"
#include "metadataservice.h"

#include "../vi18n/vi18n.h"
#include "../virtualfiles/VFservice.h"

#include <nemonotifications-qt5/notification.h>

#define MAX_TAILLE_PLAYLIST 150

/**
 * @brief lecteurService::lecteurService
 * @param parent
 */
lecteurService::lecteurService(QObject *parent)
    : QObject(parent)
    , media(0)
    , playlist(0)
    , dureeTotale(0)
    , sessionRestaure(false)
    , sessionRestaurePlaylist(false)
    , raf(false)
    , pendingRestorePositionMs(-1)
    , settings(QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation)
               + "/" + QCoreApplication::applicationName() + ".conf",
               QSettings::NativeFormat) {

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

    // dbus
    mpris = new Mpris(this);
    mprislecteur = new MprisLecteur(this);

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
 * @brief lecteurService::arreter
 */
void lecteurService::arreter() {
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
}

/**
 * @brief lecteurService::~lecteurService
 */
lecteurService::~lecteurService() {
    arreter();
}

/**
 * @brief lecteurService::getInstance
 * @return
 */
lecteurService& lecteurService::getInstance() {
    static lecteurService instance;
    return instance;
}

/**
 * @brief lecteurService::init
 * @param context
 * @param name
 */
void lecteurService::init(QQmlContext *context,QString name) {
    context->setContextProperty(name, &lecteurService::getInstance());
}

/**
 * @brief ajouter à la liste de lecture
 * si un audio, ajoute un seul fichier, si rep, ajoute tout les fichiers audio du rep
 * @param vurl la liste des vurl vers des "albums" ou des titres à ajouter
 * @param recursif : si true, ajouter les média récursivement
 */
void lecteurService::ajouter(const QList<QString> &vurl, bool recursif) {
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
void lecteurService::remplacer(const QList<QString> &vurl, bool recursif) {
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
    dureeTotale=0;
    emit listeChanged();
    emit titreChanged(NULL);
    emit etatChanged(false);

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
void lecteurService::doChargement(const QList<QString> &vurls, bool recursif, qint64 position, bool autoplay) {

    QList<VFInfo *> *li = new QList<VFInfo *>();
    foreach (QString source, vurls) {
        if(VFI.exist(source)==false) {
            continue;
        }

        // on n'ajoute à l'historique que les rep/album racines
        if( VFI.isDirectory(source)) {
            // on ajoute pas à l'historique si on est ne train de faire une restauration
            if(lecteurService::getInstance().isSessionRestaurePlaylist()==false) {
                QMetaObject::invokeMethod(&HistoriqueService::getInstance(),
                                          "ajouter",
                                          Qt::QueuedConnection,
                                          Q_ARG(QString, source));
            }

            QList<VFInfo *> res = VFI.getSynch(source, VFService::FILTERAUDIO,
                                               recursif ? VFService::RECURSIVE : VFService::FILE, VFService::URI );
            li->append(res);
            continue;
        }

        // un fichier (audio) ajoute
        VFInfo *singleInfo = VFI.getInfo(source);
        if (singleInfo) {
            li->append(singleInfo);
        }
    }

    QList<MetaData *> *ll = new QList<MetaData *>();
    foreach(VFInfo *info, *li) {
        if (!info) {
            continue;
        }
        MetaData *meta = MetaDataService::getInstance().getMetaData( info->getVUrl());
        if (!meta) {
            continue;
        }
        ll->append(meta);

        if(ll->length()>MAX_TAILLE_PLAYLIST) {
            QMetaObject::invokeMethod(&lecteurService::getInstance(),
                                      "notifierTailleMax",
                                      Qt::QueuedConnection);
            break;
        }
    }

    if(li->length()>0) {
        qDeleteAll(*li);
    }
    delete li;

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
    emit lecteurService::getInstance().chargementFaitIntr(ll,position,autoplay);
}

void lecteurService::notifierTailleMax() {
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
void lecteurService::finChargementImp(QList<MetaData *>*ll, const qint64 position,bool autoplay) {

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
        emit listeChanged();
        emit titreChanged(NULL);
        emit etatChanged(false);
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
        MetaData *t = getTitre();
        if (t && t->getDuree() > 0 && pendingRestorePositionMs > static_cast<qint64>(t->getDuree()) * 1000LL) {
            pendingRestorePositionMs = static_cast<qint64>(t->getDuree()) * 1000LL;
        }
    } else if (!restoring) {
        pendingRestorePositionMs = -1;
    }

    emit listeChanged();

    if (restoring && pendingRestorePositionMs > 0) {
        emit dureeChanged(pendingRestorePositionMs / 1000);
        applyPendingRestorePosition();
    } else if (!restoring) {
        persistSession();
    }

    if(autoplay) {
        lire();
    }

    delete ll;
}

/**
  * recharge la precedente liste de lecture
  */
void lecteurService::restaurerSession() {
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
void lecteurService::persistSession() {
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
 * @brief lecteurService::sauverSession
 */
void lecteurService::sauverSession() {
    qDebug() << "saveSession start";
    persistSession();
    qDebug() << "saveSession stop";

    arreter();
}

/**
 * @brief Applique la position restaurée dès que le média est prêt et seekable
 */
void lecteurService::applyPendingRestorePosition() {
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
    if (mprislecteur) {
        emit mprislecteur->Seeked(static_cast<qlonglong>(target) * 1000LL);
    }
}

qint64 lecteurService::getPositionMs() const {
    if (pendingRestorePositionMs > 0) {
        return pendingRestorePositionMs;
    }
    return media ? media->position() : 0;
}

int lecteurService::getPosition() const {
    return static_cast<int>(getPositionMs() / 1000);
}

/**
 * @brief retire le titre qui a l'uuid uuid
 * @param uuid
 */
void lecteurService::retirerUuid(qint64 uuid) {
    qint64 index = trouverIndex(uuid);
    if(index < 0 || index >= liste.size()) {
        return;
    }

    MetaData *removed = liste.takeAt(index);
    if (removed) {
        dureeTotale = qMax<qint64>(0, dureeTotale - removed->getDuree());
    }

    if(playlist && index < playlist->mediaCount()) {
        playlist->removeMedia(index);
    }

    persistSession();
    emit listeChanged();
}

/**
 * @brief deplace vers le haut (debut de liste) le titre à l'index index
 * @param position
 */
void lecteurService::monterUuid(const qint64 uuid) {
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
        emit listeChanged();
        return;
    }

    QMediaContent m = playlist->media(position);
    playlist->insertMedia(position-1, m);
    playlist->removeMedia(position+1);
    persistSession();
    emit listeChanged();
}

/**
 * @brief deplace vers le bas le titre à l'index index
 * @param index
 */
void lecteurService::descendreUuid(const qint64 uuid) {
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
        emit listeChanged();
        return;
    }

    QMediaContent m = playlist->media(position);
    playlist->insertMedia(position+2, m);
    playlist->removeMedia(position);
    persistSession();
    emit listeChanged();
}

/**
 * @brief retourne le titre en cours de lecture ou null si aucun
 * @return
 */
MetaData *lecteurService::getTitre() const {
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
void lecteurService::effacer() {
    if(media && media->state() != QMediaPlayer::StoppedState) {
        stopper();
    }

    if(playlist) {
        playlist->clear();
    }
    liste.clear();
    dureeTotale=0;
    nom=QString();
    pendingRestorePositionMs = -1;

    persistSession();
    emit listeChanged();
    emit titreChanged(NULL);
    emit etatChanged(false);
}

/**
 * @brief lire ou mettre en pause
 */
void lecteurService::lire(qint64 index) {
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

    media->play();
}

/**
 * @brief stopper la lecture
 */
void lecteurService::stopper() {
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
void lecteurService::suivant() {
    if(!playlist || playlist->nextIndex() == -1) {
        return;
    }
    pendingRestorePositionMs = -1;
    playlist->next();
}

/**
* @brief passe à la piste precedente si possible.
*/
void lecteurService::precedent() {
    if(!playlist || playlist->previousIndex() == -1 ) {
        return;
    }

    pendingRestorePositionMs = -1;
    playlist->previous();
}

/**
 * @brief avance la position de la lecture dans le morceau en cours
 */
void lecteurService::avancer(qint64 pos) {
    if (!media) return;
    qint64 posMs = pos * 1000LL;
    if (posMs < 0) {
        return;
    }

    if (media->state() == QMediaPlayer::StoppedState && media->duration() <= 0) {
        MetaData *t = getTitre();
        qint64 maxMs = t ? static_cast<qint64>(t->getDuree()) * 1000LL : 0;
        if (maxMs > 0 && posMs > maxMs) {
            posMs = maxMs;
        }
        pendingRestorePositionMs = posMs;
        emit dureeChanged(posMs / 1000);
        if (mprislecteur) {
            emit mprislecteur->Seeked(static_cast<qlonglong>(posMs) * 1000LL);
        }
        persistSession();
        return;
    }

    if (posMs > media->duration()) {
        return;
    }

    pendingRestorePositionMs = -1;
    media->setPosition(posMs);
    if (mprislecteur) {
        emit mprislecteur->Seeked(static_cast<qlonglong>(posMs) * 1000LL);
    }
    persistSession();
}

/**
  * @brief setAleatoire
  * @param v
  */
void lecteurService::switchAleatoire() {
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
  * @brief setBoucle
  * @param v
  */
void lecteurService::switchBoucle(){
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
 * @brief lecteurService::changementEtat
 * @param state
 */
void lecteurService::changementEtat(QMediaPlayer::State state) {
    emit etatChanged(state == QMediaPlayer::PlayingState);
    applyPendingRestorePosition();
    if (state == QMediaPlayer::PausedState || state == QMediaPlayer::StoppedState) {
        persistSession();
    }
}

/**
 * @brief lecteurService::changementPosition
 * @param position
 */
void lecteurService::changementPosition(qint64 positionMs) {
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
 * @brief lecteurService::changementDureeMedia
 * @param duration
 */
void lecteurService::changementDureeMedia(qint64 duration) {
    if (mprislecteur) {
        mprislecteur->changementDuree(duration);
    }
    applyPendingRestorePosition();
}

void lecteurService::changementMediaStatus(QMediaPlayer::MediaStatus status) {
    if (status == QMediaPlayer::InvalidMedia || status == QMediaPlayer::EndOfMedia) {
        pendingRestorePositionMs = -1;
        return;
    }
    applyPendingRestorePosition();
}

void lecteurService::changementSeekable(bool seekable) {
    if (seekable) {
        applyPendingRestorePosition();
    }
}

/**
 * @brief lecteurService::changementIndex
 * @param position
 */
void lecteurService::changementIndex(int position) {
    if(position==-1) {
        pendingRestorePositionMs = -1;
        if (media) media->stop();
        if (playlist && playlist->mediaCount() > 0) {
            playlist->setCurrentIndex(0);
        } else {
            emit titreChanged(NULL);
        }
        return;
    }

    if (!sessionRestaurePlaylist) {
        pendingRestorePositionMs = -1;
    }

    emit indexChanged(position);
    emit titreChanged(getTitre());

    if (!sessionRestaurePlaylist) {
        persistSession();
    }
}

/**
 * @brief parcours la liste et retourne l'index du titre qui a l'uuid ; -1 si trouve pas
 * @param uuid
 * @return
 */
qint64 lecteurService::trouverIndex( const qint64 uuid) const {
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
 * @brief lecteurService::appelRecu
 * @param a
 */
void lecteurService::appelRecu(const QDBusMessage &a) {

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
 * @brief lecteurService::setShowControl
 * @param val
 */
void lecteurService::setShowControl(const bool val) {
    if( this->showControl == val) {
        return;
    }
    this->showControl = val;
    emit showControlChanged();
}

/**
 * @brief si change l'affichage des controle en plein ecran dans la config
 */
void lecteurService::ControlChanged_impl() {
    this->setShowControl(configService::getInstance().getControle());
}

/**
 * @brief lecteurService::handleMprisPropertiesChanged
 * @param interface
 * @param changed
 * @param invalidated
 */
void lecteurService::handleMprisPropertiesChanged(const QString &interface, const QMap<QString, QVariant> &changed, const QStringList &invalidated) {
    qDebug() << "handleMprisPropertiesChanged ! " << interface << " " ;
}

/**
 * @brief favorisService::onRemoved
 * @param vurl
 */
void lecteurService::onRemoved(QString vurl) {

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
 * @brief lecteurService::onMoved
 * @param vurl
 * @param as
 */
void lecteurService::onMoved(QString vurl, QString as) {

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
 * @brief lecteurService::onRenamed
 * @param vurl
 * @param name
 */
void lecteurService::onRenamed(QString vurl, QString name) {
    QString p = VFI.getParentVUrl(vurl);
    QString as  = p + (VFI.isRoot(p) ? "" : "/") + name;
    onMoved(vurl, as);
}

