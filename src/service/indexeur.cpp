#include "indexeur.h"
#include <QtQml>
#include <QMetaType>
#include <QQmlEngine>
#include <QDebug>
#include <QScopedPointer>
#include <QList>
#include <QFuture>
#include <QtConcurrent/QtConcurrentRun>
#include <QCoreApplication>
#include <QtAlgorithms>

#include "../app.h"
#include "config.h"
#include "../util/daoindexeur.h"
#include "metadataservice.h"

#include "../vi18n/vi18n.h"
#include "../virtualfiles/VFservice.h"

#include <nemonotifications-qt5/notification.h>

const QRegularExpression IndexeurService::REG_fichieraudio( VFService::FILTERAUDIO, QRegularExpression::CaseInsensitiveOption);

IndexeurService::IndexeurService(QObject *parent) : QObject(parent), enCours(false), dbok(false) {
    connect(this,SIGNAL(rechargementFaitIntr()), this, SLOT(finRechargementImpl()));
    connect(this,SIGNAL(miseAJoursFaiteIntr(quint64)), this, SLOT(finVerifierMiseAJourImpl(quint64)));

    connect(&VFService::getInstance(), SIGNAL(removed(QString)),this, SLOT(onRemoved(QString)));
    connect(&VFService::getInstance(), SIGNAL(moved(QString,QString)), this, SLOT(onMoved(QString,QString)));
    connect(&VFService::getInstance(), SIGNAL(renamed(QString,QString)), this, SLOT(onRenamed(QString,QString)));

    DaoIndexeur dao;
    dbok = dao.taille()>0;
    if(dbok == false) {
        recharger();
        return;
    }

    // on a une db, mais est elle à jour
    //verifierMiseaJour();
}

/**
 * @brief rechercheService::getInstance
 * @return
 */
IndexeurService& IndexeurService::getInstance() {
    static IndexeurService instance;
    return instance;
}

/**
 * @brief indexeurService::init
 * @param context
 * @param name
 */
void IndexeurService::init(QQmlContext *context,QString name) {
    context->setContextProperty(name, &IndexeurService::getInstance());

    qmlRegisterType<RowIndexeur>("RowIndexeur", 1, 0, "RowIndexeur");
    qRegisterMetaType<RowIndexeurListe>( "RowIndexeurListe" );
}

/**
 * @brief IndexeurService::setEncours
 * @param v
 * @return
 */
void IndexeurService::setEncours(bool v) {
    enCours = v;
    emit enCoursChanged();
}

/**
 * @brief lancement du rechargement de la db (lancement du thread/future)
 */
void IndexeurService::recharger() {

    if(enCours == true) {
        return;
    }

    setEncours(true);

    dbok = false;
    emit dbOkChanged();

    // lancement du scan
    QFuture<void> f = QtConcurrent::run(doRechargement);
}

/**
 * @brief fait le rechargement
 */
void IndexeurService::doRechargement() {
    DaoIndexeur dao;

    dao.effacer();

    IndexeurService::recurRechargement("/");

    //ok, fin
    emit IndexeurService::getInstance().rechargementFaitIntr();
}

/**
 * @brief indexeurService::finRechargementImpl
 */
void IndexeurService::finRechargementImpl() {
    setEncours(false);
    dbok=true;

    // date de dernière mise à jour
    QSettings settings;
    settings.setValue(INDEXEUR_DATE, QDateTime::currentMSecsSinceEpoch());
    settings.sync();

    qDebug() << "date mise à jour : " <<  QDateTime::currentMSecsSinceEpoch();

    Notification n;
    n.setPreviewBody(vI18n::getInstance().get( "recherche.db.ok" ));
    n.setHintValue("x-nemo-icon", "image://theme/icon-lock-information");
    n.publish();

    emit dbOkChanged();
}

/**
 * @brief recur
 * @param vurl vers le repertoire à scanner
 */
void IndexeurService::recurRechargement(const QString &vurl) {
    QList<RowIndexeur *> l;

    QList<VFInfo *> lf = VFI.getSynch(vurl, VFService::FILTERNONE, VFService::VFType::BOTH, VFService::VFOrder::NONE);
    if(lf.length()==0) {
        return;
    }

    foreach (VFInfo *f, lf) {

        if(f->getType() == VFInfo::VFType::FILE) {
            if(REG_fichieraudio.match(f->getFilename()).hasMatch()==false) {
                continue;
            }
        }

        if(f->getType() == VFInfo::VFType::DIRECTORY) {
            recurRechargement(f->getVUrl());

            if(VFI.countFiles(f->getVUrl(), VFService::FILTERAUDIO) == 0) {
                continue;
            }
        }

        RowIndexeur *r=new RowIndexeur();
        r->setChemin(f->getVUrl());
        r->setFormat(f->getType() == VFInfo::VFType::DIRECTORY ? 0 : 1);

        MetaData *m = MetaDataService::getInstance().getMetaData(f->getVUrl(),true);
        if(m == NULL) {
            // si pas de meta on utilise le non du rep / titre
            r->setNom(f->getName());
            r->setMotsCles(r->getNom());

        } else {
            switch (f->getType()) {
            case VFInfo::VFType::DIRECTORY :
                r->setNom(f->getName());

                // si les info sont dispo, on recherche un album par titre et artiste
                if(m->getAlbum().length()>0 && m->getArtiste().length()>0) {
                    r->setMotsCles( m->getAlbum() + " " + m->getArtiste() );
                    break;
                }

                // on a pas les infos
                r->setMotsCles(r->getNom());
                break;

            case VFInfo::VFType::FILE:
                // pour les titre on ne recerche que dans le nom du titre
                // si on ajoutait artite + album, on se retrouve avec trop de hit
                r->setNom( m->getTitre().length()>0 ? m->getTitre() : f->getName());
                r->setMotsCles(r->getNom());
                break;

            default:
                // fail safe
                delete r;
                continue;
            }
        }

       // qDebug() << "IndexeurService, indexing "<< f->getType() << " // " << r->getChemin() << " ## " << r->getMotsCles();
        l.append(r);
    }

    qDeleteAll(lf);

    if(l.size()>0) {
        DaoIndexeur dao;
        dao.ajouter(l);
        qDeleteAll(l);

    }
}

