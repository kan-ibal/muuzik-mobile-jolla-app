#include "metadataservice.h"
#include <QtQml>
#include <QMetaType>
#include <QQmlEngine>
#include <QDebug>
#include <QFuture>
#include <QtConcurrent/QtConcurrentRun>
#include <QtAlgorithms>
#include <QMutexLocker>

#include  <taglib/fileref.h>
#include  <taglib/tag.h>

#include "../virtualfiles/VFservice.h"
#include "../vi18n/vi18n.h"
#include "artwork.h"

/**
 * @brief MetaDataService::MetaDataService
 * @param parent
 */
MetaDataService::MetaDataService(QObject *parent) : QObject(parent), mutex(QMutex::Recursive) {
}

MetaDataService::~MetaDataService() {
    QMutexLocker lock(&mutex);
    qDeleteAll(cache);
    cache.clear();
    qDeleteAll(statCache);
    statCache.clear();
}

/**
 * @brief instance (singleton) for the service
 * @return
 */
MetaDataService& MetaDataService::getInstance() {
    static MetaDataService instance;
    return instance;
}

/**
 * @brief register the service into the context so it can be used in qml
 * @param context
 * @param name : register service on this name, default to METADATASERVICE_QML_NAME
 */
void MetaDataService::init(QQmlContext *context, QString name) {
    context->setContextProperty(name,&MetaDataService::getInstance());
    qmlRegisterType<MetaData>("org.vostok.metadata", 1, 0, "MetaData");
    qRegisterMetaType<QList<MetaData*> >( "QList<MetaData*>" );
    qRegisterMetaType<MetaDataList>( "MetaDataList" );
    qmlRegisterType<AlbumStat>("org.vostok.metadata", 1, 0, "AlbumStat");
}

/**
 * @brief get metadata from url.
 * If url doesn't define a file with metadata, return an empty structure
 * @param url virtual url to the media
 * @param soft : si true et que vurl est vers un repertoire / album, retourne
 * directement les info du premier media
 * @return meta data or null if not found (or can't read)
 */
MetaData *MetaDataService::getMetaData(const QString & vurl, bool soft) {
    QMutexLocker lock(&mutex);
    //qDebug() << "MetaDataService::getMetaData " << vurl;
    if(cache.contains(vurl)) {
        return cache.value(vurl);
    }

    if(VFI.exist(vurl) == false) {
        return NULL;
    }

    // si c'est un repertoire, on prend le premier media audio comme reference
    if(VFI.isDirectory(vurl) && soft) {
        qDebug() << "on fait le rep " << vurl;
        QList<VFInfo *> l = VFI.getSynch(vurl,VFService::FILTERAUDIO,VFService::FILE);

        if(l.length()==0) {
            qDebug()<< "pas d'audio !! " << vurl;
            return NULL;
        }

        // on regarde si les 2 premier media ont les mêmes auteur / album
        // si non, renvoi une liste "vide"
        MetaData *m1 =  MetaDataService::doGetMetaData(l.at(0)->getVUrl());

        m1->setDuree(0);

        if( l.length() > 1) {
            MetaData *m2 =  MetaDataService::doGetMetaData(l.at(1)->getVUrl());

            if(m2->getAlbum().compare(m1->getAlbum()) != 0 ||
                    m2->getArtiste().compare(m1->getArtiste()) != 0
                    ) {
                // pas les mêmes on initilaise m1, c'est la marque de media dif dans un rep
                m1->setAlbum("");
                m1->setArtiste("");
            }

            delete m2;
        }

        qDeleteAll(l);

        m1->setChemin(vurl);
qDebug() << "meta de " << vurl << " " << m1->getArtiste();
        cache.insert(vurl, m1);
        return m1;
    }

    // simple media ou pas de "soft"
    MetaData *m = MetaDataService::doGetMetaData(vurl);
    cache.insert(vurl, m);

    return m;
}

/**
 * @brief doGetMetaData
 * @param vurl
 * @return
 */
