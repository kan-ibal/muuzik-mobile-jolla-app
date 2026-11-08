#ifndef DAOFAVORIS_H
#define DAOFAVORIS_H

#include <QObject>
#include <QString>
#include <QSqlQuery>
#include "rowfavoris.h"

#define DAO_FAVORIS_DATE 0
#define DAO_FAVORIS_ALPHA 1

class DaoFavoris : public QObject {
    Q_OBJECT

public:
    explicit DaoFavoris(QObject *parent = 0);
    virtual ~DaoFavoris();

    /**
     * @brief ajouter l'url à la liste des favoris
     * @param vurl
     */
    void ajouter(const QString &nom, const QString &vurl, quint64 duree, quint64 piste);

    /**
     * @brief supprime vurl de la liste des favoris
     * @param vurl
     */
    void supprimer(const QString &vurl);

    /**
     * @brief liste
     * @param ordre
     * @return
     */
    QList<RowFavoris *> liste(quint16 ordre=DAO_FAVORIS_ALPHA);

    /**
     * @brief
     */
    quint64 taille();

private:
    QSqlQuery *insertq;
    QSqlQuery *deleteq;

};

#endif // DAOFAVORIS_H
