#include "favoris.h"
#include <qdebug.h>
#include <QtAlgorithms>
#include <QStack>

#include "../app.h"
#include "../bean/MetaData.h"
#include "../util/daofavoris.h"
#include "artwork.h"

#include "../vi18n/vi18n.h"
#include "../virtualfiles/VFservice.h"
#include "../service/metadataservice.h"

#include <nemonotifications-qt5/notification.h>

favorisService::favorisService(QObject *parent) : QObject(parent){
    config = &configService::getInstance();
    connect(config,SIGNAL(ordreFavorisChanged()), this, SLOT(ordreFavorisImpl()));

    connect(&VFService::getInstance(), SIGNAL(removed(QString)),this, SLOT(onRemoved(QString)));
    connect(&VFService::getInstance(), SIGNAL(moved(QString,QString)), this, SLOT(onMoved(QString,QString)));
    connect(&VFService::getInstance(), SIGNAL(renamed(QString,QString)), this, SLOT(onRenamed(QString,QString)));

    recharger();
    importer();
}

/**
 * @brief ::getInstance
 * @return
 */
favorisService& favorisService::getInstance() {
    static favorisService instance;
    return instance;
}
/**
 * @brief register the service into the context so it can be used in qml
 * @param context
 * @param name : register service on this name, default to FAVORISSERVICE_QML_NAME
 */
void favorisService::init(QQmlContext *context, QString name) {
    context->setContextProperty(name,&favorisService::getInstance());

    qmlRegisterType<RowFavoris>("RowFavoris", 1, 0, "RowFavoris");
    qRegisterMetaType<QList<RowFavoris*> >( "QList<RowFavoris*>" );
}

/**
 * @brief ajoute le repertoire sous "path" à la liste des favoris
 * @param path url (virtuelle) vers le repertoire
 * @return true si ajouté (+ signal)
 */
qint64 favorisService::ajouter(const QString vurl) {

    if(getFavoris(vurl)>0) {
        return -1;
    }

    QString nom;

    // on recup la meta de l'album / premier titre
    MetaData *m = MetaDataService::getInstance().getMetaData(vurl,true);
    if(m != NULL) {
        if(m->getAlbum().isEmpty()==false && m->getArtiste().isEmpty() == false) {
            nom = m->getArtiste() + " - " + m->getAlbum();
        }
    }

    if(nom.isEmpty()) {
        nom = VFI.getName(vurl).toLower();
    }

    AlbumStat *stat = MetaDataService::getInstance().getStatFromAlbum(vurl);

    // persiste en db
    dao.ajouter(nom, vurl, stat->getDuree(),stat->getPistes());

    // on evite de faire un rechargement pour garder les mêmes uuid
    // comme on modifie la liste depuis Favoris.qml gros risque de cafouillage / plantage
    RowFavoris *f2 = new RowFavoris();
    f2->setChemin(vurl);
    f2->setNom(nom);
    f2->setCreation(QDateTime::currentMSecsSinceEpoch() / 1000);
    f2->setDuree(stat->getDuree());
    f2->setPiste(stat->getPistes());
    liste.append(f2);

    classer(configService::getInstance().getOrdreFavoris());

    Notification n;
    n.setPreviewBody(VI18N.get("favoris.message.ajoute"));
    n.setHintValue("x-nemo-icon", "image://theme/icon-favorite");
    n.publish();

    emit listeChanged();

    return f2->getUuid();
}

/**
 * @brief suprimme le favoris avec l'uuid uuid
 * @param uuid
 */
void favorisService::supprimer(const qint64 uuid) {

    quint64 index = 0;
    foreach (RowFavoris *f, liste) {
        if (f->getUuid()==uuid ) {

            dao.supprimer(f->getChemin());

            // NOTA : il ne faut pas recharger la liste car les uuid seront
            // mis à jour --> pb dans la vue "favoris"
            delete(f);
            liste.removeAt(index);

            Notification n;
            n.setPreviewBody(VI18N.get("favoris.message.supprime"));
            n.setHintValue("x-nemo-icon", "image://theme/icon-m-delete");
            n.publish();

            emit listeChanged();
            return;
        }
        index++;
    }
}

/**
 * @brief teste si un chemin virtuel est déjà un favoris
 * @return uuid de l'item ou -1 si pas dans la liste
 */
qint64 favorisService::getFavoris(const QString vurl) {
    foreach (RowFavoris *item, liste) {
        if( item->getChemin().compare(vurl) == 0) {
            return item->getUuid();
        }
    }

    return -1;
}

/**
 * @brief charge la liste des favoris
 */
void favorisService::recharger() {
    qDebug() << "favorisService::load";

    if(liste.isEmpty()==false) {
        qDeleteAll(liste);
    }

    liste.clear();

    QList<RowFavoris *> l = dao.liste();
    foreach (RowFavoris *f, l) {
        if(VFI.exist(f->getChemin())==false) {
            continue;
        }

        RowFavoris *f2 = new RowFavoris();
        f2->setChemin(f->getChemin());
        f2->setNom(f->getNom());
        f2->setCreation(f->getCreation());
        f2->setDuree(f->getDuree());
        f2->setPiste(f->getPiste());
        liste.append(f2);
    }

    classer(configService::getInstance().getOrdreFavoris());

    emit listeChanged();
}

