#include "artwork.h"
#include <QUrl>
#include <QRegularExpression>
#include <QRegularExpressionMatchIterator>
#include <QRegExp>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QDebug>
#include <QtAlgorithms>
#include <QFuture>
#include <QtConcurrent/QtConcurrentRun>
#include <QJsonDocument>
#include <QJsonObject>
#include <utime.h>

#include "config.h"
#include "../app.h"
#include  <taglib/fileref.h>
#include  <taglib/tag.h>

#include "../vi18n/vi18n.h"
#include "../virtualfiles/VFservice.h"
#include "../qnetworkwatcher/qnetworkwatcher.h"
#include "../qdownloader/qdownloaderservice.h"

#include <nemonotifications-qt5/notification.h>

#ifndef ART_VURL
#define ART_VURL "vurl"
#endif

#ifndef ART_SOURCE
#define ART_SOURCE "sourceurl"
#endif

#ifndef ART_DEF_COVER
#define ART_DEF_COVER "cover"
#endif

#ifndef ART_MESSAGECOVER
#define ART_MESSAGECOVER "messagecover"
#endif

#ifndef ART_PASDEPOCHETTE
#define ART_PASDEPOCHETTE  ".nocover"
#endif

#define ART_ALBUMCOVERART_PATTERN "http://ws.audioscrobbler.com/2.0/?method=album.getinfo&artist=%1&album=%2&api_key=edb5727ffe1a4ac4c0aca7c4d068aa16&autocorrect=1&format=json"
#define ART_ALBUMCOVERART_DOMAIN "akamaized.net"

static QRegExp BLANK("[ -:#';,?&$%()=+\\[\\]!*]");

/**
 * @brief pochetteService::pochetteService
 * @param parent
 */
ArtWorkService::ArtWorkService(QObject *parent) :
    QObject(parent), manager(new QNetworkAccessManager(this)), lastfmwarning(false) {
    manager->setNetworkAccessible(QNetworkAccessManager::Accessible);
    connect(manager, SIGNAL(finished(QNetworkReply *)), this, SLOT(reponseNet(QNetworkReply*)));

    connect(this, SIGNAL(doNetSearch(QString)), this, SLOT(launchNetImpl(QString)));

    connect(&QDownloaderService::getInstance(), SIGNAL(success(QUrl,QString)),
            this, SLOT(downloadSuccess(QUrl,QString)));
    connect(&QDownloaderService::getInstance(), SIGNAL(error(QUrl,QString,QNetworkReply::NetworkError)),
            this, SLOT(downloadError(QUrl,QString,QNetworkReply::NetworkError)));
}

/**
 * @brief pochetteService::getInstance
 * @return
 */
ArtWorkService& ArtWorkService::getInstance() {
    static ArtWorkService instance;
    return instance;
}

/**
 * @brief register the service into the context so it can be used in qml
 * @param context
 * @param name : register service on this name, default to ARTSERVICE_QML_NAME
 */
void ArtWorkService::init(QQmlContext *context, QString name) {
    context->setContextProperty(name,&ArtWorkService::getInstance());
}

/**
  * @brief recherche une pochette pour uri, dir ou titre
  * @param uri
  */
void ArtWorkService::getArtAsync(const QString &vurl) {
    //    qDebug() << "sur ArtWorkService::getArtAsync";
    QFuture<void> f = QtConcurrent::run(doGetArt,vurl);
}

/**
  *@brief : retourne la miniature pour l'uri
  * @param uri
  * @return url vers la miniature
  **/
QString ArtWorkService::getArt(const QString &vurl) {
    //    qDebug() << "sur ArtWorkService::getArt " << vurl;

    // cherche dans le cache
    if(cache.contains(vurl)) {
        return cache.value(vurl);
    }

    if(VFI.exist(vurl) == false) {
        return QString::null;
    }

    //-------------------------------------------
    // c'est un répertoire / album
    if(VFI.isDirectory(vurl)) {
        // on recherche une miniature dans le repertoire
        QString path = fromDirectory(vurl);
        if(path.isNull()==false) {
            cache.insert(vurl,path);
            return path;
        }

        // on cherche recup le premier fichier audio qui a put être indexe
        QList<VFInfo *> l = VFI.getSynch(vurl, "\\.(mp3|flac)$", VFService::VFType::FILE);
        if(l.length()==0) {
            //            delete(l);

            // pas d'audio
            cache.insert(vurl,"");
            return QString::null;
        }

        QString audio_path = l.at(0)->getVUrl();
        path = fromTag(audio_path);
        qDeleteAll(l);
        //        delete(l);

        // trouve par fichier audio ???
        if(path.isNull()==false) {
            cache.insert(vurl,path);
            return path;
        }

        // on a rien on lance une recherche net
        emit ArtWorkService::getInstance().doNetSearch(audio_path);

        return QString::null;
    }

    //-------------------------------------------
    // c'est un fichier audio direct
    QString path = fromTag(vurl);
    if(path.isNull()==false) {
        cache.insert(vurl,path);
        return path;
    }

    path = fromDirectory(VFI.getParentVUrl(vurl));
    if(path.isNull()==false) {
        cache.insert(vurl,path);
        return path;
    }

    // on a rien on lance une recherche net
    emit ArtWorkService::getInstance().doNetSearch(vurl);

    return QString::null;
}

