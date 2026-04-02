#include "historique.h"
#include <QtQml>
#include <QMetaType>
#include <QQmlEngine>
#include <QDateTime>
#include <QFileInfo>
#include <QtAlgorithms>
#include <QDateTime>
#include <QStack>

#include "config.h"
#include "artwork.h"
#include "metadataservice.h"

//#include "../vi18n/vi18n.h"
#include "../virtualfiles/VFservice.h"

HistoriqueService::HistoriqueService(QObject *parent) : QObject(parent) {

    connect(&VFService::getInstance(), SIGNAL(removed(QString)),this, SLOT(onRemoved(QString)));
    connect(&VFService::getInstance(), SIGNAL(moved(QString,QString)), this, SLOT(onMoved(QString,QString)));
    connect(&VFService::getInstance(), SIGNAL(renamed(QString,QString)), this, SLOT(onRenamed(QString,QString)));

    rafraichir();
}

/**
 * @brief HistoriqueService::getInstance
 * @return
 */
HistoriqueService& HistoriqueService::getInstance() {
    static HistoriqueService instance;
    return instance;
}

/**
 * @brief HistoriqueService::init
 * @param context
 * @param name
 */
void HistoriqueService::init(QQmlContext *context,QString name) {
    context->setContextProperty(name, &HistoriqueService::getInstance());
    qmlRegisterType<RowHistorique>("RowHistorique", 1, 0, "RowHistorique");
    qRegisterMetaType<RowHistoriqueListe>( "RowHistoriqueListe" );
}

/**
 * @brief HistoriqueService::ajouter
 * @param v
 */
void HistoriqueService::ajouter(const QString &vurl) {
    qDebug() << "HISTORY ADDED : " << vurl;
    if(configService::getInstance().getHistorique()==false) {
        return;
    }

    if(VFI.exist(vurl)==false) {
        return;
    }

    // on n'ajoute pas les medias seuls
    if(VFI.isDirectory(vurl) ==false) {
        return;
    }

    // netoyage historique
    qint64 d = QDateTime::currentMSecsSinceEpoch()/1000;
    switch (configService::getInstance().getModeHistorique()) {
    case 0:
        d -= 604800;
        dao.effacer(d);
        break;
    case 1 :
        d -= 2592000;
        dao.effacer(d);
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
    dao.ajouter(nom, vurl, stat->getDuree(),stat->getPistes());

    // ici on peut faire un full refresh car on ne supprimer pas une ligne individellement
    rafraichir();
}

/**
  * @brief supprime l'entrée de la liste d'historique
  * @param uuid : uuid dans la liste
  */
void HistoriqueService::supprimer(quint64 uuid) {

    for(quint64 i=0; i < liste.length() ; i++) {
        RowHistorique *f = liste.at(i);

        if ( f->getUuid() == uuid ) {
            dao.supprimer(f->getChemin());

            // NOTA : il ne faut pas recharger la liste car les uuid seront
            // mis à jour --> pb dans la vue "historique"
            delete(f);
            liste.removeAt(i);

            return;
        }
    }
}

/**
 * @brief HistoriqueService::effacer
 */
void HistoriqueService::vider() {
    if(liste.length()==0) {
        return;
    }

    dao.effacer();
    liste.clear();
    emit listeChanged();
}

/**
 * @brief HistoriqueService::rafraichir
 */
void HistoriqueService::rafraichir() {

    if(liste.isEmpty()==false) {
        qDeleteAll(liste);
    }
    liste.clear();

    QList<RowHistorique *> s = dao.liste(0);
    if( s.length()==0) {
        emit listeChanged();
    }

    foreach (RowHistorique *d, s) {
        RowHistorique *r=new RowHistorique();
        r->setChemin(d->getChemin());
        r->setNom(d->getNom());
        r->setDuree(d->getDuree());
        r->setNombre(d->getNombre());
        r->setPiste(d->getPiste());
        r->setOuverture(d->getOuverture());
        liste.append(r);
    }

    // pas besoin fait depuis la db
    //    std::sort(liste.begin(), liste.end(), HistoriqueService::ordreDate);

    emit listeChanged();
}

/**
 * @brief HistoriqueService::ordreDate
 * @param a
 * @param b
 * @return
 */
bool HistoriqueService::ordreDate( RowHistorique *a, RowHistorique  *b ) {
    return b->getOuverture() < a->getOuverture();
}


/**
  * @brief formate une date
  * @param t le temps en secondes
  * @return chaîne formaté
  */
QString HistoriqueService::formatTime(quint64 t) {
    return QDateTime::fromMSecsSinceEpoch(t*1000).toString("dd MMM hh:mm");
}

/**
 * @brief HistoriqueService::onRemoved
 * @param vurl
 */
void HistoriqueService::onRemoved(QString vurl) {

    QStack<quint64> stack;

    // on recherche les index
    quint64 index = 0;
    foreach (RowHistorique *f, liste) {
        if ( f->getChemin().startsWith(vurl) ) {
            stack.push(index);
        }
        index++;
    }

    // on supprime en lifo
    while( stack.isEmpty() == false) {
        quint64 val = stack.pop();

        RowHistorique *f = liste.at(val);
        dao.supprimer(f->getChemin());

        // NOTA : il ne faut pas recharger la liste car les uuid seront
        // mis à jour --> pb dans la vue "historique"
        delete(f);
        liste.removeAt(val);
    }
}

/**
 * @brief favorisService::onMoved
 * @param vurl
 * @param as
 */
void HistoriqueService::onMoved(QString vurl, QString as) {

    // pour l'historique on se fout du renommage de fichier
    if(VFI.isDirectory(as) == false) {
        return;
    }

    quint16 nb = 0;

    foreach (RowHistorique *f, liste) {
        if ( f->getChemin().startsWith(vurl) ) {

            QString target =  f->getChemin().replace(vurl, as);
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
            dao.ajouter( nom, target, f->getDuree(),f->getPiste(), f->getOuverture(), f->getNombre());

            nb++;
        }
    }

    if(nb >0) {
        rafraichir();
    }
}

/**
 * @brief HistoriqueService::onRenamed
 * @param vurl
 * @param name
 */
void HistoriqueService::onRenamed(QString vurl, QString name) {
    QString p = VFI.getParentVUrl(vurl);
    QString as  = p + (VFI.isRoot(p) ? "" : "/") + name;
    onMoved(vurl, as);
}