/**
 * @brief importer l'ancienne liste de favoris dans la nouvelle base
 */
void favorisService::importer() {
    QSettings settings;
    QList<QString> l = settings.value("favoris/liste").value<QList<QString> >();
    if(l.length()==0) {
    }

    foreach (QString str, l) {
        if( getFavoris(str) ) {
            continue;
        }

        if( VFI.exist(str) == false ) {
            continue;
        }

        // importe le favoris
        ajouter(str);
    }

    l.clear();

    settings.remove("favoris/liste");
    settings.sync();
}

/**
 * @brief sauve la liste des favoris
 */
void favorisService::sauver() {
    return;


    //    QList<QString> l;
    //    foreach (RowFavoris *item, liste) {
    //        l.append(item->getLabel());
    //    }

    //    settings.setValue(FAVORISLISTE, QVariant::fromValue(l));
    //    settings.sync();
}

/**
 * @brief classe la liste en fonction de config, cree liste_classe
 */
void favorisService::classer(const quint16 ordre) {

    switch(ordre) {
    case 2:
        std::sort(liste.begin(), liste.end(), favorisService::ordreAlpha);
        break;
    case 1:
        //     for(int k = 0; k < (liste.size()/2); k++) liste.swap(k,liste.size()-(1+k));
        std::sort(liste.begin(), liste.end(), favorisService::ordreInv);
        break;

    default:
        std::sort(liste.begin(), liste.end(), favorisService::ordreDate);
        break;
    }
}

/**
  * @brief favorisService::ordreAlpha
  * @param a
  * @param b
  * @return
  */
bool favorisService::ordreAlpha(  RowFavoris * a, RowFavoris  * b ) {
    return a->getNom().compare( b->getNom() ) < 0 ? true : false;
}

/**
 * @brief favorisService::ordreDate
 * @param a
 * @param b
 * @return
 */
bool favorisService::ordreDate(  RowFavoris * a, RowFavoris  * b ) {
    return b->getCreation() < a->getCreation();
}

/**
  * @brief favorisService::ordreInv
  * @param a
  * @param b
  * @return
  */
bool favorisService::ordreInv(  RowFavoris * a, RowFavoris  * b ) {
    return b->getCreation() > a->getCreation();
}

/**
 * @brief favorisService::ordreFavorisImpl
 */
void favorisService::ordreFavorisImpl() {
    if(liste.length()==0) {
        return;
    }
    classer(configService::getInstance().getOrdreFavoris());
    emit listeChanged();
}

/**
 * @brief favorisService::onRemoved
 * @param vurl
 */
void favorisService::onRemoved(QString vurl) {

    // on peut avoir supprimé le rep / album parent d'un favoris
    QStack<quint64> stack;

    // on recherche les index
    quint64 index = 0;
    foreach (RowFavoris *f, liste) {
        if ( f->getChemin().startsWith(vurl) ) {
            stack.push(index);
        }
        index++;
    }

    // on supprime en lifo
    while( stack.isEmpty() == false) {
        quint64 val = stack.pop();

        RowFavoris *f = liste.at(val);
        dao.supprimer(f->getChemin());

        // NOTA : il ne faut pas recharger la liste car les uuid seront
        // mis à jour --> pb dans la vue "favoris"
        delete(f);
        liste.removeAt(val);
    }
}

/**
  * @brief recentService::onMoved
  * @param vurl
  * @param into
  */
void favorisService::onMoved(QString vurl, QString as) {

    // pour les favoris on se fout du renommage de fichier
    if(VFI.isDirectory(as) == false) {
        return;
    }

    quint16 nb = 0;
    foreach (RowFavoris *f, liste) {
        if ( f->getChemin().startsWith(vurl) ) {

            QString target = f->getChemin().replace(vurl, as);
            QString nom = f->getNom();

            // @INFO : on peut déplacer ou renommer un rep parent de f
            // il faut verifier le nom et eventuellement le modifier
            if( f->getChemin().compare(vurl) == 0 ) {
                // c'est l'url directe qui est modifié
                nom = VFI.getName(target).toLower();

                // on recup la meta de l'album / premier titre
                MetaData *m = MetaDataService::getInstance().getMetaData(target,true);
                if(m != NULL) {
                    if(m->getAlbum().isEmpty()==false && m->getArtiste().isEmpty() == false) {
                        nom = m->getArtiste() + " - " + m->getAlbum();
                    }
                }
            }

            dao.supprimer(f->getChemin());
            dao.ajouter(nom,target,f->getDuree(),f->getPiste());

            //delete(f);
            nb++;
        }
    }

    if(nb >0) {
        recharger();
    }
}

/**
  * @brief recentService::onRenamed
  * @param vurl
  * @param name
  */
void favorisService::onRenamed(QString vurl, QString name) {
    QString p = VFI.getParentVUrl(vurl);
    QString as  = p + (VFI.isRoot(p) ? "" : "/") + name;
    onMoved(vurl, as);
}