/**
 * @brief teste si doit faire une mise à jour de la db
 * // si oui, enchaine sur un rechargement
 * @return
 */
void IndexeurService::verifierMiseaJour() {
    if(enCours) {
        return;
    }

    setEncours(true);

    QFuture<void> f = QtConcurrent::run(doVerifierMiseAJour);
}

/**
 * @brief
 * @return
 */
void IndexeurService::doVerifierMiseAJour() {

    quint64 timestamp = IndexeurService::recurMiseAJour("/");
    emit IndexeurService::getInstance().miseAJoursFaiteIntr(timestamp);
}

/**
 * @brief IndexeurService::finVerifierMiseAJourImpl
 */
void IndexeurService::finVerifierMiseAJourImpl(quint64 timestamp) {
    setEncours(false);

    // on verifie avec la valeur sauvé lors du précedent scan
    if(timestamp==0) {
        // rep vide !
        return;
    }

    QSettings settings;
    quint64 d=settings.value(INDEXEUR_DATE,INDEXEUR_DATE_DEF).toLongLong();

    qDebug() << "Date mise à jour precedente : " << d << " - " << timestamp;

    if(timestamp<d) {
        // on a déjà fait une mise à jour plus récente
        return;
    }

    // il faut faire une mise à jour
    recharger();
}

/**
 * @brief recurMiseAJour
 * @param vurl
 * @return
 */
quint64 IndexeurService::recurMiseAJour(const QString &vurl) {

    quint64 timestamp = 0;

    QList<VFInfo *> lf = VFI.getSynch(vurl, VFService::FILTERNONE, VFService::VFType::DIR, VFService::VFOrder::DATE);
    if(lf.length()==0) {
        return timestamp;
    }

    foreach (VFInfo *f, lf) {

        if(f->getType() != VFInfo::VFType::DIRECTORY) {
            continue;
        }

        timestamp = f->getTimestamp() > timestamp ? f->getTimestamp() : timestamp;

        quint64 tmp = recurMiseAJour(f->getVUrl());
        timestamp = tmp > timestamp ? tmp : timestamp;
    }
    qDeleteAll(lf);

//    qDebug() << "IndexeurService, recent ?  "<< vurl << " // " << timestamp;

    return timestamp;
}

/**
 * @brief suppression de l'url ( et de tout les childs)
 * @param vurl
 */
void IndexeurService::onRemoved(QString vurl) {
qDebug() << "IndexeurService::onRemoved";
    if(enCours) {
        return;
    }

    DaoIndexeur dao;
    dao.effacerParVUrl(vurl);
}

/**
 * @brief IndexeurService::onMoved
 * @param vurl
 * @param as
 */
void IndexeurService::onMoved(QString vurl, QString as) {

    if(enCours) {
        return;
    }

    DaoIndexeur dao;

    // si simple audio, plus simple...
    if(VFI.isFile(as)) {
        dao.effacerParVUrl(vurl);

        RowIndexeur *r=new RowIndexeur();
        r->setChemin(vurl);
        r->setFormat(1);

        MetaData *m = MetaDataService::getInstance().getMetaData(as,true);
        if(m == NULL) {
            // si pas de meta on utilise le non du rep / titre
            r->setNom(VFI.getName(as));
            r->setMotsCles(r->getNom());

        } else {
            r->setNom( m->getTitre().length()>0 ? m->getTitre() : r->getNom());
            r->setMotsCles(r->getNom());
        }

        dao.ajouter(r);
        delete r;
        return;
    }

    // bah non, c'est un rep
    QList<RowIndexeur *> l = dao.rechercherParVurl(vurl,INDEXEUR_THRESHOLD_REFRESH);
    if(l.length()>=INDEXEUR_THRESHOLD_REFRESH) {
        recharger();
        return;
    }

    // peu de lignes, on update directement
    dao.effacerParVUrl(vurl);

    // gros hack : recurRechargement ne traite pas l'url passé
    // car la méthode s'utilise originialement à partir de root
    // au lieu de réécrire la methode, on ajoute simplement
    // le rep a la db (s'il existe, e.i modif directement sur ce rep)

    foreach (RowIndexeur *r, l) {
        if(r->getChemin().compare(vurl) ==0) {
            r->setChemin(as);
            dao.ajouter(r);
            break;
        }
    }

    qDeleteAll(l);

    recurRechargement(as);
}

/**
 * @brief IndexeurService::onRenamed
 * @param vurl
 * @param name
 */
void IndexeurService::onRenamed(QString vurl, QString name) {

    QString p = VFI.getParentVUrl(vurl);
    QString as  = p + (VFI.isRoot(p) ? "" : "/") + name;
    onMoved(vurl,as);
}
