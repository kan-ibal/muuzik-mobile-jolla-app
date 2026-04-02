#include "lecteur.h"
#include <qregularexpression.h>
#include <QDir>
#include <QtAlgorithms>
#include <QFuture>
#include <QtConcurrent/QtConcurrentRun>
#include <qdebug.h>
#include <QStack>

#include "config.h"
#include "../app.h"
#include "playlist.h"
#include "historique.h"
#include "metadataservice.h"

#include "../vi18n/vi18n.h"
#include "../virtualfiles/VFservice.h"

#include <nemonotifications-qt5/notification.h>

#define MAX_TAILLE_SAUVEGARDE 50
#define MAX_TAILLE_PLAYLIST 150

/**
 * @brief lecteurService::lecteurService
 * @param parent
 */
lecteurService::lecteurService(QObject *parent) : QObject(parent),dureeTotale(0),sessionRestaure(false),sessionRestaurePlaylist(false),raf(false){

    // le "chargeur" de titre
    connect(this,
            SIGNAL(chargementFaitIntr(QList<MetaData*>*,qint64,bool)),
            this,
            SLOT(finChargementImp(QList<MetaData*>*,qint64,bool))
            );

    // le lecteur
    media.setPlaylist(&playlist);
    connect(&media, SIGNAL(stateChanged(QMediaPlayer::State)), SLOT(changementEtat(QMediaPlayer::State)));
    connect(&playlist, SIGNAL(currentIndexChanged(int)), SLOT(changementIndex(int)));
    connect(&media, SIGNAL(positionChanged(qint64)), SLOT(changementPosition(qint64)));

    // dbus
    mpris = new Mpris(this);
    mprislecteur = new MprisLecteur(this);

    QDBusConnection session = QDBusConnection::sessionBus();
    session.registerObject(MPRIS_OBJ, this);
    session.registerService(MPRIS_SERVICE);
    session.registerService(MPRIS_SERVICE_ID);

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
 * @brief lecteurService::~lecteurService
 */
lecteurService::~lecteurService() {

    QDBusConnection session = QDBusConnection::sessionBus();
    session.unregisterService(MPRIS_SERVICE);
    session.unregisterService(MPRIS_SERVICE_ID);
    session.unregisterObject(MPRIS_OBJ);

    session.disconnect("",
                       "/calls/active",
                       "org.nemomobile.voicecall.VoiceCall",
                       "lineIdChanged",
                       this, SLOT(appelRecu(const QDBusMessage&)));
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
    emit enCoursChanged();

    // arrête le player et vide la liste
    media.stop();
    playlist.clear();
    //qDeleteAll(liste);
    liste.clear();
    dureeTotale=0;

    bool autoplay = configService::getInstance().getAutoStart() && media.state() != QMediaPlayer::PlayingState ;

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
                HistoriqueService::getInstance().ajouter(source);
            }

            QList<VFInfo *> res = VFI.getSynch(source, VFService::FILTERAUDIO,
                                               recursif ? VFService::RECURSIVE : VFService::FILE, VFService::URI );
            li->append(res);
            continue;
        }

        // un fichier (audio) ajoute
        li->append( VFI.getInfo(source) );
    }

    QList<MetaData *> *ll = new QList<MetaData *>();
    foreach(VFInfo *info, *li) {
        ll->append( MetaDataService::getInstance().getMetaData( info->getVUrl()) );

        if(ll->length()>MAX_TAILLE_PLAYLIST) {
            Notification n;
            n.setPreviewBody(vI18n::getInstance().get( "lecteur.taillemax" ));
            n.setHintValue("x-nemo-icon", "image://theme/icon-s-high-importance");
            n.publish();
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
            quint64 index = 0;

            foreach(MetaData *m, *ll) {
                if(m->getChemin().compare(str)==0) {
                    break;
                }
                index++;
            }
            position = index;

        } catch(const std::exception & e) {
            qDebug()<< "on the exception";
        }
    }

    // ok, passe le resutat au service par un signal/slot
    emit lecteurService::getInstance().chargementFaitIntr(ll,position,autoplay);
}

