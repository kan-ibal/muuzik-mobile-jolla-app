#include "playlist.h"
#include <QList>
#include <QDir>
#include <QFile>
#include <QVariant>
#include <QStringList>
#include <QStandardPaths>
#include <QtAlgorithms>
#include <QFuture>
#include <QtConcurrent/QtConcurrentRun>
#include <qdebug.h>
#include <qregularexpression.h>

#include "manager/plsmanager.h"
#include "metadataservice.h"

#include "../vi18n/vi18n.h"
#include "../virtualfiles/VFservice.h"

#include <nemonotifications-qt5/notification.h>

static QRegularExpression REG_PLS( "\\.pls$", QRegularExpression::CaseInsensitiveOption);
static QRegularExpression REG_M3U( "\\.m3u$", QRegularExpression::CaseInsensitiveOption);

/**
 * @brief playlistService::playlistService
 * @param parent
 */
playlistService::playlistService(QObject *parent) : QObject(parent){
    // on init le repertoire des playlists
    this->repPath = getPlaylistLocation();

    connect(&VFService::getInstance(), SIGNAL(removed(QString)),this, SLOT(onRemoved(QString)));
    connect(&VFService::getInstance(), SIGNAL(moved(QString,QString)), this, SLOT(onMoved(QString,QString)));
    connect(&VFService::getInstance(), SIGNAL(renamed(QString,QString)), this, SLOT(onRenamed(QString,QString)));

    recharger();
}

playlistService::~playlistService() {
    qDeleteAll(liste);
    liste.clear();
}

/**
 * @brief playlistService::getInstance
 * @return
 */
playlistService& playlistService::getInstance() {
    static playlistService instance;
    return instance;
}

/**
 * @brief recentService::init
 * @param context
 * @param name
 */
void playlistService::init(QQmlContext *context,QString name) {
    context->setContextProperty(name, &playlistService::getInstance());
    qmlRegisterType<Pls>("Pls", 1, 0, "Pls");
}

/**
 * @brief retourne la liste des medias (url) de la playlist defini par uuid
 * @param nom
 * @return la liste ou vide si rien (vide)
 */
QList<MetaData *> playlistService::ouvrir(const qint64 uuid) {

    QList<MetaData *> list;

    qint64 index = trouverIndex(uuid);
    if(index==-1) {
        return list;
    }

    Pls *item = liste.at(index);

    foreach (QString s, item->getListe()) {
        list.append( MetaDataService::getInstance().getMetaData(s) );
    }

    //    switch (item->getType()) {
    //    case Pls::PlsType::PLS :
    //        return plsManager::unmarshaller(item->getPath());
    //    default:
    //        break;
    //    }

    return list;
}

/**
  * @brief retourne les info de la playlist à partir de son id
  * @param uuid
  * @return la playlist
  */
QVariant playlistService::getPls(const qint64 uuid) const {
    QVariant res;
    qint64 index = trouverIndex(uuid);
    if(index==-1) {
        return res;
    }

    res.setValue( Pls::copy(liste.at(index)) );
    return res;
}

/**
  * @brief version non blocante de ouvrir
  * la liste sera disponible sur le signal playlistOuverte
  * @param uuid  id de la liste
  */
bool playlistService::ouvrirAsync(const qint64 uuid) {

    qint64 index = trouverIndex(uuid);
    if(index==-1) {
        return false;
    }

    QFuture<void> f = QtConcurrent::run(doOuvrir,uuid);
    return true;
}

/**
 * @brief fonction app par le qfuture
 * @param uuid
 */
void playlistService::doOuvrir(const qint64 uuid) {

    QList<MetaData *> list = playlistService::getInstance().ouvrir(uuid);

    QVariantList v;
    foreach (MetaData *t, list) {
        v.append(QVariant::fromValue(t));
    }

    emit playlistService::getInstance().playlistOuverte(uuid,v);
}

/**
  *@brief : cree une nouvelle liste (vide)
  *@return : l'uuid de la nouvelle liste
  */
qint64 playlistService::creer(const QString &nom) {

    if(nom.length() ==0) {
        return -1;
    }

    //    QString dest(QDir::homePath()+"/Playlists/"+nom+".pls");
    QString dest(this->repPath + "/"+nom+".pls");
    QFileInfo info(dest);
    if(info.exists()) {
        Notification n;
        n.setPreviewBody(VI18N.get("playlists.renommer.erreur"));
        n.setHintValue("x-nemo-icon", "image://theme/icon-s-high-importance");
        n.publish();
        return -1;
    }

    QFile f(dest);
    if( f.open(QIODevice::ReadWrite) ==false) {
        return -1;
    }
    f.close();

    Pls *pls = new Pls();
    pls->setType(Pls::PlsType::PLS);
    pls->setPath(dest);
    pls->setNom(nom);

    liste.append(pls);

    classerListe();

    emit listeChanged();

    return pls->getUuid();
}