MetaData *MetaDataService::doGetMetaData(const QString & vurl) {
    MetaData *m = new MetaData();
    m->setChemin(vurl);
    // m->setPochette(ARTS.getArt(vurl));

    QString path = VFI.getPathFromVUrl(vurl);
    TagLib::FileRef fr(path.toUtf8().constData() );
    if( fr.isNull()==false) {
        if(fr.audioProperties() != NULL) {
            m->setDuree(fr.audioProperties()->length());
        }
        if(fr.tag() != NULL ) {
            m->setTitre(QString(fr.tag()->title().toCString(true)));
            m->setAlbum(QString(fr.tag()->album().toCString(true)));
            m->setArtiste(QString(fr.tag()->artist().toCString(true)));
            m->setAnnee(fr.tag()->year());
        }
    }

    if(m->getTitre().length()==0) {
        m->setTitre(VFI.getName(vurl));
    }
    //    if(m->getAlbum().length()==0) {
    //        m->setAlbum(VFI.getName(VFI.getParentVUrl(vurl)));
    //    }
    //    if(m->getArtiste().length()==0) {
    //        m->setArtiste(VI18N.get("navigateur.artiste.inconnu"));
    //    }

    return m;


}

/**
 * @brief getMetaDataAsQVariant
 * @param vurl
 * @param soft
 * @return
 */
QVariant MetaDataService::getMetaDataAsQVariant(const QString & vurl, bool soft) {
    MetaData *m = getMetaData(vurl,soft);
    if (!m) {
        return QVariant();
    }

    return QVariant::fromValue( MetaData::copy(m) );
}

/**
 * @brief recup les meta données mais par le signal MetaDataAvailable
 * @param vurl
 */
void MetaDataService::getMetaDataASynch(const QString & vurl,bool soft) {
    QFuture<void> f = QtConcurrent::run(doGet,vurl,soft);
}

/**
 * @brief qtconcurrent worker
 * @param vurl
 */
void MetaDataService::doGet(const QString &vurl,bool soft) {

    MetaData *m = MetaDataService::getInstance().getMetaData(vurl,soft);
    if(m != NULL) {
        emit MetaDataService::getInstance().metaDataAvailable(vurl, QVariant::fromValue(m) );
    }
}

/**
  * @brief calcule le temps total d'un "album"
  * @param url virtuelle de l'album/repertoire
  * @return temps en seconde
  */
AlbumStat *MetaDataService::getStatFromAlbum(const QString &vurl) {
    QMutexLocker lock(&mutex);

    if(statCache.contains(vurl)) {
        return statCache.value(vurl);
    }

    AlbumStat *stat = new AlbumStat();
    stat->setChemin(vurl);

    quint64 temps = 0;

    QList<VFInfo *> l = VFI.getSynch(vurl,VFService::FILTERAUDIO,VFService::FILE);
    foreach (VFInfo *f, l) {
        MetaData *m = getMetaData(f->getVUrl());
        if(m != NULL) {
            temps += m->getDuree();
            if(m->getArtiste().isEmpty() == false && stat->getArtiste().isEmpty() ) {
                // il est probable que si le tag artiste est defini, album aussi
                stat->setArtiste( m->getArtiste() );
                stat->setAlbum( m->getAlbum() );
                stat->setAnnee(m->getAnnee() );//@since 2.4
            }
        }
    }

    stat->setDuree(temps);
    stat->setPistes(l.length());
//qDebug() << "Stat " << stat->getPistes() << " -- " << vurl;

    if(l.length()>0) {
        qDeleteAll(l);
    }
    //    delete l;

    statCache.insert(vurl,stat);

    return stat;
}

/**
 * @brief MetaDataService::getStatFromAlbumAsQVariant
 * @param vurl
 * @return
 */
QVariant MetaDataService::getStatFromAlbumAsQVariant(const QString &vurl) {
    return QVariant::fromValue(getStatFromAlbum(vurl));
}

/**
  * @brief idem mais sur le signal timeAvailable
  * @param url virtuelle de l'album/repertoire
  */
void MetaDataService::getStatFromAlbumASynch(const QString &vurl) {
    QFuture<void> f = QtConcurrent::run(doGetStat,vurl);
}

/**
 * @brief qtconcurrent worker
 * @param vurl
 */
void MetaDataService::doGetStat(const QString &vurl) {

   QVariant s = MetaDataService::getInstance().getStatFromAlbumAsQVariant(vurl);
    emit MetaDataService::getInstance().statAvailable(vurl,s);
}
