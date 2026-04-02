#include "parole.h"
#include <QtQml>
#include <QMetaType>
#include <QQmlEngine>
#include <QStringList>
#include <QUrl>
#include <QRegularExpression>
#include <QRegularExpressionMatchIterator>
#include <QRegExp>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <qdebug.h>
#include <utime.h>

#include "../vi18n/vi18n.h"
#include "../qnetworkwatcher/qnetworkwatcher.h"
#include "../virtualfiles/VFservice.h"
#include "config.h"
#include "../app.h"

#include <nemonotifications-qt5/notification.h>

#ifndef URLID
#define URLID "URLID"
#endif

#define PAROLECACHEDIR ".lyrics"

static QRegExp BLANK("[ -:#';,?&$%()=+\\[\\]!*]");
static QRegExp NOHTML("<br ?/?>",Qt::CaseInsensitive);
//static QRegularExpression LY("<div class=[\"\']lyricbox[\"\']>(.*)(<!--|<div)");
static QRegularExpression LY("</title><meta content=(.*)(name=[\"\']description[\"\']>)");
//<div class="lyricbox">... <div>
//<!--
static QRegularExpression DEC("([0-9]+)");

/**
 * @brief paroleService::paroleService
 * @param parent
 */
paroleService::paroleService(QObject *parent) :
    QObject(parent),raf(false), manager(new QNetworkAccessManager(this)){

    manager->setNetworkAccessible(QNetworkAccessManager::Accessible);
    connect(manager, SIGNAL(finished(QNetworkReply *)), this, SLOT(reponse(QNetworkReply*)));
}

/**
 * @brief ::getInstance
 * @return
 */
paroleService& paroleService::getInstance() {
    static paroleService instance;
    return instance;
}

/**
 * @brief register the service into the context so it can be used in qml
 * @param context
 * @param name : register service on this name, default to PAROLESERVICE_QML_NAME
 */
void paroleService::init(QQmlContext *context, QString name) {
    context->setContextProperty(name,&paroleService::getInstance());
}

/**
 * @brief retrouve les parole pour chemin, artiste, titre
 * Si trouve en local, retourne sinon lance une requette web
 * @param chemin virtuel vers le fichier audio, utilisé pour le cache local
 * @param artiste
 * @praram titre
 * @return les texte ou vide si en recherche web
 */
QString paroleService::chercher(const QString & url,const QString & artiste, const QString & titre) {

    if(raf) {
        return QString();
    }

    // cherche un local
    QString tmp = lecture(url);
    if(tmp.length()>0 ) {
        return tmp;
    }

    if(QNWI.getConnected() == NetworkType::NONE) {
        // pas de cache et pas de reseau
        return QString();
    }

    if(configService::getInstance().getWifi() && (QNWI.getWifi()==false)) {
        return QString();
    }

    QUrl remoteUrl(QStringLiteral("https://www.letras.com/%1/%2/")
                   .arg(versMajuscule(artiste))
                   .arg(versMajuscule(titre)));

    QNetworkRequest request(remoteUrl);

    request.setRawHeader("User-Agent", "User Agent");
    //  request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    QNetworkReply *reply = manager->get(request);
    reply->setProperty(URLID,url);

    raf=true;
    emit enCoursChanged();

    // pas de parole en local mais si pas de telechargement direct renvoi l'url
    if(configService::getInstance().getParoleDirect()==false) {
        return remoteUrl.toString();
    }

    return QString();
}

/**
 * @brief reponse reseau
 * @param reply
 */
void paroleService::reponse(QNetworkReply *reply) {
    int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    if(statusCode==301||statusCode==302) {
        QUrl newUrl = reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl();
        QNetworkRequest request(newUrl);
        manager->get(request);
        return;
    }

    QString res;

    if(reply->error() == QNetworkReply::NoError) {
        QString tmp(reply->readAll());
        QRegularExpressionMatch m=LY.match(tmp);
        if(m.hasMatch()) {
            res.append(paroleService::decode(m.captured(1).replace(NOHTML, RETOURLIGNE)));
        }
    }

    if(res.length()>0 && reply->property(URLID).isValid()) {
        QString url = reply->property(URLID).toString();
        ecriture(url,res);
    }

    raf=false;
    emit trouve(res);
    emit enCoursChanged();

    reply->close();
}