/**
 * @brief enregistre la playlist
 * @param uuid
 * @param tracks la liste des titres
 */
bool playlistService::ecrire(const qint64 uuid,const QList<MetaData *> tracks) {
    qint64 index = trouverIndex(uuid);
    if(index==-1) {
        return false;
    }

    Pls *pls = liste.at(index);

    if(pls->getType() == Pls::PlsType::PLS) {
        if( plsManager::marshaller(tracks, pls->getPath()) ==false ) {
            return false;
        }

        // on met a jour la donne en cache -- le fait après quand on est sur que la playlist a été enregistré
        pls->resetUrl();
        foreach (MetaData *m, tracks) {
            pls->ajouterUrl(m->getChemin(),m->getDuree());
        }
    }

    emit tracksChanged(uuid);
    return true;
}

/**
 * @brief playlistService::ecrireASync
 * @param uuid
 * @param tracks
 * @return
 */
bool playlistService::ecrireAsync(const qint64 uuid,const QList<MetaData *> tracks) {
    qint64 index = trouverIndex(uuid);
    if(index==-1) {
        return false;
    }

    QFuture<void> f = QtConcurrent::run(doEcrite,uuid,tracks);
    return true;
}

/**
 * @brief enregistre la playlist
 * @param uuid
 * @param tracks la liste des titres
 */
bool playlistService::ecrireAsync(const qint64 uuid,const QList<QString> tracks) {
    QList<MetaData *> l;
    foreach (QString t, tracks) {
        MetaData *m = MetaDataService::getInstance().getMetaData(t);
        if(m != NULL) {
            l.append(m);
        }
    }

    return ecrireAsync(uuid,l);
}

/**
 * @brief doEcrite
 * @param tracks
 */
void playlistService::doEcrite(const qint64 uuid, const QList<MetaData *> tracks) {
    playlistService::getInstance().ecrire(uuid,tracks);
}

/**
  * @brief ajoute les titres à la playlist pointé par uuid
  * cette fonction a vocation à être appelé depuis le navigateur
  * donc tracks est des vurl
  * @param uuid : la playlist
  * @param tracks : la listes des pistes
  */
bool playlistService::ajouter(const qint64 uuid, const QList<QString> tracks) {

    if(tracks.length()==0) {
        return true;
    }

    qint64 index = trouverIndex(uuid);
    if(index==-1) {
        return false;
    }

    // recup la liste
    QList<MetaData *> l = ouvrir(uuid);

    //... ajoute les nouveaux medias
    foreach (QString str, tracks) {
        MetaData *m = MetaDataService::getInstance().getMetaData(str);
        if(m != NULL) {
            l.append(m);
        }
    }

    // ecrit sur hdd et met à jour la pls
    if( ecrire(uuid,l) == false ) {
        return false;
    }

    // FAIT EN ECRITURE
    // on met a jour la donne en cache -- le fait après quand on est sur que la playlist a été enregistré
    Pls *pls = liste.at(index);

    Notification n;
    n.setPreviewBody(VI18N.get("playlists.ajouter.a").arg(tracks.length()).arg(pls->getNom()));
    n.setHintValue("x-nemo-icon", "image://theme/icon-s-high-importance");
    n.publish();

    return true;
}

/**
  * @brief version non blocante d'ajouter
  * resultat sur le signal tracksChanged
  * @param uuid
  * @param tracks la liste des titres
  * @return false sur erreur, true si lance l'ajout + signal
  */
bool playlistService::ajouterAsync(const qint64 uuid, const QList<QString> tracks) {
    qint64 index = trouverIndex(uuid);
    if(index==-1) {
        return false;
    }

    QFuture<void> f = QtConcurrent::run(doAjouter,uuid,tracks);
    return true;
}

/**
 * @brief fonction app par le qfuture
 * @param uuid
 */
void playlistService::doAjouter(const qint64 uuid, const QList<QString> tracks) {
    playlistService::getInstance().ajouter(uuid,tracks);
}

/**
 * @brief suprimme la playlist "nom"
 * @param nom
 */
void playlistService::supprimer(const qint64 uuid) {

    qint64 index = trouverIndex(uuid);
    if(index==-1) {
        return;
    }
    Pls *item = liste.takeAt(index);

    QFile f(item->getPath());
    f.remove();

    delete item;
    emit listeChanged();
}

/**
  * @brief renomme une liste de lecture
  * @param uuid : l'uuid de la liste
  * @param nom : le nouveau non, doit être non null et ne pas exister
  * @return true si ok + envoie le signal , false si non
  */
