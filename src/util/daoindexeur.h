#ifndef DAORECHERCHE_H
#define DAORECHERCHE_H

#include <QObject>
#include <QString>
#include <QSqlQuery>
#include <QList>

#include "rowindexeur.h"

using namespace std;

class DaoIndexeur  : public QObject{
    Q_OBJECT

public:
    explicit DaoIndexeur(QObject *parent = 0);
    virtual ~DaoIndexeur();

    /**
     * @brief ajoute une ligne
     * @param l
      */
    void ajouter(const QList<RowIndexeur *> &l);
    void ajouter(RowIndexeur *r);

    /**
     * @brief efface le contenu de la db
     */
    void effacer();

    /**
     * @brief efface les items qui ont leurs chemin qui COMMENCE par vurl
     * @param vurl
     */
    void effacerParVUrl(const QString &str);

    /**
     * @brief rechercher les lignes par mots clés
     * @param str
     * @return
     */
    QList<RowIndexeur *> rechercher(const QString &str,quint64 max);

    /**
     * @brief recherche les lignes dont le chemin commence par vurl
     * @param vurl
     * @param max
     * @return
     */
    QList<RowIndexeur *> rechercherParVurl(const QString &vurl,quint64 max);

    /**
      * @brief recherche un titre aleatoirement
      * @parm support, -1 pour tous,0 pour rep, 1 pour audio
      * @return info du titre
      */
    RowIndexeur *rechercheAleatoire(qint8 support=-1);

    /**
     * @brief
     */
    quint64 taille();


private :

    QSqlQuery *insertq;
};

#endif // DAORECHERCHE_H
