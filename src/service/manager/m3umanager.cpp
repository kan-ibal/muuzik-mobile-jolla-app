#include "m3umanager.h"
#include <QTextStream>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QUrl>
#include <QDebug>

#include "../config.h"
#include "../metadataservice.h"
#include "../../virtualfiles/VFservice.h"
#include "../../util/webradioutil.h"

m3uManager::m3uManager(QObject *parent) : QObject(parent) {
}

namespace {
struct M3uEntry {
    QString filePath;
    QString title;
    QString artist;
    quint64 length;
    bool isStream;
};

QString resolvePlaylistPath(const QString &rawEntry, const QDir &playlistDir) {
    QString p = rawEntry.trimmed();
    if (p.isEmpty()) {
        return QString();
    }

    if (p.startsWith(QStringLiteral("file://"), Qt::CaseInsensitive)) {
        const QString candidate = p.startsWith(QStringLiteral("file://localhost/"), Qt::CaseInsensitive)
                ? p.mid(16)
                : p.mid(7);
        const QString decoded = QUrl::fromEncoded(p.toUtf8()).toLocalFile();
        if (!candidate.isEmpty() && QFileInfo::exists(candidate)) {
            p = candidate;
        } else if (!decoded.isEmpty() && QFileInfo::exists(decoded)) {
            p = decoded;
        } else if (!decoded.isEmpty() && decoded != candidate && candidate.contains('%')) {
            p = decoded;
        } else {
            p = candidate;
        }
    }

    if (!p.isEmpty() && !QFileInfo::exists(p) && p.contains('\\')) {
        QString unixPath = p;
        unixPath.replace('\\', '/');
        if (QFileInfo::exists(unixPath) || (!QDir::isAbsolutePath(unixPath) && QFileInfo::exists(playlistDir.absoluteFilePath(unixPath)))) {
            p = unixPath;
        }
    }

    if (!p.isEmpty() && !QDir::isAbsolutePath(p)) {
        p = QDir::cleanPath(playlistDir.absoluteFilePath(p));
    }

    return p;
}
}

bool m3uManager::marshaller(const QList<MetaData *> list, const QString &path) {
    QList<M3uEntry> entries;
    foreach (MetaData *s, list) {
        if (!s) {
            continue;
        }
        if (WebRadioUtil::isStreamUrl(s->getChemin())) {
            M3uEntry entry;
            entry.filePath = s->getChemin().trimmed();
            entry.title = s->getTitre();
            entry.artist = s->getArtiste();
            entry.length = static_cast<quint64>(s->getDuree() > 0 ? s->getDuree() : 0);
            entry.isStream = true;
            entries.append(entry);
            continue;
        }
        const QString filePath = VFI.getPathFromVUrl(s->getChemin());
        if (filePath.isEmpty()) {
            continue;
        }
        M3uEntry entry;
        entry.filePath = filePath;
        entry.title = s->getTitre();
        entry.artist = s->getArtiste();
        entry.length = static_cast<quint64>(s->getDuree() > 0 ? s->getDuree() : 0);
        entry.isStream = false;
        entries.append(entry);
    }

    const bool useFileScheme = (configService::getInstance().getPlaylistFormat() != PLAYLIST_FORMAT_UNPLAYER);

    QFile f(path);
    if (f.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        QTextStream stream(&f);
        stream.setCodec("UTF-8");

        stream << "#EXTM3U" << "\n";
        foreach (const M3uEntry &e, entries) {
            QString displayTitle = e.title;
            if (!e.artist.isEmpty() && !e.title.isEmpty()) {
                displayTitle = e.artist + QStringLiteral(" - ") + e.title;
            } else if (displayTitle.isEmpty()) {
                displayTitle = QFileInfo(e.filePath).completeBaseName();
            }
            stream << "#EXTINF:" << e.length << "," << displayTitle << "\n";
            stream << ((!e.isStream && useFileScheme) ? QStringLiteral("file://") : QString()) << e.filePath << "\n";
        }
        stream.flush();
        f.close();
        return true;
    }

    return false;
}

Pls *m3uManager::unmarshaller(const QString &path) {
    Pls *pls = new Pls();
    pls->setPath(path);
    pls->setType(Pls::PlsType::M3U);
    pls->setNom(VFI.getName(path));

    QFile f(path);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return pls;
    }

    const QDir playlistDir = QFileInfo(path).absoluteDir();
    QTextStream in(&f);
    in.setCodec("UTF-8");

    qint64 pendingDuration = -1;
    QString pendingTitle;
    quint64 entryCount = 0;

    while (!in.atEnd() && entryCount < 10000) {
        QString line = in.readLine();
        if (!line.isEmpty() && line.at(0) == QChar(0xFEFF)) {
            line.remove(0, 1);
        }
        line = line.trimmed();

        if (line.isEmpty()) {
            continue;
        }

        if (line.startsWith('#')) {
            if (line.startsWith(QStringLiteral("#EXTINF:"), Qt::CaseInsensitive)) {
                const QString rest = line.mid(8).trimmed();
                const int commaIdx = rest.indexOf(',');
                QString durToken = (commaIdx >= 0 ? rest.left(commaIdx) : rest).trimmed();
                pendingTitle = (commaIdx >= 0) ? rest.mid(commaIdx + 1).trimmed() : QString();
                const int spaceIdx = durToken.indexOf(' ');
                if (spaceIdx > 0) {
                    durToken = durToken.left(spaceIdx);
                }
                bool ok = false;
                const double parsedSeconds = durToken.toDouble(&ok);
                pendingDuration = (ok && parsedSeconds > 0) ? static_cast<qint64>(parsedSeconds) : 0;
            }
            continue;
        }

        if (WebRadioUtil::isStreamUrl(line)) {
            if (!pendingTitle.isEmpty()) {
                MetaDataService::getInstance().registerStreamHint(line, pendingTitle);
            }
            const quint64 t = (pendingDuration > 0) ? static_cast<quint64>(pendingDuration) : 0;
            pls->ajouterUrl(line, t);
            entryCount++;
        } else {
            const QString p = resolvePlaylistPath(line, playlistDir);
            QString u = VFI.getVUrl(p);
            if ((u.isEmpty() || !VFI.exist(u)) && !p.isEmpty() && QFileInfo::exists(p)) {
                const QString canonical = QFileInfo(p).canonicalFilePath();
                if (!canonical.isEmpty()) {
                    u = VFI.getVUrl(canonical);
                }
            }

            if (VFI.exist(u)) {
                const quint64 t = (pendingDuration > 0) ? static_cast<quint64>(pendingDuration) : 0;
                pls->ajouterUrl(u, t);
                entryCount++;
            }
        }

        pendingDuration = -1;
        pendingTitle.clear();
    }

    f.close();
    return pls;
}
