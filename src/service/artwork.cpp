#include "artwork.h"
#include <QUrl>
#include <QRegularExpression>
#include <QRegularExpressionMatchIterator>
#include <QRegExp>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QDateTime>
#include <QSaveFile>
#include <QStandardPaths>
#include <QCryptographicHash>
#include <QDebug>
#include <QtAlgorithms>
#include <QFuture>
#include <QtConcurrent/QtConcurrentRun>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMutexLocker>
#include <utime.h>

#include "config.h"
#include "../app.h"
#include <taglib/taglib.h>
#include <taglib/fileref.h>
#include <taglib/tag.h>
#include <taglib/mpegfile.h>
#include <taglib/id3v2tag.h>
#include <taglib/attachedpictureframe.h>
#include <taglib/flacfile.h>
#include <taglib/flacpicture.h>
#include <taglib/mp4file.h>
#include <taglib/mp4tag.h>
#include <taglib/mp4coverart.h>
#include <taglib/vorbisfile.h>
#include <taglib/opusfile.h>
#include <taglib/xiphcomment.h>
#include <taglib/wavfile.h>
#include <taglib/aifffile.h>
#include <taglib/asffile.h>
#include <taglib/asftag.h>
#include <taglib/asfattribute.h>
#include <taglib/asfpicture.h>

