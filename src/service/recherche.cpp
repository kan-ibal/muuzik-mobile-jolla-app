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

    qDeleteAll(liste);
    liste.clear();

    QString query = str.trimmed();
    if(query.isEmpty()) {
        emit listeChanged();
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
        // 1. Search for Albums (directories) whose album name, artist, or keywords match the query
        QSqlQuery q = db.createQuery();
        if(q.exec("SELECT nom, chemin, motscles FROM Item WHERE format = 0")) {
            while(q.next()) {
                QString nom = q.value(0).toString();
                QString chemin = q.value(1).toString();
                QString motscles = q.value(2).toString();

                if(!VFI.exist(chemin)) {
                    continue;
                }

                QString albumName;
                QString artistName;

                MetaData *m = MetaDataService::getInstance().getMetaData(chemin, true);
                if(m != NULL) {
                    albumName = m->getAlbum();
                    artistName = m->getArtiste();
                }

                if(albumName.isEmpty()) {
                    AlbumStat *stat = MetaDataService::getInstance().getStatFromAlbum(chemin);
                    if(stat != NULL) {
                        albumName = stat->getAlbum();
                        if(artistName.isEmpty()) {
                            artistName = stat->getArtiste();
                        }
                    }
                }

                bool match = false;
                if(!albumName.isEmpty() && albumName.contains(query, Qt::CaseInsensitive)) {
                    match = true;
                } else if(!albumName.isEmpty() && !artistName.isEmpty() &&
                          (artistName + " " + albumName).contains(query, Qt::CaseInsensitive)) {
                    match = true;
                } else if(nom.contains(query, Qt::CaseInsensitive)) {
                    match = true;
                } else if(motscles.contains(query.toLower())) {
                    match = true;
                } else if(tokens.length() > 0) {
                    QString haystack = (albumName + " " + artistName + " " + nom + " " + motscles).toLower();
                    bool allMatch = true;
                    foreach(const QString &tok, tokens) {
                        if(tok.length() >= 2 && !haystack.contains(tok.toLower())) {
                            allMatch = false;
                            break;
                        }
                    }
                    if(allMatch) {
                        match = true;
                    }
                }

                if(match && !addedVurls.contains(chemin)) {
                    addedVurls.insert(chemin);

                    QString displayName;
                    if(!artistName.isEmpty() && !albumName.isEmpty()) {
                        displayName = artistName + " - " + albumName;
                    } else if(!albumName.isEmpty()) {
                        displayName = albumName;
                    } else {
                        displayName = nom;
                    }

                    ResultItem *r = new ResultItem();
                    r->setName(displayName);
                    r->setVurl(chemin);
                    r->setType(VFInfo::VFType::DIRECTORY);
                    albumResults.append(r);

                    if((quint64)albumResults.length() >= taille) {
                        break;
                    }
                }
            }
        }

        // 2. Search Favoris and Historique for matching album names if quota not reached
        if((quint64)albumResults.length() < taille) {
            QSqlQuery qFav = db.createQuery();
            qFav.prepare("SELECT nom, chemin FROM Favoris WHERE nom LIKE ? LIMIT 10");
            qFav.bindValue(0, "%" + query + "%");
            if(qFav.exec()) {
                while(qFav.next()) {
                    QString chemin = qFav.value(1).toString();
                    if(!addedVurls.contains(chemin) && VFI.exist(chemin)) {
                        addedVurls.insert(chemin);
                        ResultItem *r = new ResultItem();
                        r->setName(qFav.value(0).toString());
                        r->setVurl(chemin);
                        r->setType(VFInfo::VFType::DIRECTORY);
                        albumResults.append(r);
                    }
                }
            }
        }

        if((quint64)albumResults.length() < taille) {
            QSqlQuery qHist = db.createQuery();
            qHist.prepare("SELECT nom, chemin FROM Historique WHERE nom LIKE ? LIMIT 10");
            qHist.bindValue(0, "%" + query + "%");
            if(qHist.exec()) {
                while(qHist.next()) {
                    QString chemin = qHist.value(1).toString();
                    if(!addedVurls.contains(chemin) && VFI.exist(chemin)) {
                        addedVurls.insert(chemin);
                        ResultItem *r = new ResultItem();
                        r->setName(qHist.value(0).toString());
                        r->setVurl(chemin);
                        r->setType(VFInfo::VFType::DIRECTORY);
                        albumResults.append(r);
                    }
                }
            }
        }
    }

    // 3. Search via DaoIndexeur (standard keyword search for tracks and items)
    QList<RowIndexeur *> s = dao.rechercher(query, taille * 2);
    foreach (RowIndexeur *d, s) {
        if(addedVurls.contains(d->getChemin()) || VFI.exist(d->getChemin()) == false) {
            continue;
        }

        addedVurls.insert(d->getChemin());

        ResultItem *r = new ResultItem();
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

    // 4. Combine results: Albums first, then tracks/items up to taille limit
    quint64 totalAdded = 0;
    foreach(ResultItem *r, albumResults) {
        if(totalAdded < taille) {
            liste.append(r);
            totalAdded++;
        } else {
            delete r;
        }
    }

    foreach(ResultItem *r, itemResults) {
        if(totalAdded < taille) {
            liste.append(r);
            totalAdded++;
        } else {
            delete r;
        }
    }

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
