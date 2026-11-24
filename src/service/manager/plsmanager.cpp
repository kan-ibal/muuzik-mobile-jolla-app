#include "plsmanager.h"
#include <QTextStream>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QUrl>
#include <qregularexpression.h>
#include <QHash>
#include <QDebug>

#include "../config.h"
#include "../metadataservice.h"

#include "../../virtualfiles/VFservice.h"
#include "../../util/webradioutil.h"

plsManager::plsManager(QObject *parent) : QObject(parent) {
}

/**
 * @brief Write data into 'disk'
 * @param list list of title
 * @param path full absolute path to the playlist,if exit, override
 * @return true if list writen
 */
namespace {
struct PlsEntry {
    QString filePath;
    QString title;
    quint64 length;
    bool isStream;
};
}

bool plsManager::marshaller(const QList<MetaData *> list ,const QString &path ) {

    QList<PlsEntry> entries;
    foreach (MetaData *s, list) {
        if (!s) {
            continue;
        }
        if (WebRadioUtil::isStreamUrl(s->getChemin())) {
            PlsEntry entry;
            entry.filePath = s->getChemin().trimmed();
            entry.title = s->getTitre();
            entry.length = s->getDuree();
            entry.isStream = true;
            entries.append(entry);
            continue;
        }
        const QString filePath = VFI.getPathFromVUrl(s->getChemin());
        if (filePath.isEmpty()) {
            continue;
        }
        PlsEntry entry;
        entry.filePath = filePath;
        entry.title = s->getTitre();
        entry.length = s->getDuree();
        entry.isStream = false;
        entries.append(entry);
    }

    const bool useFileScheme = (configService::getInstance().getPlaylistFormat() != PLAYLIST_FORMAT_UNPLAYER);

    QFile f(path);
    if( f.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {

        QTextStream stream(&f);
        stream.setCodec("UTF-8");

        stream << "[playlist]" << "\n";
        stream << "NumberOfEntries=" << entries.length() << "\n";

        int i=1;
        foreach (const PlsEntry &e, entries) {
            stream << "File" << i << "=" << ((!e.isStream && useFileScheme) ? QStringLiteral("file://") : QString()) << e.filePath << "\n";
            stream << "Title" << i << "=" << e.title << "\n";
            stream << "Length" << i << "=" << e.length << "\n";
            i++;
        }
        stream  << endl;

        f.close();

        return true;
    }

    return false;
}

/**
 * @brief Read playlist and return a title list
 * @param path full absolute path too the playlist
 * @return the list, NULL on error
 */
Pls *plsManager::unmarshaller(const QString &path ) {

    Pls *pls = new Pls();
    pls->setPath(path);
    pls->setType(Pls::PlsType::PLS);
    pls->setNom(VFI.getName(path));

    QHash<QString,QString> hash;

    QFile f(path);
    if( f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&f);
        in.setCodec("UTF-8");
        while(!in.atEnd()) {
            const QString line = in.readLine();
            const int eqIdx = line.indexOf('=');
            if(eqIdx <= 0) {
                continue;
            }
            hash.insert(line.left(eqIdx).trimmed().toLower(), line.mid(eqIdx + 1).trimmed());
        }

        f.close();
    }

    const QDir playlistDir = QFileInfo(path).absoluteDir();
    const quint64 declaredCount = hash.value(QStringLiteral("numberofentries")).toULongLong();

    // parse entries, automatically detecting file:///... (Jolla Media player) and /... (Unplayer) formats
    quint64 nb=1; // failsafe à 10000

    while( (hash.contains(QString("file"+QString::number(nb))) || nb <= declaredCount) && nb < 10000 ) {
        const QString fileKey = QString("file" + QString::number(nb));
        if(!hash.contains(fileKey)) {
            nb++;
            continue;
        }
        try {
            QString p = hash.value(fileKey).trimmed();
            if(WebRadioUtil::isStreamUrl(p)) {
                const QString titleHint = hash.value(QString("title" + QString::number(nb))).trimmed();
                if(!titleHint.isEmpty()) {
                    MetaDataService::getInstance().registerStreamHint(p, titleHint);
                }
                bool ok = false;
                const qint64 rawLen = hash.value(QString("length" + QString::number(nb))).toLongLong(&ok);
                const quint64 t = (ok && rawLen > 0) ? static_cast<quint64>(rawLen) : 0;
                pls->ajouterUrl(p, t);
            } else {
                if(p.startsWith(QStringLiteral("file://"), Qt::CaseInsensitive)) {
                    const QString candidate = p.startsWith(QStringLiteral("file://localhost/"), Qt::CaseInsensitive)
                            ? p.mid(16)
                            : p.mid(7);
                    const QString decoded = QUrl::fromEncoded(p.toUtf8()).toLocalFile();
                    if(!candidate.isEmpty() && QFileInfo::exists(candidate)) {
                        p = candidate;
                    } else if(!decoded.isEmpty() && QFileInfo::exists(decoded)) {
                        p = decoded;
                    } else if(!decoded.isEmpty() && decoded != candidate && candidate.contains('%')) {
                        p = decoded;
                    } else {
                        p = candidate;
                    }
                }
                if(!p.isEmpty() && !QDir::isAbsolutePath(p)) {
                    p = QDir::cleanPath(playlistDir.absoluteFilePath(p));
                }
                QString u = VFI.getVUrl(p);
                if((u.isEmpty() || !VFI.exist(u)) && !p.isEmpty() && QFileInfo::exists(p)) {
                    const QString canonical = QFileInfo(p).canonicalFilePath();
                    if(!canonical.isEmpty()) {
                        u = VFI.getVUrl(canonical);
                    }
                }
                if(VFI.exist(u)) {
                    bool ok = false;
                    const qint64 rawLen = hash.value(QString("length" + QString::number(nb))).toLongLong(&ok);
                    const quint64 t = (ok && rawLen > 0) ? static_cast<quint64>(rawLen) : 0;
                    pls->ajouterUrl(u, t);
                }
            }
        }catch( const std::exception & e ) {
        }
        nb++;
    }

    return pls;
}
