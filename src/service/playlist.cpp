#include "playlist.h"
#include <QList>
#include <QDir>
#include <QFile>
#include <QSet>
#include <QVariant>
#include <QStringList>
#include <QStandardPaths>
#include <QtAlgorithms>
#include <QFuture>
#include <QtConcurrent/QtConcurrentRun>
#include <qdebug.h>
#include <qregularexpression.h>
#include <QMetaObject>
#include <QThread>

#include "config.h"
#include "manager/plsmanager.h"
#include "metadataservice.h"

#include "../vi18n/vi18n.h"
#include "../virtualfiles/VFservice.h"
#include "../sdcardlocator/sdcardlocator.h"

#include <QFileInfo>
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
    connect(&configService::getInstance(), SIGNAL(sourceChanged()), this, SLOT(onSourceChanged()));
    connect(&configService::getInstance(), SIGNAL(playlistLocationChanged()), this, SLOT(onSourceChanged()));

    refresh();
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
QList<MetaData *> playlistService::open(const qint64 uuid) {

    QList<MetaData *> list;

    qint64 index = findIndex(uuid);
    if(index==-1) {
        return list;
    }

    Pls *item = liste.at(index);

    foreach (QString s, item->getListe()) {
        MetaData *m = MetaDataService::getInstance().getMetaData(s);
        if (m != NULL) {
            list.append(m);
        }
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
    qint64 index = findIndex(uuid);
    if(index==-1) {
        return res;
    }

    res.setValue( Pls::copy(liste.at(index)) );
    return res;
}

/**
  * @brief version non blocante de open
  * la liste sera disponible sur le signal playlistOuverte
  * @param uuid  id de la liste
  */
bool playlistService::openAsync(const qint64 uuid) {

    qint64 index = findIndex(uuid);
    if(index==-1) {
        return false;
    }

    QFuture<void> f = QtConcurrent::run(doOpen,uuid);
    return true;
}

/**
 * @brief fonction app par le qfuture
 * @param uuid
 */
void playlistService::doOpen(const qint64 uuid) {

    QList<MetaData *> list = playlistService::getInstance().open(uuid);

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
qint64 playlistService::create(const QString &nom) {

    if(nom.length() ==0) {
        return -1;
    }

    if(this->repPath.isEmpty() || !QDir().exists(this->repPath) || !QFileInfo(this->repPath).isWritable()) {
        this->repPath = getPlaylistLocation();
    }
    QDir().mkpath(this->repPath);

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

    sortList();

    emit listeChanged();

    return pls->getUuid();
}

/**
 * @brief enregistre la playlist
 * @param uuid
 * @param tracks la liste des titres
 */
bool playlistService::write(const qint64 uuid,const QList<MetaData *> tracks) {
    qint64 index = findIndex(uuid);
    if(index==-1) {
        return false;
    }

    Pls *pls = liste.at(index);
    QStringList chemins;
    QVariantList durees;

    if(pls->getType() == Pls::PlsType::PLS) {
        if( plsManager::marshaller(tracks, pls->getPath()) ==false ) {
            return false;
        }

        foreach (MetaData *m, tracks) {
            if (m) {
                chemins.append(m->getChemin());
                durees.append(QVariant::fromValue(static_cast<qulonglong>(m->getDuree())));
            }
        }
    }

    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this,
                                  "finaliseWrite",
                                  Qt::QueuedConnection,
                                  Q_ARG(qint64, uuid),
                                  Q_ARG(QStringList, chemins),
                                  Q_ARG(QVariantList, durees));
    } else {
        finaliseWrite(uuid, chemins, durees);
    }
    return true;
}

void playlistService::finaliseWrite(const qint64 uuid, const QStringList &chemins, const QVariantList &durees) {
    qint64 index = findIndex(uuid);
    if(index==-1) {
        return;
    }

    Pls *pls = liste.at(index);
    if(pls->getType() == Pls::PlsType::PLS) {
        pls->resetUrl();
        for (int i = 0; i < chemins.size(); ++i) {
            quint64 d = (i < durees.size()) ? durees.at(i).toULongLong() : 0;
            pls->ajouterUrl(chemins.at(i), d);
        }
    }

    emit tracksChanged(uuid);
}