bool playlistService::renommer(const qint64 uuid,const QString &nom) {
    if(nom.length()==0) {
        return false;
    }

    qint64 index = trouverIndex(uuid);
    if(index==-1) {
        return false;
    }
    Pls *item = liste.at(index);

    QString ext(".pls");
    if(item->getType() == Pls::PlsType::M3U) {
        ext = ".m3u";
    }
    //    QString dest(QDir::homePath()+"/Playlists/"+nom +ext);
    QString dest(this->repPath+"/"+nom +ext);


    QFileInfo info(dest);
    if(info.exists()) {
        Notification n;
        n.setPreviewBody(vI18n::getInstance().get("playlists.renommer.erreur"));
        n.setHintValue("x-nemo-icon", "image://theme/icon-s-high-importance");
        n.publish();
        return false;
    }

    QFile f(item->getPath());
    if(f.rename(dest) == false) {
        Notification n;
        n.setPreviewBody(vI18n::getInstance().get("playlists.renommer.erreur"));
        n.setHintValue("x-nemo-icon", "image://theme/icon-s-high-importance");
        n.publish();
        return false;
    }

    item->setPath(dest);
    item->setNom(nom);

    classerListe();

    emit nomChanged(uuid,nom);
    emit listeChanged();
    return true;
}

/**
 * @brief recharger la liste des playlist
 */
void playlistService::recharger() {
    //    QDir dir(QDir::homePath()+"/Playlists");
    QDir dir(this->repPath);
    dir.setFilter(QDir::Files|QDir::Readable);
    dir.setSorting(QDir::Name);

    if(liste.length()>0) {
        qDeleteAll(liste);
    }
    liste.clear();

    QStringList l = dir.entryList();
    foreach (QString f, l) {
        if(REG_PLS.match(f).hasMatch()) {
            //            liste.append( plsManager::unmarshaller(QDir::homePath()+"/Playlists/" + f) );
            liste.append( plsManager::unmarshaller(this->repPath+"/"+ f) );
        } else if(REG_M3U.match(f).hasMatch()) {
        }
    }

    classerListe();
    emit listeChanged();
}

/**
 * @brief retourne l'index dans la liste correspondant a l'uuid ou -1
 * @param uuid
 * @return
 */
qint64 playlistService::trouverIndex( const qint64 uuid) const {
    qint64 index = 0;

    foreach (Pls *t , liste) {
        if (t->getUuid()==uuid ) {
            return index;
        }
        index++;
    }

    return -1;
}

/**
 * @brief classerListe
 */
void playlistService::classerListe() {
    std::sort(liste.begin(), liste.end(), playlistService::ordreAlpha);
}

/**
 * @brief ordreAlpha
 * @param a
 * @param b
 * @return
 */
bool playlistService::ordreAlpha(  Pls * a, Pls  * b ) {
    return a->getNom().compare( b->getNom() ) < 0 ? true : false;
}

/**
 * @brief retourne le repertoire ou sont les playlists
 * @return
 */
QString playlistService::getPlaylistLocation() {

    // on regarde si un local exist
    QString local = QStandardPaths::writableLocation(QStandardPaths::DataLocation) + "/Playlists";
    if(QDir().exists(local)) {
        qDebug() << "playlistService : using local app location" ;
        return local;
    }

    // ok, le rep par defaut ???
    QString def  = QDir::homePath()+"/Playlists";
    if(QDir().exists(def)) {
        qDebug() << "playlistService : using default playlist location" ;
        return def;
    }

    // on cree le rep
    QDir().mkpath(local);

    qDebug() << "playlistService : making and using local app location" ;
    return local;
}

/**
 * @brief suppression de l'url ( et de tout les childs)
 * @param vurl
 */
void playlistService::onRemoved(QString vurl) {

    // on passe toutes les playlists pour verifier les titres
    foreach (Pls * p, liste) {

        QStringList l;

        foreach (QString s, p->getListe()) {
            if( s.startsWith(vurl) ) {
                continue;
            }
            l.append(s);
        }

        // les listes sont identiques
        if(l.length() == p->getListe().length()) {
            continue;
        }

        // on a supprimé des titres, on update la liste
        ecrireAsync(p->getUuid(),l);
    }
}

/**
     * @brief recentService::onMoved
     * @param vurl
     * @param into
     */
void playlistService::onMoved(QString vurl, QString as) {

    // on passe toutes les playlists pour verifier les titres
    foreach (Pls * p, liste) {

        QStringList l;
        bool changed = false;

        foreach (QString s, p->getListe()) {
            if( s.startsWith(vurl) ) {
                QString target = s.replace(vurl, as);
                l.append(target);

                changed = true;
                continue;
            }
            l.append(s);
        }

        // les listes sont identiques
        if(changed == false) {
            continue;
        }

        // on a modifié des titres, on update la liste
        ecrireAsync(p->getUuid(),l);
    }
}

/**
     * @brief recentService::onRenamed
     * @param vurl
     * @param name
     */
void playlistService::onRenamed(QString vurl, QString name) {
    QString p = VFI.getParentVUrl(vurl);
    QString as  = p + (VFI.isRoot(p) ? " " : "/") + name;
    onMoved(vurl, as);
}

