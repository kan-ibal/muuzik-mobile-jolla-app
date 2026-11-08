#ifndef RECHERCHESERVICE_H
#define RECHERCHESERVICE_H

#include <QObject>
#include <qqml.h>
#include "../util/daoindexeur.h"
#include "../bean/resultitem.h"
#include <QQmlContext>

#define DEF_RECH_TAILLE 20

#define RECHERCHESERVICE_QML_NAME "RechercherService"

using namespace std;

class rechercheService : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QQmlListProperty<ResultItem> liste READ getListe NOTIFY listeChanged )
    Q_PROPERTY(bool taille READ getTaille WRITE setTaille NOTIFY tailleChanged )

public:
    static rechercheService& getInstance();
    static void init(QQmlContext *context, QString name = RECHERCHESERVICE_QML_NAME);
    virtual ~rechercheService();

    /**
      * @brief recherche des enregistrement
      * @param str chaine de recherche
      */
    Q_INVOKABLE QList<ResultItem *> rechercher(QString const &str);

    /**
     * @brief getListe
     * @return
     */
    QQmlListProperty<ResultItem> getListe() {
        return QQmlListProperty<ResultItem>(this, liste);
    }

    /**
      *@brief efface la rechercher
      */
    Q_INVOKABLE void vider();

    /**
     * @brief max nb de resultat de recherche
     * @return
     */
    quint64 getTaille() const {return taille;}
    void setTaille(quint64 v);

    /**
      * @brief recherche un titre aleatoirement
      * @parm type, -1 pour tous,0 pour rep, 1 pour audio
      * @return info du titre
      */
    Q_INVOKABLE ResultItem *rechercheAleatoire(qint8 support=-1);

private:
    explicit rechercheService(QObject *parent = 0);

    QList<ResultItem *> liste;

    quint64 taille;

    DaoIndexeur dao;

signals:
    void listeChanged();
    void tailleChanged();

};

#endif // RECHERCHESERVICE_H
