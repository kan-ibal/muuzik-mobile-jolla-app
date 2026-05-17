#include "lastfm.h"
#include <QUrl>
#include <QRegularExpression>
#include <QRegularExpressionMatchIterator>
#include <QRegExp>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QStandardPaths>
#include <qdebug.h>

#include "../app.h"
#include "config.h"

#include "../vi18n/vi18n.h"
#include "../qnetworkwatcher/qnetworkwatcher.h"
#include "../qdownloader/qdownloaderservice.h"
#include "../virtualfiles/VFservice.h"

#include <nemonotifications-qt5/notification.h>

#define LOCALBIO ".bio"
#define LOCALTOP ".top"
#define LOCALIMG ".img"

#define REQUETTECHEMINID "requettecheminid"
#define REQUETTEMDIB "requettemdib"


#ifndef LASTFMIMGORIGINE
#define LASTFMIMGORIGINE "https://lastfm-img2.akamaized.net/i/u/300x300"
#endif

#ifndef DATAIMGURL
#define DATAIMGURL "data://.img"
#endif

#define ARTISTGETINFO "http://ws.audioscrobbler.com/2.0/?method=artist.getinfo&artist=%1&api_key=edb5727ffe1a4ac4c0aca7c4d068aa16&autocorrect=1&format=json"
#define ARTISTTOPALBUMS "http://ws.audioscrobbler.com/2.0/?method=artist.gettopalbums&mbid=%1&api_key=edb5727ffe1a4ac4c0aca7c4d068aa16&limit=20&format=json"
#define TYPEMETHODE "typemethode"
#define  ARTISTTOPALBUMSMAX 20

//static QRegExp BLANK("[ -:#';,?&$%()=+\\[\\]!*]");
static QRegExp BLANK("[ -#':;,?$%()=+\\[\\]!*-]");
static QRegularExpression LASTFMIMGSOURCE("(https://lastfm-img2.akamaized.net/i/u/300x300/[0-9a-f]+\\.png)");

/**
 * @brief lastfmService::lastfmService
 * @param parent
 */
lastfmService::lastfmService(QObject *parent) :
    QObject(parent),raf(false), manager(new QNetworkAccessManager(this)){

    QDir d(QString(QStandardPaths::writableLocation(QStandardPaths::DataLocation)));
    d.mkpath(LOCALBIO);
    d.mkpath(LOCALTOP);
    d.mkpath(LOCALIMG);

    manager->setNetworkAccessible(QNetworkAccessManager::Accessible);
    connect(manager, SIGNAL(finished(QNetworkReply *)), this, SLOT(reponseNet(QNetworkReply*)));
}

/**
 * @brief ::getInstance
 * @return
 */
lastfmService& lastfmService::getInstance() {
    static lastfmService instance;
    return instance;
}

/**
 * @brief register the service into the context so it can be used in qml
 * @param context
 * @param name : register service on this name, default to LASTFMSERVICE_QML_NAME
 */
void lastfmService::init(QQmlContext *context, QString name) {
    context->setContextProperty(name,&lastfmService::getInstance());
}

/**
 * @brief lastfmService::artiste
 * @param nom
 * @return info en cache ou vide si en recherche
 */
QString lastfmService::biographie(QString const &nom) {

    if(raf) {
        return QString();
    }
    QString lnom = nom.toLower();
//    qDebug() << "Value lnom: " << lnom;

    // cherche en local
    QString tmp = lireBio(lnom);
    if(tmp.length()>0) {
        return tmp;
    }

    if(QNWI.getNetwork() == NetworkType::NONE ) {
        // pas de cache et pas de reseau
        return QString();
    }

    if(configService::getInstance().getWifi() && (QNWI.getWifi()==false)) {
        return QString();
    }

    QUrl url(QStringLiteral(ARTISTGETINFO).arg(lnom.replace(BLANK, "%20").replace("&", "%26")));
//    qDebug() << "Value url: " << url;
    QNetworkRequest request(url);
    request.setRawHeader("User-Agent", "User Agent");
    QNetworkReply *reply = manager->get(request);
    reply->setProperty(REQUETTECHEMINID,lnom);

    raf=true;
    emit enCoursChanged();

    // pas de bio en local
    return QString();
}

/**
     * @brief retourne la liste des album de l'artiste au mbid
     * @param mbid de l'artiste
     * @return si existe déjà ou vide si en attente depuis web
 */
QString lastfmService::topAlbum(QString const &mbid) {
    // ne gere pas le "en cours" pour les top album, chargement fait asynch

    // cherche en local
    QString tmp = lireTopAlbum(mbid);
    if(tmp.length()>0) {
        return tmp;
    }

    if(QNWI.getConnected() == NetworkType::NONE) {
        // pas de cache et pas de reseau
        return QString();
    }

    if(configService::getInstance().getWifi() && (QNWI.getWifi()==false)) {
        return QString();
    }

    QUrl url(QStringLiteral(ARTISTTOPALBUMS).arg(mbid));
    QNetworkRequest request(url);
    request.setRawHeader("User-Agent", "User Agent");
    QNetworkReply *reply = manager->get(request);
    reply->setProperty(REQUETTEMDIB,mbid);

    // pas de bio en local
    return QString();
}

/**
 * @brief efface la version locale d'une bio
 * @param nom de de la bio
 */