/**
 * @brief fait la recherche effective async
 * @param vurl
 * @return
 */
void ArtWorkService::doGetArt(const QString &vurl) {

    QString path = ArtWorkService::getInstance().getArt(vurl);
    if( path.isNull() || path.isEmpty()) {
//        if( VFI.isDirectory(vurl) == false) {
//            emit ArtWorkService::getInstance().artNotFound(
//                        VFI.getParentVUrl(vurl)
//                        ,"");
//            return;
//        }

        emit ArtWorkService::getInstance().artNotFound(vurl,"");
        return;
    }

    emit  ArtWorkService::getInstance().artFound(vurl, path);
}

/**
 * @brief getFromDirectory
 * @param vurl
 * @return path ou QString:null
 */
QString ArtWorkService::fromDirectory(const QString &vurl) {

    QList<VFInfo *> l = VFI.getSynch(vurl, VFService::FILTERIMAGE, VFService::VFType::FILE);
    if(l.length()>0) {
        // on regarde si on a un fichier cover.jpg
        foreach (VFInfo * info, l) {
            if(info->getName().compare(ART_DEF_COVER,Qt::CaseInsensitive)==0) {
                QString cover = VFI.getPathFromVUrl(info->getVUrl());
                qDeleteAll(l);
                return cover;
            }
        }

        // on va prendre la première image
        QString path = VFI.getPathFromVUrl( l.at(0)->getVUrl() );
        qDeleteAll(l);
        return path;
    }

    return QString::null;
}

/**
 * @brief getFromTag
 * @param vurl
 * @return path ou QString:null
 */
QString ArtWorkService::fromTag(const QString &vurl) {

    TagLib::FileRef fr(VFI.getPathFromVUrl(vurl).toUtf8().constData() );
    if( fr.isNull() == false) {

        QString album(fr.tag()->album().toCString());
        QString artiste(fr.tag()->artist().toCString());

        if(album.length()>0 && artiste.length()>0) {
            QString url(
                        QDir::homePath()+"/.cache/media-art/album-" +
                        QCryptographicHash::hash(
                            artiste.toLower().normalized(QString::NormalizationForm_KD).toUtf8(),
                            QCryptographicHash::Md5).toHex()+
                        "-" +
                        QCryptographicHash::hash(
                            album.toLower().normalized(QString::NormalizationForm_KD).toUtf8(),
                            QCryptographicHash::Md5).toHex()+
                        ".jpeg"
                        );

            QFileInfo info(url);
            if(info.exists() && info.isReadable()) {
                return url;
            }
        }
    }

    return QString::null;
}

/**
 * @brief lance la procedure de recherche de artwork en ligne pour le titre vurl
 * @param vurl
 */
void ArtWorkService::launchNetImpl(const QString &vurl) {
    qDebug() << "ArtWorkService::launchNet : " << vurl;

    // verif si on a déjà fait cette recherche
    if(history.contains(vurl)) {
        return;
    }
    history.append(vurl);

    // verif le reseau
    if( QNWI.getConnected() == NetworkType::NONE ){
        return;
    }

    if(configService::getInstance().getPochetteWifi() && (QNWI.getWifi()==false)) {
        return;
    }

    if(configService::getInstance().getLastfm() == false) {
        if(lastfmwarning==false) {
            lastfmwarning=true;

            Notification n;
            n.setPreviewBody(vI18n::getInstance().get( "pochette.last.warning" ));
            n.setHintValue("x-nemo-icon", "image://theme/icon-s-certificates");
            n.publish();
        }

        return;
    }

    QString path = VFI.getPathFromVUrl(vurl);

    // verif si fichier nocover existe dejà
    QFileInfo fi(VFI.getParentVUrl(path)+"/"+ART_PASDEPOCHETTE);
    if(fi.exists()) {
        return;
    }

    TagLib::FileRef fr(path.toUtf8().constData() );
    if( fr.isNull() ) {
        return;
    }

    QString album = QString(fr.tag()->album().toCString());
    QString artiste =QString(fr.tag()->artist().toCString());
    if(artiste.length()==0 || album.length()==0 ) {
        return;
    }

    QUrl url(QStringLiteral(ART_ALBUMCOVERART_PATTERN).
             arg(artiste.replace(BLANK, "%20")).
             arg(album.replace(BLANK, "%20"))
             );

    QNetworkRequest request(url);
    request.setRawHeader("User-Agent", "User Agent");
    qDebug() << "Lance recherche : "  << url;

    QNetworkReply *reply = manager->get(request);
    reply->setProperty(ART_VURL,vurl);

    QFileInfo info(VFI.getPathFromVUrl(vurl));
    infoRep.insert(vurl,info);

    if(configService::getInstance().getPochetteFeedback()) {
        reply->setProperty(ART_MESSAGECOVER, QString( vI18n::getInstance().get("pochette.message").arg(album)));
    }
}

