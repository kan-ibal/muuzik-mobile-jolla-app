#include "recherche.h"
#include <QDebug>
#include "indexeur.h"
#include "artwork.h"

#include "../virtualfiles/VFservice.h"

rechercheService::rechercheService(QObject *parent) : QObject(parent), taille(DEF_RECH_TAILLE) {
}

rechercheService::~rechercheService() {
    qDeleteAll(liste);
    liste.clear();
}

/**
 * @brief rechercheService::getInstance
 * @return
 */
rechercheService& rechercheService::getInstance() {
    static rechercheService instance;
    return instance;
}

/**
 * @brief rechercheService::init
 * @param context
 * @param name
 */
void rechercheService::init(QQmlContext *context,QString name) {
    context->setContextProperty(name, &rechercheService::getInstance());
}

/**
  * @brief recherche des enregistrement
  * @param str chaine de recherche
  * @param max nb max d'enregistrement
  */
QList<ResultItem *> rechercheService::rechercher(QString const &str) {

    if(IndexeurService::getInstance().estEnCours() || (IndexeurService::getInstance().getDbok()==false)) {
        return liste;
    }

    qDeleteAll(liste);
    liste.clear();

    qDebug() << "on recherche : " << str;

    // on prend de la marge car on s'attend à avoir des fauts resultats
    QList<RowIndexeur *> s = dao.rechercher(str,taille*2);
    if( s.length()==0) {
        emit listeChanged();
        return liste;
    }

    quint64 nb = 0;
    foreach (RowIndexeur *d, s) {
        if(VFI.exist(d->getChemin())==false) {
            continue;
        }

        ResultItem *r=new ResultItem();
        r->setName(d->getNom());
        r->setVurl(d->getChemin());
        r->setType(d->getFormat()==0 ? VFInfo::VFType::DIRECTORY : VFInfo::VFType::FILE );

        liste.append(r);

        // on a notre quota
        if( nb++ > taille) {
            break;
        }
    }

    qDeleteAll(s);

    emit listeChanged();

    return liste;
}

/**
  *@brief efface la rechercher
  */
void rechercheService::vider() {
    if(liste.length()==0) {
        return;
    }

    qDeleteAll(liste);
    liste.clear();
    emit listeChanged();
}

/**
 * @brief rechercheService::setTaille
 * @param v
 */
void rechercheService::setTaille(quint64 v) {
    taille=v;
    emit tailleChanged();
}

/**
  * @brief recherche un titre aleatoirement
  * @parm type, -1 pour tous,0 pour rep, 1 pour audio
  * @return info du titre
  */
ResultItem *rechercheService::rechercheAleatoire(qint8 support) {

    QScopedPointer<RowIndexeur> d(dao.rechercheAleatoire(support));

    ResultItem *r=new ResultItem();
    r->setName(d->getNom());
    r->setVurl(d->getChemin());
    r->setType(d->getFormat()==0 ? VFInfo::VFType::DIRECTORY : VFInfo::VFType::FILE );

    return r;
}