/**
 * @brief playlistService::writeAsync
 * @param uuid
 * @param tracks
 * @return
 */
bool playlistService::writeAsync(const qint64 uuid,const QList<MetaData *> tracks) {
    qint64 index = findIndex(uuid);
    if(index==-1) {
        return false;
    }

    QFuture<void> f = QtConcurrent::run(doWrite,uuid,tracks);
    return true;
}

/**
 * @brief enregistre la playlist
 * @param uuid
 * @param tracks la liste des titres
 */
bool playlistService::writeAsync(const qint64 uuid,const QList<QString> tracks) {
    QList<MetaData *> l;
    foreach (QString t, tracks) {
        MetaData *m = MetaDataService::getInstance().getMetaData(t);
        if(m != NULL) {
            l.append(m);
        }
    }

    return writeAsync(uuid,l);
}

/**
 * @brief doWrite
 * @param tracks
 */
void playlistService::doWrite(const qint64 uuid, const QList<MetaData *> tracks) {
    playlistService::getInstance().write(uuid,tracks);
}

/**
  * @brief ajoute les titres à la playlist pointé par uuid
  * cette fonction a vocation à être appelé depuis le navigateur
  * donc tracks est des vurl
  * @param uuid : la playlist
  * @param tracks : la listes des pistes
  */
bool playlistService::add(const qint64 uuid, const QList<QString> tracks) {

    if(tracks.length()==0) {
        return true;
    }

    qint64 index = findIndex(uuid);
    if(index==-1) {
        return false;
    }

    // recup la liste
    QList<MetaData *> l = open(uuid);

    //... ajoute les nouveaux medias
    foreach (QString str, tracks) {
        MetaData *m = MetaDataService::getInstance().getMetaData(str);
        if(m != NULL) {
            l.append(m);
        }
    }

    // ecrit sur hdd et met à jour la pls
    if( write(uuid,l) == false ) {
        return false;
    }

    // FAIT EN ECRITURE
    // on met a jour la donne en cache -- le fait après quand on est sur que la playlist a été enregistré
    Pls *pls = liste.at(index);
    const QString plsNom = pls->getNom();

    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this,
                                  "notifierAdd",
                                  Qt::QueuedConnection,
                                  Q_ARG(int, tracks.length()),
                                  Q_ARG(QString, plsNom));
    } else {
        notifierAdd(tracks.length(), plsNom);
    }

    return true;
}

void playlistService::notifierAdd(int count, const QString &nom) {
    Notification n;
    n.setPreviewBody(VI18N.get("playlists.ajouter.a").arg(count).arg(nom));
    n.setHintValue("x-nemo-icon", "image://theme/icon-s-high-importance");
    n.publish();
}

/**
  * @brief version non blocante d'add
  * resultat sur le signal tracksChanged
  * @param uuid
  * @param tracks la liste des titres
  * @return false sur erreur, true si lance l'ajout + signal
  */
bool playlistService::addAsync(const qint64 uuid, const QList<QString> tracks) {
    qint64 index = findIndex(uuid);
    if(index==-1) {
        return false;
    }

    QFuture<void> f = QtConcurrent::run(doAdd,uuid,tracks);
    return true;
}

/**
 * @brief fonction app par le qfuture
 * @param uuid
 */
void playlistService::doAdd(const qint64 uuid, const QList<QString> tracks) {
    playlistService::getInstance().add(uuid,tracks);
}

/**
 * @brief suprimme la playlist "nom"
 * @param nom
 */
