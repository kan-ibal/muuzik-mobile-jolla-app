#ifndef FAVORIS_H
#define FAVORIS_H

#include <QObject>
#include <QQmlContext>
#include <QList>
#include <QHash>
#include <qqml.h>
#include <QSettings>
#include "../util/daofavoris.h"

#include "config.h"

using namespace std;

#define FAVORISLISTE "favoris/liste"

#define FAVORISSERVICE_QML_NAME "FavorisService"

class favorisService : public QObject{

    Q_OBJECT
    Q_PROPERTY(QQmlListProperty<RowFavoris> liste READ getListe NOTIFY listeChanged )

public:
    static favorisService& getInstance();
    virtual ~favorisService();

    /**
     * @brief register the service into the context so it can be used in qml
     * @param context
     * @param name : register service on this name, default to FAVORISSERVICE_QML_NAME
     */
    static void init(QQmlContext *context, QString name=FAVORISSERVICE_QML_NAME);

    /**
     * @brief getListe
     * @return
     */
    QQmlListProperty<RowFavoris> getListe(){ return QQmlListProperty<RowFavoris>(this, liste);}
    QList<RowFavoris *> getRawListe() const { return liste;}

    /**
     * @brief suprimme le favoris avec l'uuid uuid
     * @param uuid
     */
    Q_INVOKABLE void supprimer(const qint64 uuid);

    /**
     * @brief ajoute le repertoire sous "path" à la liste des favoris
     * @param vurl url (virtuelle) vers le repertoire
     * @return true si ajouté (+ signal)
     */
    Q_INVOKABLE qint64 ajouter(const QString vurl);

    /**
     * @brief teste si un chemin virtuel est déjà un favoris
     * @return uuid de l'item ou -1 si pas dans la liste
     */
    Q_INVOKABLE qint64 getFavoris(const QString vurl);

  private:
    explicit favorisService(QObject *parent = 0);

    configService *config;

    QSettings settings;

    QList<RowFavoris *> liste;

    DaoFavoris dao;

    /**
     * @brief charge la liste des favoris
     */
    void recharger();

    /**
     * @brief sauve la liste des favoris
     */
    void sauver();

    /**
     * @brief importer l'ancienne liste de favoris dans la nouvelle base
     */
    void importer();

    /**
     * @brief classe la liste en fonction de config, cree liste_classe
     */
    void classer(const quint16 ordre);

    static bool ordreAlpha(  RowFavoris * a, RowFavoris  * b );
    static bool ordreDate(  RowFavoris * a, RowFavoris  * b );
    static bool ordreInv(  RowFavoris * a, RowFavoris  * b );

signals:
    void listeChanged(); // ajout / suppression dans la liste

public slots:
    void ordreFavorisImpl();

    // suppression de l'url ( et de tout les childs)
    void onRemoved(QString vurl);
    void onMoved(QString vurl, QString as);
    void onRenamed(QString vurl, QString name);
};

Q_DECLARE_METATYPE(QList<QString>)

#endif // FAVORIS_H
