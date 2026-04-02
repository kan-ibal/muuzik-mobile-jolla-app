#include "daoindexeur.h"
#include <QSqlQuery>
#include <QDebug>
#include <QRegularExpression>
#include <QSqlError>

#include "db.h"

#define TAILLEMINMOT 3
#define NBMAXMOT 5

static QRegularExpression separateurrecherche("[ -;.:,_]");

/**
 * @brief Dao::Dao
 */
DaoIndexeur::DaoIndexeur(QObject *parent) : QObject(parent){

    DB &db = DB::getInstance();
    if(db.isOpen()==false) {
        qDebug() << "Can't create statement";
        return;
    }

    insertq = db.createStatement("INSERT INTO Item (nom,chemin,motscles,format) values (?,?,?,?)");
}

/**
 * @brief ajoute une ligne
 * @param nom
 * @param chemin
 * @param support
 * @param format
 */
void DaoIndexeur::ajouter(const QList<RowIndexeur *> & l) {
    DB &db = DB::getInstance();

    if(db.isOpen()==false) {
        return;
    }

    QVariantList noms;
    QVariantList chemins;
    QVariantList motscles;
    QVariantList formats;

    foreach (RowIndexeur *r, l) {
        noms << r->nom.toLower();
        chemins << r->chemin;
        motscles << r->motscles.toLower().replace(separateurrecherche, " ");
        formats << r->format;
    }

    QSqlQuery &q = *insertq;

    q.bindValue(0,noms);
    q.bindValue(1,chemins);
    q.bindValue(2,motscles);
    q.bindValue(3,formats);

    db.transaction();
    if(q.execBatch()==false) {
        qDebug() << "can't insert batch " << q.lastError().text();
    }
    db.commit();
}

/**
 * @brief ajouter
 * @param r
 */
void DaoIndexeur::ajouter(RowIndexeur *r) {

    QList<RowIndexeur *> l;
    l.append(r);
    ajouter(l);
}


/**
 * @brief efface le contenu de la db
 */
void DaoIndexeur::effacer() {
    DB &db = DB::getInstance();

    if(db.isOpen()==false) {
        return;
    }

    db.transaction();
    QSqlQuery q = db.createQuery();

    q.exec("DELETE FROM Item");
    q.exec("VACUUM");

    q.finish();
    db.commit();
}

/**
 * @brief efface les items qui ont leurs chemin qui COMMENCE par vurl
 * @param vurl
 */
void DaoIndexeur::effacerParVUrl(const QString &str) {

    DB &db = DB::getInstance();

    if(db.isOpen()==false) {
        return;
    }

    db.transaction();

    QSqlQuery q = db.createQuery();
    q.prepare("DELETE FROM Item WHERE chemin LIKE ? ESCAPE '\\'" );
    QString tmp = QString(str);
    q.bindValue(0,
                QString("%1%").arg(
                    tmp.replace("%", "\\%").replace("_", "\\_").replace("'", "\\'")
                    ));


    qDebug() << "DaoIndexeur::effacerParVUrl " <<  q.lastQuery();

    q.exec();
    //q.exec("VACUUM");

    q.finish();
    db.commit();
}

/**
 * @brief rechercher
 * @param str
 * @return
 */
QList<RowIndexeur *> DaoIndexeur::rechercher(const QString &str,quint64 max) {
    DB &db = DB::getInstance();

    QList<RowIndexeur *> l;

    if(db.isOpen()==false) {
        return l;
    }

    QStringList ml = str.split(separateurrecherche,QString::SkipEmptyParts );

    if(ml.length()==0) {
        return l;
    }

    QStringList sq;
    quint64 nb =NBMAXMOT;
    foreach (QString s, ml) {
        if(s.length() < TAILLEMINMOT) {
            continue;
        }
        sq.append("motscles LIKE '%" + s.toLower() + "%'");
        nb--;
        if(nb==0) {
            break;
        }
    }

    if(sq.length()==0) {
        return l;
    }


    QSqlQuery q = db.createQuery();
    q.prepare("SELECT nom, chemin,motscles, format FROM Item WHERE "
              + sq.join(" AND ") + " LIMIT ?" );
    q.bindValue(0, max);
    db.transaction();

    if(q.exec()) {
        while(q.next()) {
            RowIndexeur *r=new RowIndexeur();
            r->nom=q.value(0).toString();
            r->chemin=q.value(1).toString();
            r->motscles= q.value(2).toString();
            r->format=q.value(3).toInt();
            l.append(r);
            r->deleteLater();
        }
    }

    q.finish();
    db.commit();

    return l;
}

/**
 * @brief recherche les lignes dont le chemin commence par vurl
 * @param vurl
 * @param max
 * @return
 */
QList<RowIndexeur *>  DaoIndexeur::rechercherParVurl(const QString &vurl,quint64 max) {
    DB &db = DB::getInstance();

    QList<RowIndexeur *> l;

    if(db.isOpen()==false) {
        return l;
    }

    QSqlQuery q = db.createQuery();
    q.prepare("SELECT nom, chemin, motscles,format FROM Item WHERE chemin LIKE ? ESCAPE '\\' LIMIT ?" );
    QString tmp = QString(vurl);
    q.bindValue(0,
                QString("%1%").arg(
                    tmp.replace("%", "\\%").replace("_", "\\_").replace("'", "\\'")
                    ));
    q.bindValue(1, max);
    db.transaction();

    if(q.exec()) {
        while(q.next()) {
            RowIndexeur *r=new RowIndexeur();
            r->nom=q.value(0).toString();
            r->chemin=q.value(1).toString();
            r->motscles= q.value(2).toString();
            r->format=q.value(3).toInt();
            l.append(r);
            r->deleteLater();
        }
    }

    q.finish();
    db.commit();

    return l;
}


/**
  * @brief recherche un titre aleatoirement
  * @parm format, -1 pour tous,0 pour rep, 1 pour audio
  * @return info du titre
  */
RowIndexeur *DaoIndexeur::rechercheAleatoire(qint8 format) {
    DB &db = DB::getInstance();

    RowIndexeur *r=new RowIndexeur();

    QSqlQuery q=db.createQuery();

    QString where = "";

    switch (format) {
    case 0:
        where = "WHERE format = 0";
        break;
    case 1:
        where = "WHERE format = 1";
        break;
    }

    q.prepare("SELECT nom,chemin,format FROM Item " + where + " ORDER BY RANDOM() LIMIT 1");
    db.transaction();

    if(q.exec()) {
        if(q.next()) {
            r->nom=q.value(0).toString();
            r->chemin=q.value(1).toString();
            r->format=q.value(2).toInt();
        }
    }

    q.finish();
    db.commit();

    return r;
}

/**
 * @brief
 */
quint64 DaoIndexeur::taille() {
    DB &db = DB::getInstance();

    quint64 val=-1;

    if(db.isOpen()==false) {
        return val;
    }

    QSqlQuery q=db.createQuery();

    db.transaction();
    if(q.exec("SELECT Count(*) FROM Item") ==false) {
        qDebug()<< "pb size Item";
        return val;
    }

    if(q.next()) {
        val=q.value(0).toInt();
    }

    q.finish();
    db.commit();

    return val;
}