/**
  * @brief pochetteService::reponseBio
  * @param reply
  */
void ArtWorkService::reponseNet(QNetworkReply *reply) {
    qDebug() << "ArtWorkService::reponseNet";

    int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    if(statusCode==301||statusCode==302) {
        qDebug() << "301/302";
        QUrl newUrl = reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl();
        QNetworkRequest request(newUrl);
        manager->get(request);
        reply->close();
        reply->deleteLater();
        return;
    }

    QString vurl = reply->property(ART_VURL).toString();

    if(reply->error() == QNetworkReply::NoError) {
        QByteArray ar =reply->readAll();

        QString source = getImageFromJson(ar);
        if(source.isEmpty() == false) {

            // url image valide
            QString target = QString(
                        VFI.getPathFromVUrl( VFI.getParentVUrl(vurl))
                        + "/" +
                        VFI.getFileName(source)
                        );
            qDebug() << " ArtWorkService cover : " << source << " As : " << target;

            QDOWNSERVICE.start( source, target ,
                                reply->property(ART_MESSAGECOVER).isValid() ?
                                    reply->property(ART_MESSAGECOVER).toString().replace("%20"," ") :""
                                    );

            reply->close();
            reply->deleteLater();
            return;
        }
    }

    QFile f( VFI.getParentVUrl(vurl) + "/" + ART_PASDEPOCHETTE );

    if(f.open(QIODevice::WriteOnly)) {
        f.close();
    }

    reply->close();
    reply->deleteLater();
}

/**
 * @brief ArtWorkService::downloadSuccess
 * @param url
 * @param target
 */
void ArtWorkService::downloadSuccess(QUrl const &url, QString const &target ) {

    // on verifie si c'est une url originale de audioscrobbler
    if( url.toString().contains(ART_ALBUMCOVERART_DOMAIN) == false) {
        return;
    }

    QString vurl = VFI.getVUrl(target);

    if( VFI.isDirectory(vurl) == false) {
        vurl = VFI.getParentVUrl(vurl);
    }

    try {
        // on retablie la date originale (modifié par le download)
        // afin que la liste "recent" soit correcte
        QFileInfo info = infoRep.value(vurl);

        utimbuf t {
            info.lastRead().toMSecsSinceEpoch()/1000,
            info.lastModified().toMSecsSinceEpoch()/1000
        };
        utime(VFI.getPathFromVUrl(vurl).toLocal8Bit(), &t);

    } catch(const std::exception & e) {
    }

    emit artFound(vurl, target);
}

/**
 * @brief erreur de telechargement -> envoie le signal pochette pas trouvé
 * @param url
 * @param status
 */
void ArtWorkService::downloadError(QUrl const & url,  QString const &target, QNetworkReply::NetworkError status) {

    // on verifie si c'est une url originale de audioscrobbler
    if( url.toString().contains(ART_ALBUMCOVERART_DOMAIN) == false) {
        return;
    }

//    QString vurl = VFI.getVUrl(target);

//    if( VFI.isDirectory(vurl) == false) {
//        vurl = VFI.getParentVUrl(vurl);
//    }

    emit artNotFound(url.toString(), target);
}

/**
 * @brief recherche la meilleure miniature depuis les donnée json
 * @param data
 * @return url de la miniature ou QString:null si aucune
 */
QString ArtWorkService::getImageFromJson(const QByteArray &data) {

    try {
        QJsonObject json_obj = QJsonDocument::fromJson(data).object();
        QVariantMap  result = json_obj.toVariantMap();
        QList<QVariant> images = result["album"].toMap()["image"].toList();

        foreach (QVariant v, images) {
            QVariantMap m = v.value<QVariantMap>();
            if(m["size"].value<QString>().toLower().compare("mega") == 0) {
                return m["#text"].value<QString>();
            }
        }
    } catch ( const std::exception & e ) {
    }

    return QString::null;
}
