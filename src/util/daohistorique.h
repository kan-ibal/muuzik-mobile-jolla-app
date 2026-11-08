#ifndef DAOHISTORIQUE_H
#define DAOHISTORIQUE_H

#include <QObject>
#include <QString>
#include <QSqlQuery>
#include "rowhistorique.h"

#define DAO_HISTORIQUE_DATE 0
#define DAO_HISTORIQUE_TAILLE 1
#define DAO_HISTORIQUE_ALPHA 2

class DaoHistorique  : public QObject{
    Q_OBJECT

public:
    explicit DaoHistorique(QObject *parent=0);
    virtual ~DaoHistorique();

    /**
     * @brief ajouter une nouvelle entrée
     * @param v
     * @param ouverture : 0 pour la date actuelle, en sec depuis epoch (pour op edit sur moved, renamed)
     * @param nombre : # déjà ouvert (pour op edit sur moved, renamed)
     */
    void ajouter(const QString &nom, const QString &vurl, quint64 duree, quint64 piste, quint64 ouverture = 0,quint64 nombre = 0);

    /**
     * @brief supprime vurl de la liste d'hystorique
     * @param vurl
     * @since 2.1
     */
    void supprimer(const QString &vurl);

    /**
     * @brief efface le contenu de la db
     * @param v en minute depuis epoch
     */
    void effacer(quint64 v=0);

    /**
     * @brief liste l'historique
     * @param odre : comment trier les lignes
     * @return
     */
    QList<RowHistorique *> liste(quint16 ordre=DAO_HISTORIQUE_DATE);


    /**
     * @brief
     */
    quint64 taille();

private:
    QSqlQuery *insertq;
    QSqlQuery *insertcountq;
    QSqlQuery *deletecheminq;
    QSqlQuery *deleteq;


};

#endif // DAOHISTORIQUE_H