/**
 * @brief la liste de lecture est créé et disponible pour affichage
 * @param ll
 * @param pls
 * @param position
 */
void lecteurService::finChargementImp(QList<MetaData *>*ll, const qint64 position,bool autoplay) {

    raf=false;
    emit  enCoursChanged();

    if(ll->length()==0) {
        return;
    }

    playlist.addMedia( MetaData::versMediaContent(ll) );
    liste.append(*ll);

    foreach (MetaData *t, liste) {
        dureeTotale+=t->getDuree();
    }

    // an s'assure de ne plus être dans le cas d'une restauration (pas de play, pas d'historique)
    sessionRestaurePlaylist=false;


    // debut de liste : ne peut venir que d'une restauration
    if(position > -1) {
        long p = position<playlist.mediaCount()-1 ? position : 0;
        playlist.setCurrentIndex(p);
    }

    emit listeChanged();

    if(autoplay) {
        lire();
    }
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


    quint64 pos = settings.value(LECTURE_LISTE_INDEX,0).toInt();

    raf=true;
    emit enCoursChanged();

    // lancement du scan
    sessionRestaurePlaylist=true;   // on recharger depuis la session, pas de lecture automatique
    QFuture<void> f = QtConcurrent::run(doChargement,vurl,false, pos,false);
}

/**
 * @brief lecteurService::sauverSession
 */
void lecteurService::sauverSession() {
    qDebug() << "saveSession start";

    QList<QString> l;

    quint64 taille = 0;
    foreach (MetaData *t, liste) {
        l.append(t->getChemin());
        if(++taille > MAX_TAILLE_SAUVEGARDE) {
            break;
        }
    }

    settings.setValue(LECTURE_LISTE, QVariant::fromValue(l));
    settings.setValue(LECTURE_LISTE_INDEX, playlist.currentIndex() );
    settings.sync();
    qDebug() << "saveSession stop";
}

/**
 * @brief retire le titre qui a l'uuid uuid
 * @param uuid
 */
void lecteurService::retirerUuid(qint64 uuid) {
    qint64 index = trouverIndex(uuid);
    if(index==-1) {
        return;
    }

    playlist.removeMedia(index);
    liste.removeAt(index);

    emit listeChanged();
}

/**
 * @brief deplace vers le haut (debut de liste) le titre à l'index index
 * @param position
 */
void lecteurService::monterUuid(const qint64 uuid) {
    qint64 position = trouverIndex(uuid);
    if(position==-1) {
        return;
    }

    liste.swap(position, position-1);

    if(position==playlist.currentIndex()) {
        QMediaContent m = playlist.media(position-1);
        playlist.insertMedia(position+1, m);
        playlist.removeMedia(position-1);
        emit listeChanged();
        return;
    }

    QMediaContent m = playlist.media(position);
    playlist.insertMedia(position-1, m);
    playlist.removeMedia(position+1);
    emit listeChanged();
}

/**
 * @brief deplace vers le bas le titre à l'index index
 * @param index
 */
void lecteurService::descendreUuid(const qint64 uuid) {
    qint64 position = trouverIndex(uuid);
    if(position==-1) {
        return;
    }

    liste.swap(position, position+1);

    if(position==playlist.currentIndex()) {
        QMediaContent m = playlist.media(position+1);
        playlist.insertMedia(position-1, m);
        playlist.removeMedia(position+2);
        emit listeChanged();
        return;
    }

    QMediaContent m = playlist.media(position);
    playlist.insertMedia(position+2, m);
    playlist.removeMedia(position);
    emit listeChanged();
}

/**
 * @brief retourne le titre en cours de lecture ou null si aucun
 * @return
 */
MetaData *lecteurService::getTitre() const {
    if( playlist.mediaCount()<=0) {
        return NULL;
    }
    if( playlist.currentIndex()<0) {
        return NULL;
    }

    return liste.at(playlist.currentIndex());
}

/**
 * @brief effacer la liste et eventuellement arreter la lecture
 * @return
 */
void lecteurService::effacer() {
    if(media.state() != QMediaPlayer::StoppedState) {
        stopper();
    }

    playlist.clear();
    liste.clear();
    dureeTotale=0;
    nom=QString();

    emit listeChanged();
    emit titreChanged(NULL);
}