void playlistService::supprimer(const qint64 uuid) {

    qint64 index = findIndex(uuid);
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
bool playlistService::rename(const qint64 uuid,const QString &nom) {
    if(nom.length()==0) {
        return false;
    }

    qint64 index = findIndex(uuid);
    if(index==-1) {
        return false;
    }
    Pls *item = liste.at(index);

    QString ext(".pls");
    if(item->getType() == Pls::PlsType::M3U) {
        ext = ".m3u";
    }
    QString parentDir = QFileInfo(item->getPath()).absolutePath();
    if(parentDir.isEmpty()) {
        parentDir = this->repPath;
    }
    QString dest(parentDir+"/"+nom +ext);


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

    sortList();

    emit nomChanged(uuid,nom);
    emit listeChanged();
    return true;
}

/**
 * @brief refresh la liste des playlist
 */
void playlistService::refresh() {
    if(liste.length()>0) {
        qDeleteAll(liste);
    }
    liste.clear();

    QSet<QString> seenFiles;
    const QStringList locations = getPlaylistLocations();

    foreach (const QString &location, locations) {
        QDir dir(location);
        dir.setFilter(QDir::Files|QDir::Readable);
        dir.setSorting(QDir::Name);

        QStringList l = dir.entryList();
        foreach (const QString &f, l) {
            if(REG_PLS.match(f).hasMatch()) {
                const QString fullPath = dir.absoluteFilePath(f);
                const QString canonical = QFileInfo(fullPath).canonicalFilePath();
                const QString key = canonical.isEmpty() ? fullPath : canonical;
                if(seenFiles.contains(key)) {
                    continue;
                }
                seenFiles.insert(key);
                liste.append( plsManager::unmarshaller(fullPath) );
            } else if(REG_M3U.match(f).hasMatch()) {
            }
        }
    }

    sortList();
    emit listeChanged();
}

/**
 * @brief retourne l'index dans la liste correspondant a l'uuid ou -1
 * @param uuid
 * @return
 */
qint64 playlistService::findIndex( const qint64 uuid) const {
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
 * @brief sortList
 */
void playlistService::sortList() {
    std::sort(liste.begin(), liste.end(), playlistService::alphaOrder);
}

/**
 * @brief alphaOrder
 * @param a
 * @param b
 * @return
 */
bool playlistService::alphaOrder(  Pls * a, Pls  * b ) {
    return a->getNom().compare( b->getNom() ) < 0 ? true : false;
}

/**
 * @brief retourne le repertoire ou sont les playlists
 * @return
 */
QStringList playlistService::getPlaylistLocations() {
    QStringList locations;
    const QString activeLocation = getPlaylistLocation();
    QFileInfo info(activeLocation);
    if(info.exists() && info.isDir() && info.isReadable()) {
        locations.append(QDir::cleanPath(info.absoluteFilePath()));
    }
    return locations;
}

/**
 * @brief retourne le repertoire ou sont les playlists
 * @return
 */
QString playlistService::getPlaylistLocation() {
    const quint16 locMode = configService::getInstance().getPlaylistLocation();
    QString targetPath;

    if(locMode == PLAYLIST_LOCATION_MUSIC) {
        targetPath = QDir::cleanPath(QDir::homePath() + "/Music/playlists");
    } else if(locMode == PLAYLIST_LOCATION_SDCARD) {
        if(SdcardLocator::getInstance().isPresent()) {
            QString sdMusic = configService::getInstance().getNomsdcard().trimmed();
            while (sdMusic.startsWith('/')) {
                sdMusic.remove(0, 1);
            }
            while (sdMusic.endsWith('/')) {
                sdMusic.chop(1);
            }
            if(sdMusic.isEmpty()) {
                sdMusic = QStringLiteral("Music");
            }
            targetPath = QDir::cleanPath(SdcardLocator::getInstance().getLocation() + "/" + sdMusic + "/playlists");
        } else {
            qDebug() << "playlistService : sdcard not present, falling back to ~/Playlist";
            targetPath = QDir::cleanPath(QDir::homePath() + "/Playlist");
        }
    } else {
        targetPath = QDir::cleanPath(QDir::homePath() + "/Playlist");
    }

    if(!QDir().exists(targetPath)) {
        QDir().mkpath(targetPath);
    }

    qDebug() << "playlistService : using playlist location" << targetPath;
    return targetPath;
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
        writeAsync(p->getUuid(),l);
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
        writeAsync(p->getUuid(),l);
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

void playlistService::onSourceChanged() {
    this->repPath = getPlaylistLocation();
    refresh();
}

