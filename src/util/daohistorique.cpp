#include "daohistorique.h"
#include <QSqlQuery>
#include <QDebug>
#include <QSqlError>
#include <QDateTime>
#include "db.h"

#include "../app.h"
#include "../virtualfiles/VFservice.h"

DaoHistorique::DaoHistorique(QObject *parent) : QObject(parent),
    insertq(0), insertcountq(0), deletecheminq(0), deleteq(0) {

    DB &db = DB::getInstance();
    if(db.isOpen()==false) {
        qDebug() << "Can't create statement";
        return;
    }

    insertq=db.createStatement("INSERT INTO Historique (nom,chemin,ouverture,nombre,duree,pistes) values (?,?,?,?,?,?)");
    insertcountq=db.createStatement("SELECT Count(*) FROM Historique WHERE chemin = ?");
    deletecheminq=db.createStatement("DELETE FROM Historique WHERE chemin = ?");
    deleteq=db.createStatement("DELETE FROM Historique WHERE ouverture <= ?");
}

DaoHistorique::~DaoHistorique() {
    delete insertq;
    delete insertcountq;
    delete deletecheminq;
    delete deleteq;
    insertq = 0;
    insertcountq = 0;
    deletecheminq = 0;
    deleteq = 0;
}

/**
 * @brief ajouter une nouvelle entrée
 * @param v
 */
void DaoHistorique::ajouter(const QString &nom, const QString &vurl, quint64 duree, quint64 piste, quint64 ouverture,quint64 nombre) {
    DB &db = DB::getInstance();

    if(db.isOpen()==false) {
        return;
    }

    if( ouverture == 0) {
        ouverture = QDateTime::currentMSecsSinceEpoch() / 1000;
    }

    db.transaction();

    QSqlQuery &qc = *insertcountq;
    qc.bindValue(0,vurl);
    if(qc.exec() ==false) {
        qc.finish();
        db.commit();
        return ;
    }

    if(qc.next()) {
        nombre = qc.value(0).toInt();
    }
    qc.finish();

    // supprime l'ancienne ligne
    if( nombre > 0 ) {
        QSqlQuery &qd = *deletecheminq;
        qd.bindValue(0,vurl);
        qd.exec();
        qd.finish();
        db.commit();
    }

    QSqlQuery &q = *insertq;
    q.bindValue(0,nom);
    q.bindValue(1,vurl);
    q.bindValue(2,ouverture);
    q.bindValue(3,nombre+1); // ancien + 1
    q.bindValue(4,duree);
    q.bindValue(5,piste);

    q.exec();
    q.finish();

    db.commit();
}

/**
 * @brief supprime vurl de la liste d'hystorique
 * @param vurl
 * @since 2.1
 */
void DaoHistorique::supprimer(const QString &vurl) {

    DB &db = DB::getInstance();

    if(db.isOpen()==false) {
        return;
    }

    db.transaction();

    QVariant da(vurl);
    QSqlQuery &qd = *deletecheminq;
    qd.bindValue(0,da);
    qd.exec();
    qd.finish();

    QSqlQuery q = db.createQuery();
    q.exec("VACUUM");
    q.finish();

    db.commit();
}

/**
 * @brief efface le contenu de la db
 */
void DaoHistorique::effacer(quint64 v) {
    DB &db = DB::getInstance();

    if(db.isOpen()==false) {
        return;
    }

    db.transaction();

    if(v==0) {
        QSqlQuery qd = db.createQuery();
        qd.exec("DELETE FROM Historique");
        qd.finish();
    } else {
        QVariant da(v);
        QSqlQuery &qd = *deleteq;
        qd.bindValue(0,da);
        qd.exec();
        qd.finish();
    }
    QSqlQuery q = db.createQuery();
    q.exec("VACUUM");
    q.finish();

    db.commit();
}

/**
 * @brief liste l'historique
 * @param odre : comment trier les lignes
 * @return
 */
QList<RowHistorique *> DaoHistorique::liste(quint16 ordre) {
    DB &db = DB::getInstance();

    QList<RowHistorique *> l;

    if(db.isOpen()==false) {
        return l;
    }

    QString str_o("ouverture DESC");
//    switch (ordre) {
//    case DAO_HISTORIQUE_ALPHA:
//        str_o = "nom ASC";
//        break;
//    case DAO_HISTORIQUE_TAILLE:
//        str_o = "nombre DESC";
//        break;
//    }

    QSqlQuery q = db.createQuery();
    db.transaction();

    if(q.exec("SELECT nom,chemin,ouverture,nombre,duree,pistes FROM Historique ORDER BY " + str_o)) {
        while(q.next()) {
            RowHistorique *r = new RowHistorique();
            r->nom=q.value(0).toString();
            r->chemin=q.value(1).toString();
            r->ouverture=q.value(2).toInt();
            r->nombre=q.value(3).toInt();
            r->duree=q.value(4).toInt();
            r->pistes=q.value(5).toInt();
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
quint64 DaoHistorique::taille() {
    DB &db = DB::getInstance();

    quint64 val=-1;

    if(db.isOpen()==false) {
        return val;
    }

    QSqlQuery q=db.createQuery();

    db.transaction();
    if(q.exec("SELECT Count(*) FROM Historique") ==false) {
        return val;
    }

    if(q.next()) {
        val=q.value(0).toInt();
    }

    q.finish();
    db.commit();

    return val;
}