void lastfmService::effacerBio(QString const &nom) {

    QString lnom = nom.toLower();

    QFile f(QStandardPaths::writableLocation(QStandardPaths::DataLocation)
            + "/" +LOCALBIO +"/" +  lnom + ".json");
    f.remove();
}

/**
 * @brief lastfmService::effacerTopAlbum
 * @param mbid
 */
void lastfmService::effacerTopAlbum(QString const &mbid){

    QFile f(QStandardPaths::writableLocation(QStandardPaths::DataLocation)
            + "/" +LOCALTOP +"/" + mbid  + ".json");
    f.remove();
}

/**
 * @brief lastfmService::reponseBio
 * @param reply
 */
void lastfmService::reponseNet(QNetworkReply *reply) {
    int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    if(statusCode==301||statusCode==302) {
        qDebug() << "301/302";
        QUrl newUrl = reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl();
        QNetworkRequest request(newUrl);
        manager->get(request);
        return;
    }

    QString res;

    if(reply->error() == QNetworkReply::NoError) {
        QByteArray ar =reply->readAll();

        QJsonDocument doc = QJsonDocument::fromJson(ar);
        if(doc.isObject()) {
            if( doc.object().value("error") == QJsonValue::Undefined ) {
                res.append(ar);
            }
        }

        // c'est une bio ????
        if(res.length()>0 && reply->property(REQUETTECHEMINID).isValid()) {
            ecrireBio(reply->property(REQUETTECHEMINID).toString(),res);

            // pas de "en cours" pour les requettes album
            raf=false;
            emit enCoursChanged();

            emit trouveBio(res);

        } else if(res.length()>0 && reply->property(REQUETTEMDIB).isValid()) {
            // c'est une liste top album
            ecrireTopAlbum(reply->property(REQUETTEMDIB).toString(),res);
            emit trouveTopAlbum(res);
        }
    }

    reply->close();
    reply->deleteLater();
}

/**
* @brief recherche une bio en local
* @param nom
* @return
*/
QString lastfmService::lireBio(QString const &nom) {
    return lire(QStandardPaths::writableLocation(QStandardPaths::DataLocation)
                + "/" +LOCALBIO +"/" +  nom + ".json");
}

/**
* @brief recherche un top album
* @param nom
* @return
*/
QString lastfmService::lireTopAlbum(QString const &mbid) {
    return lire(QStandardPaths::writableLocation(QStandardPaths::DataLocation)
                + "/" +LOCALTOP +"/" + mbid  + ".json");
}

/**
 * @brief lire un fichier json / texte; fait la convertion url
 * @param url
 * @return le fichier sous forme texte
 */
QString lastfmService::lire(QString const &url) {
    QString res;

    QFile f(url.toUtf8());

    if(f.exists()==false) {
        f.setFileName( QString(url).replace(' ', "%20"));
        f.setFileName( QString(url).replace('&', "&#38;"));
    }


    if( f.open( QIODevice::ReadOnly | QIODevice::Text) ) {
        QTextStream stream(&f);
        stream.setCodec("UTF-8");
        res= stream.readAll();

        f.close();

        // convertion des url data:// en full local (file://)
        QString tmp(
                    "file://" +
                    QStandardPaths::writableLocation(QStandardPaths::DataLocation)
                    + "/" + LOCALIMG );
        res.replace(DATAIMGURL, tmp);
    }

    return res;
}

/**
 * @param nom
 * @param data
 */
void lastfmService::ecrireBio(QString const &nom, QString const &data) {
    ecrire(QStandardPaths::writableLocation(QStandardPaths::DataLocation)
           + "/" +LOCALBIO + "/" +nom + ".json",data);
}

/**
 * @param nom
 * @param data
 */
void lastfmService::ecrireTopAlbum(QString const &mbid, QString const &data) {
    ecrire(QStandardPaths::writableLocation(QStandardPaths::DataLocation)
           + "/" +LOCALTOP + "/" +mbid + ".json",data);
}

/**
 * @brief ecrire un fichier json/txt ; fait la convertion url
 * @param nom
 */
void lastfmService::ecrire(QString const &url, QString const &data) {
    if(configService::getInstance().getPasDecache()) {
        return;
    }

    QFile f(url.toUtf8());
    if( f.exists()) {
        return;
    }

    if( f.open( QIODevice::WriteOnly) ==false ) {
        Notification n;
        n.setPreviewBody(vI18n::getInstance().get( "bio.erreur.ecriture" ));
        n.setHintValue("x-nemo-icon", "image://theme/icon-s-high-importance");
        n.publish();
        return;
    }

    QString tmp(data);
    tmp.replace(LASTFMIMGORIGINE, DATAIMGURL);

    QTextStream stream( &f );
    stream.setCodec("UTF-8");
    stream << tmp  << endl;

    f.close();

    // les images en local
    QRegularExpressionMatchIterator i = LASTFMIMGSOURCE.globalMatch(data);
    while (i.hasNext()) {
        QRegularExpressionMatch match = i.next();
        if (match.hasMatch()) {

            QString target = QString(
                        QStandardPaths::writableLocation(QStandardPaths::DataLocation)
                        + "/" + LOCALIMG + "/" + VFI.getFileName(match.captured(0))
                        );

            QDOWNSERVICE.start(match.captured(0),target);
        }
    }
}


//http://ws.audioscrobbler.com/2.0/?method=artist.gettopalbums&mbid=4c5ff1ac-dce8-4815-90f9-232c5ea8d262&api_key=edb5727ffe1a4ac4c0aca7c4d068aa16&format=json
