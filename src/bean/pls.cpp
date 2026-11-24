#include "pls.h"
#include <QDebug>

Pls::Pls(QObject *parent) : UuidItem(parent), type(PlsType::PLS), duree(0) {

}

/**
 * @brief ajoute un nouveau media à la liste
 * @param url
 * @param duree
 */
void Pls::ajouterUrl(const QString &vurl, quint64 timestamp) {
    duree += timestamp;
    liste.append(vurl);
}

/**
 * @brief efface les info pistes
 */
void Pls::resetUrl() {
    liste.clear();
    duree=0;
}


/**
 * @brief copy
 * @param obj
 * @return
 */
Pls *Pls::copy(Pls *obj) {
    Pls *p = new Pls();

    p->nom = obj->nom;
    p->path = obj->path;
    p->type = obj->type;
    p->duree = obj->duree;

    p->liste.append( obj->liste );

    return p;
}
