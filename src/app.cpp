#include "app.h"
#include <QFileInfo>
#include <QRegularExpression>
#include <qdebug.h>
#include <QDateTime>

#include <nemonotifications-qt5/notification.h>

#include "virtualfiles/VFservice.h"


const QRegularExpression App::REG_fichierimage( "\\.(jpg|jpeg|png)$", QRegularExpression::CaseInsensitiveOption);
const QRegularExpression App::REG_fichiercover( "\\.(mp3|flac)$", QRegularExpression::CaseInsensitiveOption);
const QRegularExpression App::REG_fichieraudio( VFService::FILTERAUDIO, QRegularExpression::CaseInsensitiveOption);
const QRegularExpression App::REG_fichierpourart( "\\.(mp3|flac)$", QRegularExpression::CaseInsensitiveOption);


const QString App::IMG_REPERTOIRE=QString("image://theme/icon-m-folder");
const QString App::IMG_AUDIO=QString("qrc:///img/son");
const QString App::IMG_ALBUM=QString("qrc:///img/album");
const QString App::IMG_TITRE=QString("qrc:///img/son");
const QString App::IMG_FAV=QString("image://theme/icon-m-favorite");
const QString App::IMG_HISTORIQUE=QString("qrc:///img/historique");
const QString App::IMG_DEFAULT=QString("image://theme/icon-m-note");

/**
 * @param v
 * @return
 */
QString App::tailleHumaine(const long v) {
    if(v>512000) {
        return QString::number( ((float)v/1048576.0), 'f',2 ) + "Gb";
    }
    return QString::number(((float)v/1024.0), 'f',2  ) + "Mb";
}

/**
 * @brief retourne une chaine sous la forme xx:xx:xx
 * @param v en seconde
 * @return
 */
QString App::tempsHumaine(const long v) {

    if(v >86400) {
        return QDateTime::fromMSecsSinceEpoch(v*1000).toString("d MMM h:m");
    }

    long tmp = v;
    QString l;
    l.reserve(8);
    if( tmp < 36000 ) {
        l.append("0");
    }
    l.append( QString::number(tmp / 3600));
    l.append(":");
    tmp = tmp % 3600;

    if( tmp < 360) {
        l.append("0");
    }
    l.append( QString::number(tmp / 60));
    l.append(":");
    tmp = tmp % 60;

    if( tmp < 60) {
        l.append("0");
    }
    l.append(QString::number(tmp));

    return QString(l);
}

/**
 * @brief true si la notification existe
 * @param id
 * @return
 */
Notification *App::trouverNotification(quint32 id) {

    QList<QObject *> l = Notification::notifications();
    if(l.length()>0) {
       foreach (QObject *o, l) {
            Notification *nn = (Notification *)o;
            if(nn->replacesId()== id) {
                qDebug() << id << " exists";
                return nn;
            }
        }
    }

    return NULL;
}