/**
 * @brief decode une base &#xxx;
 * @return
 */
QString *paroleService::decode(QString const v) {
    QString *res = new QString();

    QRegularExpressionMatchIterator i = DEC.globalMatch(v);
    while (i.hasNext()) {
        QRegularExpressionMatch match = i.next();
        if (match.hasMatch()) {
            res->append(QChar(match.captured(0).toUInt()));
        }
    }

    return res;
}

/**
* @brief ecrit les paroles en local pour le fichier chemin
* @param url virtuel vers le fichier audio
* @param donnee
*/
void paroleService::ecriture(QString const &url, QString const &donnee) {

    QString vparent = VFI.getParentVUrl(url);
    QString parent = VFI.getPathFromVUrl(vparent);

    if(VFI.exist(vparent + "/" + PAROLECACHEDIR) ==false) {
        // on recup la date du rep parent car elle va changer lors de la creation de PAROLECACHEDIR
        QFileInfo info(parent);

        QDir d(parent + "/" + PAROLECACHEDIR);
        d.mkpath(".");

        utimbuf t {
            info.lastRead().toMSecsSinceEpoch()/1000,
                    info.lastModified().toMSecsSinceEpoch()/1000
        };
        utime(parent.toLocal8Bit(), &t);
    }

    QString fichier = VFI.getName(url);
    QFile f(parent + "/" + PAROLECACHEDIR + "/" + fichier + ".txt");
    if( f.exists()) {
        return;
    }

    if( f.open( QIODevice::WriteOnly) ==false ) {
        Notification n;
        n.setPreviewBody(vI18n::getInstance().get( "parole.erreur.ecriture" ));
        n.setHintValue("x-nemo-icon", "image://theme/icon-s-high-importance");
        n.publish();
        return;
    }

    // ajoute en local
    QTextStream stream( &f );
    stream.setCodec("UTF-8");
    stream << donnee << "\n" << endl;

    f.close();
}

/**
 * @brief charge les paroles à partir d'une version locale
 * @param chemin virtuel vers le fichier audio
 * @return empty si aucune
 */
QString paroleService::lecture(QString const &url) {

    QString chemin = VFI.getPathFromVUrl(url);
    QString path = VFI.getParentVUrl(chemin);
    QString fichier = VFI.getName(chemin);

    QFile f(path + "/" + PAROLECACHEDIR + "/" + fichier + ".txt");

    QString res;
    if( f.open( QIODevice::ReadOnly) ) {
        QTextStream stream(&f);
        stream.setCodec("UTF-8");
        res= stream.readAll();

        f.close();
    }

    return res;
}

/**
 * @brief efface la version locale
 * @param chemin virtuel vers le fichier audio
 */
void paroleService::effacer(const QString & url) {

    QString chemin = VFI.getPathFromVUrl(url);
    QString path = VFI.getParentVUrl(chemin);
    QString fichier = VFI.getName(chemin);

    QFile f(path + "/" + PAROLECACHEDIR + "/" + fichier + ".txt");
    f.remove();
}

/**
 * @brief tous les mots avec premier lettre en majuscule
 * @param str
 * @param sep
 * @return
 */
QString paroleService::versMajuscule(QString const &str, QString const &sep) {

    QStringList tmp = str.toLower().split(BLANK,QString::SkipEmptyParts);
    QStringList list;

    foreach (QString s, tmp) {
        s[0] = s[0].toUpper();
        list.append(s);
    }

    return list.join(sep);
}


//http://lyrics.wikia.com/wiki/The_Pretty_Reckless:Kill_Me
//http://lyrics.wikia.com/wiki/Pink_Floyd:Another_Brick_In_The_Wall_Part_2
//<div class="lyricbox">... <div>
//<!--

//http://www.metrolyrics.com/kill-me-lyrics-pretty-reckless.html