/**
 * @brief lire ou mettre en pause
 */
void lecteurService::lire(qint64 index) {
    if(playlist.mediaCount()<=0) {
        return;
    }

    if( index >playlist.mediaCount()-1) {
        return;
    }

    if( index >=0) {
        playlist.setCurrentIndex(index);
        media.play();
        return;
    }

    if( media.state() == QMediaPlayer::PlayingState) {
        media.pause();
        return;
    }

    media.play();
}

/**
 * @brief stopper la lecture
 */
void lecteurService::stopper() {
    media.stop();
    //@fix utile ou deja fait par le signal positionChanged ???
    //dureetitre=0;
    // emit dureeChanged(dureetitre);
}

/**
* @brief passe à la piste suivant si possible.
*/
void lecteurService::suivant() {

    if(playlist.nextIndex() == -1) {
        return;
    }
    playlist.next();
}

/**
* @brief passe à la piste precedente si possible.
*/
void lecteurService::precedent() {
    if(playlist.previousIndex() == -1 ) {
        return;
    }

    playlist.previous();
}

/**
 * @brief avance la position de la lecture dans le morceau en cours
 */
void lecteurService::avancer(qint64 pos) {
    pos *=1000;
    if( pos < 0 || pos > media.duration()) {
        return;
    }

    media.setPosition(pos);
}

/**
  * @brief setAleatoire
  * @param v
  */
void lecteurService::switchAleatoire() {
    playlist.setPlaybackMode( playlist.playbackMode() != QMediaPlaylist::Random
            ? QMediaPlaylist::Random : QMediaPlaylist::Sequential);

    Notification n;
    n.setPreviewBody(vI18n::getInstance().get( playlist.playbackMode() == QMediaPlaylist::Random  ? "lecteur.aleatoire.on" :"lecteur.aleatoire.off") );
    n.setHintValue("x-nemo-icon", "image://theme/icon-s-attach");
    n.publish();

    emit modeChanged();
}

/**
  * @brief setBoucle
  * @param v
  */
void lecteurService::switchBoucle(){
    playlist.setPlaybackMode( playlist.playbackMode() != QMediaPlaylist:: Loop
            ? QMediaPlaylist::Loop : QMediaPlaylist::Sequential);

    Notification n;
    n.setPreviewBody(vI18n::getInstance().get( playlist.playbackMode() == QMediaPlaylist::Loop  ? "lecteur.boucle.on" :"lecteur.boucle.off") );
    n.setHintValue("x-nemo-icon", "image://theme/icon-s-attach");
    n.publish();

    emit modeChanged();
}

/**
 * @brief lecteurService::changementEtat
 * @param state
 */
void lecteurService::changementEtat(QMediaPlayer::State state) {
    emit etatChanged(state == QMediaPlayer::PlayingState);
}

/**
 * @brief lecteurService::changementPosition
 * @param position
 */
void lecteurService::changementPosition(qint64 duration) {
    emit dureeChanged(duration/1000);
}

/**
 * @brief lecteurService::changementIndex
 * @param position
 */
void lecteurService::changementIndex(int position) {
    if(position==-1) {
        media.stop();
        playlist.setCurrentIndex(0);
        return;
    }

    emit indexChanged(position);
    emit titreChanged(getTitre());
}

/**
 * @brief parcours la liste et retourne l'index du titre qui a l'uuid ; -1 si trouve pas
 * @param uuid
 * @return
 */
qint64 lecteurService::trouverIndex( const qint64 uuid) const {
    qint64 index = 0;

    foreach (MetaData *t , liste) {
        if (t->getUuid()==uuid ) {
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

    if( media.state() == QMediaPlayer::PlayingState) {
        media.pause();
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
        if(m->getChemin().startsWith(vurl)) {
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

        if ( m->getChemin().startsWith(vurl) ) {
            QString target = m->getChemin().replace(vurl, as);
            m->setChemin(target);

            // media
            playlist.removeMedia(i);
            playlist.insertMedia(i, MetaData::versMediaContent(m) );
        }
    }
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

