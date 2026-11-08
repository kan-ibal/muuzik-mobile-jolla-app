#include "qdownloader.h"
#include <QDebug>
#include <QFileInfo>
#include <QDir>

#ifdef LIBSAILFISHAPP_SAILFISHAPP_H
#include <nemonotifications-qt5/notification.h>
#endif

/**
 * @brief QDownloader::QDownloader
 * @param parent
 */
QDownloader::QDownloader(QObject *parent) : QObject(parent) {
    manager = new QNetworkAccessManager(this);
}

/**
 * @brief QDownloader::~QDownloader
 */
QDownloader::~QDownloader() {
    manager->deleteLater();
}

/**
 * @brief QDownloader::setFile
 * @param source
 * @param target
 * @param message
 */
bool QDownloader::start(const QString &source, const QString &target, const QString &message) {

    // bad params or already running
    if(source.isEmpty() || target.isEmpty() || ( url.isEmpty() == false) ) {
#ifdef QT_DEBUG
        qDebug() << "QDownloader::start : empty source, target or QDownlader already in use";
#endif
        return false;
    }

    file.setFileName(target);
    if(file.exists()) {
#ifdef QT_DEBUG
        qDebug() << "QDownloader::start : file exist : " << target;
#endif
        return false;
    }

    // mk path to file
    if( QFileInfo(file).absoluteDir().mkpath(".") == false ) {
#ifdef QT_DEBUG
        qDebug() << "QDownloader::start : unable to create target directory for " << target;
#endif
        return false;
    }

    if(file.open(QIODevice::WriteOnly) ==false) {
#ifdef QT_DEBUG
        qDebug() << "QDownloader::start : unable to create target file : " << target;
#endif
        return false;
    }

    notification = message;

    QNetworkRequest request;

    url.setUrl(source);
    request.setUrl(url);
    reply = manager->get(request);

    connect(reply,SIGNAL(downloadProgress(qint64,qint64)),this,SLOT(onDownloadProgress(qint64,qint64)));
    connect(manager,SIGNAL(finished(QNetworkReply*)),this,SLOT(onFinished(QNetworkReply*)));
    connect(reply,SIGNAL(readyRead()),this,SLOT(onReadyRead()));
    connect(reply,SIGNAL(finished()),this,SLOT(onReplyFinished()));

    return true;
}

/**
 * @brief QDownloader::onDownloadProgress
 * @param bytesRead
 * @param bytesTotal
 */
void QDownloader::onDownloadProgress(qint64 bytesRead,qint64 bytesTotal) {
    //   qDebug(QString::number(bytesRead).toLatin1() +" - "+ QString::number(bytesTotal).toLatin1());
    emit downloading(url,bytesRead,bytesTotal);
}

/**
 * @brief QDownloader::onFinished
 * @param reply
 */
void QDownloader::onFinished(QNetworkReply * reply) {

    QNetworkReply::NetworkError status = reply->error();

    if(reply->isOpen()) {
        reply->close();
    }
    reply->deleteLater();

    if(file.isOpen()) {
        file.close();
    }

    // found error
    if(status != QNetworkReply::NoError) {
#ifdef QT_DEBUG
        qDebug() << "QDownloader : " << status <<  " for " << url;
#endif

        // delete tempory file
        file.remove();
        emit error(url, QFileInfo(file).absoluteFilePath(), status);
        return;
    }

#ifdef LIBSAILFISHAPP_SAILFISHAPP_H
    if(notification.length()>0) {
        Notification n;
        n.setPreviewBody(notification);
        n.setHintValue("x-nemo-icon", "image://theme/icon-s-cloud-download");
        n.publish();
    }
#endif

    emit success(url, QFileInfo(file).absoluteFilePath());
}

/**
 * @brief QDownloader::onReadyRead
 */
void QDownloader::onReadyRead() {
    file.write(reply->readAll());
}

/**
 * @brief QDownloader::onReplyFinished
 */
void QDownloader::onReplyFinished() {
    if(file.isOpen()) {
        file.close();
    }
}
