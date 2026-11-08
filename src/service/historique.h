#ifndef HISTORIQUE_H
#define HISTORIQUE_H
#include <QObject>
#include <QQmlContext>
#include <qqml.h>
#include "../util/rowhistorique.h"
#include "../util/daohistorique.h"
#include "../bean/resultitem.h"

#define HISTORIQUESERVICE_QML_NAME "historiqueService"

class HistoriqueService : public QObject {
    Q_OBJECT

    Q_PROPERTY(QQmlListProperty<RowHistorique> liste READ getListe NOTIFY listeChanged )

public:
    static HistoriqueService& getInstance();
    static void init(QQmlContext *context, QString name = HISTORIQUESERVICE_QML_NAME);
    virtual ~HistoriqueService();

    /**
     * @brief getListe
     * @return
     */
    QQmlListProperty<RowHistorique> getListe() {
        return QQmlListProperty<RowHistorique>(this, liste);
    }

    /**
      * @brief ajoute (ou incremente) une nouvelle entrée
      * @param vurl : repetoire virtuel
      */
    Q_INVOKABLE void ajouter(const QString &vurl);

    /**
      * @brief supprime l'entrée de la liste d'historique
      * @param uuid : uuid dans la liste
      */
    Q_INVOKABLE void supprimer(quint64 uuid);

    /**
      * @brief efface l'historique
      */
    Q_INVOKABLE void vider();

    /**
      * @brief formate une date
      * @param t le temps en secondes
      * @return chaîne formaté
      */
    Q_INVOKABLE QString formatTime(quint64 t);

private:
    explicit HistoriqueService(QObject *parent = 0);


    QList<RowHistorique *> liste;

    DaoHistorique dao;

    /**
     * @brief recharge la liste depuis la db
     */
    void rafraichir();

    /**
     * @brief ordreDate
     * @param a
     * @param b
     * @return
     */
    static bool ordreDate( RowHistorique *a, RowHistorique  *b );


signals:
    void listeChanged();
    void ordreChanged();

public slots:

    // suppression de l'url ( et de tout les childs)
    void onRemoved(QString vurl);
    void onMoved(QString vurl, QString as);
    void onRenamed(QString vurl, QString name);

};

#endif // HISTORIQUE_H
