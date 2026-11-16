#ifndef POCHETTESERVICE_H
#define POCHETTESERVICE_H

#include <QObject>
#include <QQmlContext>
#include <QList>
#include <QString>
#include <QHash>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QHash>
#include <QFileInfo>
#include <QMutex>
#include <qqml.h>

#define ARTSERVICE_QML_NAME "ARTService"

#define ARTS ArtWorkService::getInstance()

/**
 * @brief ce service est responsable de la gestion des pochettes / art
 */
class ArtWorkService : public QObject {
    friend class QNetworkAccessManager;

    Q_OBJECT

public:
    static ArtWorkService& getInstance();

    /**
     * @brief register the service into the context so it can be used in qml
     * @param context
     * @param name : register service on this name, default to ARTSERVICE_QML_NAME
     */
    static void init(QQmlContext *context, QString name=ARTSERVICE_QML_NAME);

    static const QString IMG_REPERTOIRE;
    static const QString IMG_AUDIO;
    static const QString IMG_ALBUM;
    static const QString IMG_TITRE;

    /**
      * @brief recherche une pochette pour uri, dir ou fichier
      * non blocanquante. si une pochette existe, res sur artFound
      * cette fonction cherche si un resulat existe en local, sinon
      * recupère une pochette sur le reseau (si possible/autorisé)
      * @param vurl
      */
    Q_INVOKABLE void getArtAsync(const QString &vurl);

    /**
      *@brief : retourne la miniature pour l'uri
      * fonction synchrone / blocante.
      * cette fonction cherche si un resulat existe en local, sinon
      * recupère une pochette sur le reseau (si possible/autorisé)
      * @param vurl
      * @return path vers la miniature (et pas vurl) ou QString::null
      **/
   Q_INVOKABLE QString getArt(const QString &vurl);

protected :
    QNetworkAccessManager *manager;

private :
    explicit ArtWorkService(QObject *parent = 0);

    // on cache les resultats
    mutable QMutex cacheMutex;
    QHash<QString,QString> cache;

    bool lastfmwarning;

    // la liste des url pour les cover qui on déjà été recherché : evite les recherches multiples et paralléles
    QList<QString> history;

    // sauve les date originale d'un rep avant de telechager une capture
    QHash<QString, QFileInfo> infoRep;

    /**
     * @brief fait la recherche effective asyc
     * @param vurl
     */
    static void doGetArt(const QString &vurl);

    /**
     * @brief getFromDirectory
     * @param vurl
     * @return path ou QString:null
     */
    static QString fromDirectory(const QString &vurl);

    /**
     * @brief getFromTag
     * @param vurl
     * @return path ou QString:null
     */
    static QString fromTag(const QString &vurl);

    /**
     * @brief recherche la meilleure miniature depuis les donnée json
     * @param data
     * @return url de la miniature ou QString:null si aucune
     */
    static QString getImageFromJson(const QByteArray &data);

private slots:

    /**
     * @brief lance la procedure de recherche de artwork en ligne pour le titre vurl
     * @param vurl
     */
    void launchNetImpl(const QString &vurl);

    /**
     * @brief reponse reseau
     * @param reply
     */
    void reponseNet(QNetworkReply *reply);

    /**
     * @brief downloadSuccess
     * @param url
     * @param target
     */
    void downloadSuccess(QUrl const & url,QString const &target );

    /**
     * @brief erreur de telechargement -> envoie le signal pochette pas trouvé
     * @param url
     * @param status
     */
    void downloadError(QUrl const & url, QString const &target, QNetworkReply::NetworkError status);

signals :
    /**
     * @brief PRIVÉ : emis depuis les qfutures
     * @param vurl
     */
    void doNetSearch(const QString &vurl);

    /**
     * @brief trouve une pochette pour l'url virtuelle donnée
     * @param vurl : url virtuelle du media ou repertoire contenant le titre
     * @param artPath : path (absolute) vers la miniature (jpg/png)
     */
    void artFound(const QString vurl, const QString artPath);

    /**
     * @brief si ne trouve pas de pochette pour l'url virtuelle donnée
     * @param vurl
     */
    void artNotFound(const QString vurl, const QString artPath);

};

#endif // POCHETTESERVICE_H
