#include "MetaData.h"
#include <QDebug>

#include "../virtualfiles/VFservice.h"

MetaData::MetaData(QObject *parent) : UuidItem(parent),annee(0),duree(0){
}

/**
 * @brief MetaData::~MetaData
 */
MetaData::~MetaData() {
}

/**
 * @brief calcule la duree de la liste en secondes
 * @param l
 * @return
 */
qint64 MetaData::dureeListe(const QList<MetaData *> *l) {
    qint64 d=0;
    foreach (MetaData *t,*l) {
        d+=t->getDuree();
    }

    return d;
}
/**
  * @brief chaine representant cette duree
  * @return
  */
QString MetaData::dureeString(const qint64 d) {
    qint64 h = d / 3600;
    qint64 dd = d % 3600;
    qint64 m = dd/60;
    qint64 s = dd%60;

    return (h>0 ? QString::number(h) + ":":"") +
            (m <10? "0" : "")+ QString::number(m) + ":"+
            (s<10? "0" : "")+ QString::number(s) ;
}

/**
  * @brief versMediaContent
  * @return
  */
QList<QMediaContent> MetaData::versMediaContent(QList<MetaData *> *s) {
    QList<QMediaContent> l;

    foreach (MetaData *t, *s) {
        QString url = VFI.getPathFromVUrl(t->getChemin());
        l.append(QMediaContent(QUrl::fromLocalFile(url)));
    }

    return l;
}

/**
  * @brief Titre::versMediaContent
  * @param t
  * @return
  */
QMediaContent MetaData::versMediaContent(MetaData *t) {
    QString url = VFI.getPathFromVUrl(t->getChemin());
    return QMediaContent(QUrl::fromLocalFile(url));
}

/**
 * @brief fait une copy de l'objet
 * @param t
 * @return
 */
MetaData *MetaData::copy(MetaData *obj) {
    if (!obj) {
        return NULL;
    }
    MetaData *t = new MetaData();

    t->uuid=obj->uuid;
    t->titre=obj->titre;
    t->album =obj->album;
    t->artiste =obj->artiste;
    t->chemin =obj->chemin;
    t->annee =obj->annee;
    t->duree =obj->duree;
    return t;
}
