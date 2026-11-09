#include "recherche.h"
#include <QDebug>
#include <QRegularExpression>
#include <QSet>
#include <QSqlQuery>
#include "indexeur.h"
#include "artwork.h"
#include "metadataservice.h"
#include "../util/db.h"

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

    QString query = str.trimmed();
    if(query.isEmpty()) {
        vider();
        return liste;
    }

    qDebug() << "on recherche : " << query;

    QSet<QString> addedVurls;
    QList<ResultItem *> albumResults;
    QList<ResultItem *> itemResults;

    static const QRegularExpression separateur("[ -;.:,_]");
    QStringList tokens = query.split(separateur, QString::SkipEmptyParts);

    DB &db = DB::getInstance();
    if(db.isOpen()) {
        // 1. Search for Albums (directories, format = 0) matching the query
        QSqlQuery qAlbum = db.createQuery();
        qAlbum.prepare("SELECT nom, chemin, motscles FROM Item WHERE format = 0 AND (motscles LIKE ? OR nom LIKE ?) LIMIT ?");
        qAlbum.bindValue(0, "%" + query.toLower() + "%");
        qAlbum.bindValue(1, "%" + query + "%");
        qAlbum.bindValue(2, (qulonglong)taille);

        if(qAlbum.exec()) {
            while(qAlbum.next()) {
                QString nom = qAlbum.value(0).toString();
                QString chemin = qAlbum.value(1).toString();

                if(!VFI.exist(chemin) || addedVurls.contains(chemin)) {
                    continue;
                }
                addedVurls.insert(chemin);

                QString displayName = nom;
                MetaData *m = MetaDataService::getInstance().getMetaData(chemin, true);
                if(m != NULL && !m->getAlbum().isEmpty()) {
                    if(!m->getArtiste().isEmpty()) {
                        displayName = m->getArtiste() + " - " + m->getAlbum();
                    } else {
                        displayName = m->getAlbum();
                    }
                }

                ResultItem *r = new ResultItem(this);
                r->setName(displayName);
                r->setVurl(chemin);
                r->setType(VFInfo::VFType::DIRECTORY);
                albumResults.append(r);
            }
        }
        qAlbum.finish();

        // If tokens > 1 and quota not reached, also search with token conjunction
        if((quint64)albumResults.length() < taille && tokens.length() > 1) {
            QStringList conds;
            for(int i = 0; i < tokens.length(); ++i) {
                conds.append("motscles LIKE ?");
            }
            QSqlQuery qTok = db.createQuery();
            qTok.prepare("SELECT nom, chemin, motscles FROM Item WHERE format = 0 AND (" + conds.join(" AND ") + ") LIMIT ?");
            for(int i = 0; i < tokens.length(); ++i) {
                qTok.bindValue(i, "%" + tokens.at(i).toLower() + "%");
            }
            qTok.bindValue(tokens.length(), (qulonglong)(taille - albumResults.length()));
            if(qTok.exec()) {
                while(qTok.next()) {
                    QString nom = qTok.value(0).toString();
                    QString chemin = qTok.value(1).toString();

                    if(!VFI.exist(chemin) || addedVurls.contains(chemin)) {
                        continue;
                    }
                    addedVurls.insert(chemin);

                    QString displayName = nom;
                    MetaData *m = MetaDataService::getInstance().getMetaData(chemin, true);
                    if(m != NULL && !m->getAlbum().isEmpty()) {
                        if(!m->getArtiste().isEmpty()) {
                            displayName = m->getArtiste() + " - " + m->getAlbum();
                        } else {
                            displayName = m->getAlbum();
                        }
                    }

                    ResultItem *r = new ResultItem(this);
                    r->setName(displayName);
                    r->setVurl(chemin);
                    r->setType(VFInfo::VFType::DIRECTORY);
                    albumResults.append(r);
                }
            }
            qTok.finish();
        }

        // 2. Search Favoris for matching album names if quota not reached
        if((quint64)albumResults.length() < taille) {
            QSqlQuery qFav = db.createQuery();
            qFav.prepare("SELECT nom, chemin FROM Favoris WHERE nom LIKE ? LIMIT ?");
            qFav.bindValue(0, "%" + query + "%");
            qFav.bindValue(1, (qulonglong)(taille - albumResults.length()));
            if(qFav.exec()) {
                while(qFav.next()) {
                    QString chemin = qFav.value(1).toString();
                    if(!addedVurls.contains(chemin) && VFI.exist(chemin)) {
                        addedVurls.insert(chemin);
                        ResultItem *r = new ResultItem(this);
                        r->setName(qFav.value(0).toString());
                        r->setVurl(chemin);
                        r->setType(VFInfo::VFType::DIRECTORY);
                        albumResults.append(r);
                    }
                }
            }
            qFav.finish();
        }

        // 3. Search Historique for matching album names if quota not reached
        if((quint64)albumResults.length() < taille) {
            QSqlQuery qHist = db.createQuery();
            qHist.prepare("SELECT nom, chemin FROM Historique WHERE nom LIKE ? LIMIT ?");
            qHist.bindValue(0, "%" + query + "%");
            qHist.bindValue(1, (qulonglong)(taille - albumResults.length()));
            if(qHist.exec()) {
                while(qHist.next()) {
                    QString chemin = qHist.value(1).toString();
                    if(!addedVurls.contains(chemin) && VFI.exist(chemin)) {
                        addedVurls.insert(chemin);
                        ResultItem *r = new ResultItem(this);
                        r->setName(qHist.value(0).toString());
                        r->setVurl(chemin);
                        r->setType(VFInfo::VFType::DIRECTORY);
                        albumResults.append(r);
                    }
                }
            }
            qHist.finish();
        }
    }

    // 4. Search via DaoIndexeur (standard keyword search for tracks and items)
    QList<RowIndexeur *> s = dao.rechercher(query, taille * 2);
    foreach (RowIndexeur *d, s) {
        if(d == NULL) {
            continue;
        }
        if(addedVurls.contains(d->getChemin()) || VFI.exist(d->getChemin()) == false) {
            continue;
        }

        addedVurls.insert(d->getChemin());

        ResultItem *r = new ResultItem(this);
        QString displayName = d->getNom();

        if(d->getFormat() == 0) {
            // For directories/albums, display actual album name if available
            MetaData *m = MetaDataService::getInstance().getMetaData(d->getChemin(), true);
            if(m != NULL && m->getAlbum().length() > 0) {
                if(m->getArtiste().length() > 0) {
                    displayName = m->getArtiste() + " - " + m->getAlbum();
                } else {
                    displayName = m->getAlbum();
                }
            }
            r->setType(VFInfo::VFType::DIRECTORY);
        } else {
            r->setType(VFInfo::VFType::FILE);
        }

        r->setName(displayName);
        r->setVurl(d->getChemin());
        itemResults.append(r);
    }
    qDeleteAll(s);

    // 5. Combine results: Albums first, then tracks/items up to taille limit
    QList<ResultItem *> newResults;
    quint64 totalAdded = 0;
    foreach(ResultItem *r, albumResults) {
        if(totalAdded < taille) {
            newResults.append(r);
            totalAdded++;
        } else {
            delete r;
        }
    }

    foreach(ResultItem *r, itemResults) {
        if(totalAdded < taille) {
            newResults.append(r);
            totalAdded++;
        } else {
            delete r;
        }
    }

    QList<ResultItem *> toDelete = liste;
    liste = newResults;
    emit listeChanged();
    foreach(ResultItem *r, toDelete) {
        if(r) {
            r->deleteLater();
        }
    }

    return liste;
}

/**
  *@brief efface la rechercher
  */
void rechercheService::vider() {
    if(liste.length()==0) {
        return;
    }

    QList<ResultItem *> toDelete = liste;
    liste.clear();
    emit listeChanged();
    foreach(ResultItem *r, toDelete) {
        if(r) {
            r->deleteLater();
        }
    }
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
    if(d.isNull() || d->getChemin().isEmpty()) {
        return NULL;
    }

    ResultItem *r=new ResultItem();
    QString displayName = d->getNom();
    if(d->getFormat() == 0) {
        MetaData *m = MetaDataService::getInstance().getMetaData(d->getChemin(), true);
        if(m != NULL && m->getAlbum().length() > 0) {
            if(m->getArtiste().length() > 0) {
                displayName = m->getArtiste() + " - " + m->getAlbum();
            } else {
                displayName = m->getAlbum();
            }
        }
    }

    r->setName(displayName);
    r->setVurl(d->getChemin());
    r->setType(d->getFormat()==0 ? VFInfo::VFType::DIRECTORY : VFInfo::VFType::FILE );

    return r;
}
