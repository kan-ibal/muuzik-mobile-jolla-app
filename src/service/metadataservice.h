#ifndef METADATASERVICE_H
#define METADATASERVICE_H

#include <QObject>
#include <QQmlContext>
#include <QMutex>

#include "../bean/MetaData.h"
#include "../bean/albumstat.h"

#include "../virtualfiles/VFInfo.h"

#define METADATASERVICE_QML_NAME "MetaDataService"

/**
 * @brief fournis un service de recherche de metadata (tag mp3) depuis
 * des url virtuelle (vfinfo)
 * permet d'être utilisé depuis qml
 */
class MetaDataService : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief instance (singleton) for the service
     * @return
     */
    static MetaDataService& getInstance();

    /**
     * @brief register the service into the context so it can be used in qml
     * @param context
     * @param name : register service on this name, default to METADATASERVICE_QML_NAME
     */
    static void init(QQmlContext *context, QString name=METADATASERVICE_QML_NAME);

    /**
     * @brief get metadata from url.
     * If url doesn't define a file with metadata, return an empty structure
     * NOTA : il ne faut pas supprimer la MetaData retourné cas en cache
     * @param url virtual url to the media
     * @param soft : si true et que vurl est vers un repertoire / album, retourne
     * directement les info du premier media
     * @return meta data or null if not found (or can't read)
     */
    MetaData *getMetaData(const QString & vurl, bool soft = false) ;
    Q_INVOKABLE QVariant getMetaDataAsQVariant(const QString & vurl, bool soft = false);

    /**
     * @brief recup les meta données mais par le signal MetaDataAvailable
     * @param vurl
     */
    Q_INVOKABLE void getMetaDataASynch(const QString & vurl,bool soft = false);

    /**
      * @brief calcule le temps total d'un "album"
      * @param url virtuelle de l'album/repertoire
      * @return info sur l'album
      */
    AlbumStat *getStatFromAlbum(const QString &vurl);
    Q_INVOKABLE QVariant getStatFromAlbumAsQVariant(const QString &vurl);

    /**
      * @brief idem mais sur le signal statAvailable (toujours envoyé)
      * @param url virtuelle de l'album/repertoire
      */
    Q_INVOKABLE void getStatFromAlbumASynch(const QString &vurl);

private:
    explicit MetaDataService(QObject *parent = 0);
    virtual ~MetaDataService();

    mutable QMutex mutex;
    QHash<QString, MetaData *> cache;

    QHash<QString, AlbumStat *> statCache;

    /**
     * @brief doGetMetaData
     * @param vurl
     * @return
     */
    static MetaData *doGetMetaData(const QString & vurl) ;


    /**
     * @brief qtconcurrent worker
     * @param vurl
     */
    static void doGet(const QString &vurl, bool soft);

    /**
     * @brief qtconcurrent worker
     * @param vurl
     */
    static void doGetStat(const QString &vurl);

signals :

    /**
     * @brief MetaData disponible
     * @param vurl
     * @param meta
     */
    void metaDataAvailable(QString vurl, QVariant meta);

    /**
     * @brief timeAvailable
     * @param vurl
     * @param time
     * @since 2.4 passé sur un qVariant(Album)
     */
    void statAvailable(QString vurl, QVariant stat);
};

#endif // METADATASERVICE_H