#include "../vi18n/vi18n.h"
#include "../virtualfiles/VFservice.h"
#include "../util/webradioutil.h"
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
    QObject(parent), manager(new QNetworkAccessManager(this)), cacheMutex(QMutex::Recursive), lastfmwarning(false) {
    manager->setNetworkAccessible(QNetworkAccessManager::Accessible);
    connect(manager, SIGNAL(finished(QNetworkReply *)), this, SLOT(reponseNet(QNetworkReply*)));

    connect(this, SIGNAL(doNetSearch(QString)), this, SLOT(launchNetImpl(QString)), Qt::QueuedConnection);

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
    {
        QMutexLocker locker(&cacheMutex);
        if(cache.contains(vurl)) {
            return cache.value(vurl);
        }
    }

    if (WebRadioUtil::isStreamUrl(vurl)) {
        const WebRadioInfo info = WebRadioUtil::parse(vurl);
        const QString icon = info.iconUrl.trimmed();
        {
            QMutexLocker locker(&cacheMutex);
            cache.insert(vurl, icon);
        }
        return icon;
    }

    if(VFI.exist(vurl) == false) {
        return QString();
    }

    //-------------------------------------------
    // c'est un répertoire / album
    if(VFI.isDirectory(vurl)) {
        // on recherche une miniature dans le repertoire
        QString path = fromDirectory(vurl);
        if(!path.isEmpty()) {
            QMutexLocker locker(&cacheMutex);
            cache.insert(vurl,path);
            return path;
        }

        // on cherche une pochette embarquee dans les fichiers audio du repertoire
        QList<VFInfo *> l = VFI.getSynch(vurl, VFService::FILTERAUDIO, VFService::VFType::FILE);
        if(l.length()==0) {
            QMutexLocker locker(&cacheMutex);
            cache.insert(vurl,"");
            return QString();
        }

        QString audio_path = l.at(0)->getVUrl();
        foreach (VFInfo *info, l) {
            path = fromTag(info->getVUrl());
            if (!path.isEmpty()) {
                break;
            }
        }
        qDeleteAll(l);

        // trouve par fichier audio ???
        if(!path.isEmpty()) {
            QMutexLocker locker(&cacheMutex);
            cache.insert(vurl,path);
            return path;
        }

        // on a rien on lance une recherche net
        emit ArtWorkService::getInstance().doNetSearch(audio_path);

        return QString();
    }

    //-------------------------------------------
    // c'est un fichier audio direct
    QString path = fromTag(vurl);
    if(!path.isEmpty()) {
        QMutexLocker locker(&cacheMutex);
        cache.insert(vurl,path);
        return path;
    }

    path = fromDirectory(VFI.getParentVUrl(vurl));
    if(!path.isEmpty()) {
        QMutexLocker locker(&cacheMutex);
        cache.insert(vurl,path);
        return path;
    }

    // on a rien on lance une recherche net
    emit ArtWorkService::getInstance().doNetSearch(vurl);

    return QString();
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

namespace {

QString detectImageExtension(const TagLib::ByteVector &data, const QString &mimeHint) {
    const unsigned int sz = data.size();
    if (sz >= 4 &&
        static_cast<unsigned char>(data[0]) == 0x89 &&
        data[1] == 'P' && data[2] == 'N' && data[3] == 'G') {
        return QStringLiteral("png");
    }
    if (sz >= 3 &&
        static_cast<unsigned char>(data[0]) == 0xFF &&
        static_cast<unsigned char>(data[1]) == 0xD8 &&
        static_cast<unsigned char>(data[2]) == 0xFF) {
        return QStringLiteral("jpg");
    }
    if (sz >= 4 &&
        data[0] == 'G' && data[1] == 'I' && data[2] == 'F' && data[3] == '8') {
        return QStringLiteral("gif");
    }
    if (sz >= 12 &&
        data[0] == 'R' && data[1] == 'I' && data[2] == 'F' && data[3] == 'F' &&
        data[8] == 'W' && data[9] == 'E' && data[10] == 'B' && data[11] == 'P') {
        return QStringLiteral("webp");
    }
    if (sz >= 2 && data[0] == 'B' && data[1] == 'M') {
        return QStringLiteral("bmp");
    }

    QString lower = mimeHint.toLower();
    if (lower.contains(QStringLiteral("png"))) {
        return QStringLiteral("png");
    }
    if (lower.contains(QStringLiteral("gif"))) {
        return QStringLiteral("gif");
    }
    if (lower.contains(QStringLiteral("webp"))) {
        return QStringLiteral("webp");
    }
    if (lower.contains(QStringLiteral("bmp"))) {
        return QStringLiteral("bmp");
    }
    return QStringLiteral("jpg");
}

bool extractFromId3v2(TagLib::ID3v2::Tag *tag, TagLib::ByteVector &outData, QString &outMime) {
    if (!tag) {
        return false;
    }
    const TagLib::ID3v2::FrameList frames = tag->frameList("APIC");
    if (frames.isEmpty()) {
        return false;
    }

    TagLib::ID3v2::AttachedPictureFrame *fallback = 0;
    for (TagLib::ID3v2::FrameList::ConstIterator it = frames.begin(); it != frames.end(); ++it) {
        TagLib::ID3v2::AttachedPictureFrame *pic = dynamic_cast<TagLib::ID3v2::AttachedPictureFrame *>(*it);
        if (!pic || pic->picture().isEmpty()) {
            continue;
        }
        if (pic->type() == TagLib::ID3v2::AttachedPictureFrame::FrontCover) {
            outData = pic->picture();
            outMime = QString::fromUtf8(pic->mimeType().toCString(true));
            return true;
        }
        if (!fallback) {
            fallback = pic;
        }
    }

    if (fallback) {
        outData = fallback->picture();
        outMime = QString::fromUtf8(fallback->mimeType().toCString(true));
        return true;
    }
    return false;
}

bool extractFromFlacPictures(const TagLib::List<TagLib::FLAC::Picture *> &pics, TagLib::ByteVector &outData, QString &outMime) {
    if (pics.isEmpty()) {
        return false;
    }

    TagLib::FLAC::Picture *fallback = 0;
    for (TagLib::List<TagLib::FLAC::Picture *>::ConstIterator it = pics.begin(); it != pics.end(); ++it) {
        TagLib::FLAC::Picture *pic = *it;
        if (!pic || pic->data().isEmpty()) {
            continue;
        }
        if (pic->type() == TagLib::FLAC::Picture::FrontCover) {
            outData = pic->data();
            outMime = QString::fromUtf8(pic->mimeType().toCString(true));
            return true;
        }
        if (!fallback) {
            fallback = pic;
        }
    }

    if (fallback) {
        outData = fallback->data();
        outMime = QString::fromUtf8(fallback->mimeType().toCString(true));
        return true;
    }
    return false;
}

bool extractEmbeddedArtwork(TagLib::File *file, TagLib::ByteVector &outData, QString &outMime) {
    if (!file) {
        return false;
    }

    if (TagLib::MPEG::File *mpeg = dynamic_cast<TagLib::MPEG::File *>(file)) {
        if (extractFromId3v2(mpeg->ID3v2Tag(), outData, outMime)) {
            return true;
        }
    }

    if (TagLib::FLAC::File *flac = dynamic_cast<TagLib::FLAC::File *>(file)) {
        if (extractFromFlacPictures(flac->pictureList(), outData, outMime)) {
            return true;
        }
        if (extractFromId3v2(flac->ID3v2Tag(), outData, outMime)) {
            return true;
        }
    }

    if (TagLib::MP4::File *mp4 = dynamic_cast<TagLib::MP4::File *>(file)) {
        TagLib::MP4::Tag *mp4Tag = mp4->tag();
        if (mp4Tag) {
            TagLib::MP4::CoverArtList covers;
#if (TAGLIB_MAJOR_VERSION > 1) || (TAGLIB_MAJOR_VERSION == 1 && TAGLIB_MINOR_VERSION >= 12)
            if (mp4Tag->contains("covr")) {
                covers = mp4Tag->item("covr").toCoverArtList();
            }
#else
            const TagLib::MP4::ItemListMap &items = mp4Tag->itemListMap();
            if (items.contains("covr")) {
                covers = items["covr"].toCoverArtList();
            }
#endif
            for (TagLib::MP4::CoverArtList::ConstIterator it = covers.begin(); it != covers.end(); ++it) {
                if (!it->data().isEmpty()) {
                    outData = it->data();
                    switch (it->format()) {
                    case TagLib::MP4::CoverArt::PNG:
                        outMime = QStringLiteral("image/png");
                        break;
                    case TagLib::MP4::CoverArt::GIF:
                        outMime = QStringLiteral("image/gif");
                        break;
                    case TagLib::MP4::CoverArt::BMP:
                        outMime = QStringLiteral("image/bmp");
                        break;
                    default:
                        outMime = QStringLiteral("image/jpeg");
                        break;
                    }
                    return true;
                }
            }
        }
    }

#if (TAGLIB_MAJOR_VERSION > 1) || (TAGLIB_MAJOR_VERSION == 1 && TAGLIB_MINOR_VERSION >= 11)
    if (TagLib::Ogg::Vorbis::File *vorbis = dynamic_cast<TagLib::Ogg::Vorbis::File *>(file)) {
        if (vorbis->tag() && extractFromFlacPictures(vorbis->tag()->pictureList(), outData, outMime)) {
            return true;
        }
    }

    if (TagLib::Ogg::Opus::File *opus = dynamic_cast<TagLib::Ogg::Opus::File *>(file)) {
        if (opus->tag() && extractFromFlacPictures(opus->tag()->pictureList(), outData, outMime)) {
            return true;
        }
    }
#endif

    if (TagLib::RIFF::WAV::File *wav = dynamic_cast<TagLib::RIFF::WAV::File *>(file)) {
        if (extractFromId3v2(wav->ID3v2Tag(), outData, outMime)) {
            return true;
        }
    }

    if (TagLib::RIFF::AIFF::File *aiff = dynamic_cast<TagLib::RIFF::AIFF::File *>(file)) {
        if (extractFromId3v2(aiff->tag(), outData, outMime)) {
            return true;
        }
    }

    if (TagLib::ASF::File *asf = dynamic_cast<TagLib::ASF::File *>(file)) {
        TagLib::ASF::Tag *asfTag = asf->tag();
        if (asfTag) {
            TagLib::ASF::AttributeList attrs;
#if (TAGLIB_MAJOR_VERSION > 1) || (TAGLIB_MAJOR_VERSION == 1 && TAGLIB_MINOR_VERSION >= 12)
            if (asfTag->contains("WM/Picture")) {
                attrs = asfTag->attribute("WM/Picture");
            }
#else
            const TagLib::ASF::AttributeListMap &attrMap = asfTag->attributeListMap();
            if (attrMap.contains("WM/Picture")) {
                attrs = attrMap["WM/Picture"];
            }
#endif
            for (TagLib::ASF::AttributeList::ConstIterator it = attrs.begin(); it != attrs.end(); ++it) {
                TagLib::ASF::Picture pic = it->toPicture();
                if (pic.isValid() && !pic.picture().isEmpty()) {
                    outData = pic.picture();
                    outMime = QString::fromUtf8(pic.mimeType().toCString(true));
                    return true;
                }
            }
        }
    }

    return false;
}

} // namespace

/**
 * @brief getFromTag
 * @param vurl
 * @return path ou QString:null
 */
QString ArtWorkService::fromTag(const QString &vurl) {
    QString filePath = VFI.getPathFromVUrl(vurl);
    if (filePath.isEmpty()) {
        return QString();
    }

    QFileInfo srcInfo(filePath);
    if (!srcInfo.exists() || !srcInfo.isReadable()) {
        return QString();
    }

    QString cacheBaseDir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
    if (!cacheBaseDir.isEmpty()) {
        cacheBaseDir += QStringLiteral("/art");
        QString key = filePath + QStringLiteral("|") + QString::number(srcInfo.size()) +
                      QStringLiteral("|") + QString::number(srcInfo.lastModified().toMSecsSinceEpoch());
        QString hash = QString::fromLatin1(QCryptographicHash::hash(key.toUtf8(), QCryptographicHash::Md5).toHex());
        const char *exts[] = {"jpg", "png", "gif", "webp", "bmp"};
        for (size_t i = 0; i < sizeof(exts) / sizeof(exts[0]); ++i) {
            QString candidate = cacheBaseDir + QStringLiteral("/") + hash + QStringLiteral(".") + QLatin1String(exts[i]);
            QFileInfo cachedInfo(candidate);
            if (cachedInfo.exists() && cachedInfo.isReadable() && cachedInfo.size() > 0) {
                return candidate;
            }
        }
    }

    TagLib::FileRef fr(QFile::encodeName(filePath).constData());
    if (fr.isNull() == false) {
        TagLib::ByteVector imgData;
        QString mimeHint;
        if (extractEmbeddedArtwork(fr.file(), imgData, mimeHint) && !imgData.isEmpty() && !cacheBaseDir.isEmpty()) {
            QDir dir;
            if (dir.mkpath(cacheBaseDir)) {
                QString key = filePath + QStringLiteral("|") + QString::number(srcInfo.size()) +
                              QStringLiteral("|") + QString::number(srcInfo.lastModified().toMSecsSinceEpoch());
                QString hash = QString::fromLatin1(QCryptographicHash::hash(key.toUtf8(), QCryptographicHash::Md5).toHex());
                QString ext = detectImageExtension(imgData, mimeHint);
                QString targetPath = cacheBaseDir + QStringLiteral("/") + hash + QStringLiteral(".") + ext;

                QFileInfo existingInfo(targetPath);
                if (existingInfo.exists() && existingInfo.size() > 0) {
                    return targetPath;
                }

                QSaveFile outFile(targetPath);
                if (outFile.open(QIODevice::WriteOnly)) {
                    qint64 written = outFile.write(imgData.data(), static_cast<qint64>(imgData.size()));
                    if (written == static_cast<qint64>(imgData.size()) && outFile.commit()) {
                        return targetPath;
                    }
                    outFile.cancelWriting();
                }
            }
        }

        if (fr.tag() != NULL) {
            QString album = QString::fromUtf8(fr.tag()->album().toCString(true));
            QString artiste = QString::fromUtf8(fr.tag()->artist().toCString(true));

            if (album.length() > 0 && artiste.length() > 0) {
                QString url(
                            QDir::homePath() + "/.cache/media-art/album-" +
                            QCryptographicHash::hash(
                                artiste.toLower().normalized(QString::NormalizationForm_KD).toUtf8(),
                                QCryptographicHash::Md5).toHex() +
                            "-" +
                            QCryptographicHash::hash(
                                album.toLower().normalized(QString::NormalizationForm_KD).toUtf8(),
                                QCryptographicHash::Md5).toHex() +
                            ".jpeg"
                            );

                QFileInfo info(url);
                if (info.exists() && info.isReadable()) {
                    return url;
                }
            }
        }
    }

    return QString();
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

    TagLib::FileRef fr(QFile::encodeName(path).constData() );
    if( fr.isNull() || fr.tag() == NULL ) {
        return;
    }

    QString album = QString::fromUtf8(fr.tag()->album().toCString(true));
    QString artiste = QString::fromUtf8(fr.tag()->artist().toCString(true));
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
