#include "daofavoris.h"
#include <QSqlQuery>
#include <QDebug>
#include <QSqlError>
#include <QDateTime>
#include "db.h"

#include "../app.h"

DaoFavoris::DaoFavoris(QObject *parent) : QObject(parent), insertq(0), deleteq(0) {
    DB &db = DB::getInstance();
    if(db.isOpen()==false) {
        qDebug() << "Can't create statement";
        return;
    }

    insertq=db.createStatement("INSERT INTO Favoris (nom,chemin,creation,duree,pistes) values (?,?,?,?,?)");
    deleteq=db.createStatement("DELETE FROM Favoris WHERE chemin = ?");
}

DaoFavoris::~DaoFavoris() {
    delete insertq;
    delete deleteq;
    insertq = 0;
    deleteq = 0;
}

/**
 * @brief ajouter l'url à la liste des favoris
 * @param vurl
 */
void DaoFavoris::ajouter(const QString &nom, const QString &vurl, quint64 duree, quint64 pistes) {

    DB &db = DB::getInstance();

    if(db.isOpen()==false) {
        return;
    }

    db.transaction();

    QSqlQuery &q = *insertq;
    q.bindValue(0,nom);
    q.bindValue(1,vurl);
    q.bindValue(2,QDateTime::currentMSecsSinceEpoch() / 1000);
    q.bindValue(3,duree);
    q.bindValue(4,pistes);

    q.exec();
    q.finish();

    db.commit();
}

/**
 * @brief supprime vurl de la liste des favoris
 * @param vurl
 */
void DaoFavoris::supprimer(const QString &vurl) {
    DB &db = DB::getInstance();

    if(db.isOpen()==false) {
        return;
    }

    db.transaction();

    QVariant da(vurl);
    QSqlQuery &qd = *deleteq;
    qd.bindValue(0,da);
    qd.exec();
    qd.finish();

    QSqlQuery q = db.createQuery();
    q.exec("VACUUM");
    q.finish();

    db.commit();
}

/**
 * @brief liste
 * @param ordre
 * @return
 */
QList<RowFavoris *> DaoFavoris::liste(quint16 ordre) {
    DB &db = DB::getInstance();

    QList<RowFavoris *> l;

    if(db.isOpen()==false) {
        return l;
    }

    QString str_o("nom ASC");
    switch (ordre) {
    
    case DAO_FAVORIS_DATE:
        str_o = "creation DESC";
        break;
    }

    QSqlQuery q = db.createQuery();
    db.transaction();

    if(q.exec("SELECT nom,chemin,creation,duree,pistes FROM Favoris ORDER BY " + str_o)) {
        while(q.next()) {
            RowFavoris *r = new RowFavoris();
            r->nom=q.value(0).toString();
            r->chemin=q.value(1).toString();
            r->creation=q.value(2).toInt();
            r->duree= q.value(3).toInt();
            r->pistes= q.value(4).toInt();
            l.append(r);
        }
    }
    q.finish();

    db.commit();

    return l;
}

/**
 * @brief
 */
quint64 DaoFavoris::taille() {
    DB &db = DB::getInstance();

    quint64 val=-1;

    if(db.isOpen()==false) {
        return val;
    }

    QSqlQuery q=db.createQuery();

    db.transaction();
    if(q.exec("SELECT Count(*) FROM Favoris") ==false) {
        return val;
    }

    if(q.next()) {
        val=q.value(0).toInt();
    }

    q.finish();
    db.commit();

    return val;
}
