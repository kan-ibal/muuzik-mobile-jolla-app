#include "sdcardlocator.h"
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSet>
#include <QStorageInfo>
#include <QStringList>
#include <QTextStream>
#include <QUrl>
#include <sys/stat.h>

namespace {

bool isPseudoFilesystem(const QString &fsType) {
    const QString lower = fsType.trimmed().toLower();
    if (lower.isEmpty()) {
        return false;
    }
    static const QSet<QString> pseudoTypes = {
        QStringLiteral("tmpfs"),
        QStringLiteral("devtmpfs"),
        QStringLiteral("autofs"),
        QStringLiteral("proc"),
        QStringLiteral("sysfs"),
        QStringLiteral("cgroup"),
        QStringLiteral("cgroup2"),
        QStringLiteral("pstore"),
        QStringLiteral("securityfs"),
        QStringLiteral("debugfs"),
        QStringLiteral("tracefs"),
        QStringLiteral("overlay"),
        QStringLiteral("squashfs"),
        QStringLiteral("ramfs"),
        QStringLiteral("rootfs"),
        QStringLiteral("devpts"),
        QStringLiteral("mqueue"),
        QStringLiteral("hugetlbfs"),
        QStringLiteral("rpc_pipefs"),
        QStringLiteral("configfs"),
        QStringLiteral("fusectl"),
        QStringLiteral("bpf")
    };
    return pseudoTypes.contains(lower);
}

bool isSdcardMountPoint(const QString &rawPath) {
    const QString clean = QDir::cleanPath(rawPath);
    if (clean.startsWith(QLatin1String("/media/sdcard/"))) {
        return !clean.mid(14).isEmpty();
    }
    if (clean.startsWith(QLatin1String("/run/media/"))) {
        const QStringList parts = clean.mid(11).split(QLatin1Char('/'), QString::SkipEmptyParts);
        return parts.length() >= 2;
    }
    if (clean.startsWith(QLatin1String("/opt/alien/run/media/"))) {
        const QStringList parts = clean.mid(21).split(QLatin1Char('/'), QString::SkipEmptyParts);
        return parts.length() >= 2;
    }
    return false;
}

QString unescapeMountPath(QString path) {
    path.replace(QStringLiteral("\\040"), QStringLiteral(" "));
    path.replace(QStringLiteral("\\011"), QStringLiteral("\t"));
    path.replace(QStringLiteral("\\012"), QStringLiteral("\n"));
    path.replace(QStringLiteral("\\134"), QStringLiteral("\\"));
    return path;
}

void appendUniqueCandidate(QStringList &list, QSet<QString> &seen, const QString &path) {
    const QString clean = QDir::cleanPath(path);
    if (!clean.isEmpty() && !seen.contains(clean)) {
        seen.insert(clean);
        list.append(clean);
    }
}

void appendUserNames(QStringList &users, QSet<QString> &seenUsers, const QString &baseMediaDir) {
    QDir mediaDir(baseMediaDir);
    if (!mediaDir.exists() || !mediaDir.isReadable()) {
        return;
    }
    const QStringList entries = mediaDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot | QDir::Readable, QDir::Name);
    foreach (const QString &entry, entries) {
        if (!seenUsers.contains(entry)) {
            seenUsers.insert(entry);
            users.append(entry);
        }
    }
}

bool hasDifferentMountDevice(const QString &parentPath, const QString &candidatePath) {
    struct stat parentStat;
    struct stat candidateStat;
    const QByteArray encodedParent = QFile::encodeName(parentPath);
    const QByteArray encodedCandidate = QFile::encodeName(candidatePath);
    if (::stat(encodedParent.constData(), &parentStat) != 0 ||
        ::stat(encodedCandidate.constData(), &candidateStat) != 0) {
        return false;
    }
    return parentStat.st_dev != candidateStat.st_dev;
}

} // namespace

SdcardLocator::SdcardLocator(QObject *parent) : QObject(parent),
    location(""), type("") {
    SdcardLocator::checkSdcard();
}

/**
 * @brief instance (singleton) for the service
 * @return
 */
SdcardLocator& SdcardLocator::getInstance() {
    static SdcardLocator instance;
    return instance;
}

/**
 * @brief register the service into the context so it can be used in qml
 * @param context
 * @param name : register service on this name, SdcardLocator instead if null
 */
void SdcardLocator::init(QQmlContext *context, QString name) {
    context->setContextProperty(name, &SdcardLocator::getInstance());
}

/**
 * @brief check if url is on the sdcard.
 * @param url
 * @return false if not or no sdcard
 * @since 1.4
 */
bool SdcardLocator::isOnSdcard(const QString &url) {
    if (!isPresent() || url.isEmpty()) {
        return false;
    }

    QString candidate = url.trimmed();
    if (candidate.startsWith(QStringLiteral("file://"), Qt::CaseInsensitive)) {
        candidate = QUrl(candidate).toLocalFile();
    }
    candidate = QDir::cleanPath(candidate);
    const QString cleanLocation = QDir::cleanPath(location);

    if (candidate == cleanLocation || candidate.startsWith(cleanLocation + QLatin1Char('/'))) {
        return true;
    }

    const QString canonicalCandidate = QFileInfo(candidate).canonicalFilePath();
    const QString canonicalLocation = QFileInfo(cleanLocation).canonicalFilePath();
    if (!canonicalCandidate.isEmpty() && !canonicalLocation.isEmpty()) {
        if (canonicalCandidate == canonicalLocation ||
            canonicalCandidate.startsWith(canonicalLocation + QLatin1Char('/'))) {
            return true;
        }
    }

    return false;
}

void SdcardLocator::refresh() {
    const QString prevLocation = location;
    const QString prevType = type;
    checkSdcard();
    if (prevLocation != location || prevType != type) {
        emit sdcardChanged();
    }
}

/**
 * @brief check for a sdcard in a Sailjail-safe manner without spawning external processes
 */
void SdcardLocator::checkSdcard() {
    QString foundLocation;
    QString foundType;

    // 1. Inspect mounted volumes via QStorageInfo (works in-process in QtCore)
    const QList<QStorageInfo> volumes = QStorageInfo::mountedVolumes();
    foreach (const QStorageInfo &volume, volumes) {
        if (!volume.isValid() || !volume.isReady()) {
            continue;
        }
        const QString root = QDir::cleanPath(volume.rootPath());
        const QString fsType = QString::fromLatin1(volume.fileSystemType()).trimmed();
        if (isPseudoFilesystem(fsType) || !isSdcardMountPoint(root)) {
            continue;
        }
        const QFileInfo info(root);
        if (info.exists() && info.isDir() && info.isReadable()) {
            foundLocation = root;
            foundType = fsType;
            break;
        }
    }

    // 2. Fallback: read /proc/self/mounts (catches FUSE/exFAT mounts QStorageInfo may skip)
    if (foundLocation.isEmpty()) {
        const QStringList procPaths = {
            QStringLiteral("/proc/self/mounts"),
            QStringLiteral("/proc/mounts")
        };
        foreach (const QString &procPath, procPaths) {
            QFile mountsFile(procPath);
            if (!mountsFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
                continue;
            }
            QTextStream in(&mountsFile);
            while (!in.atEnd()) {
                const QString line = in.readLine().trimmed();
                if (line.isEmpty()) {
                    continue;
                }
                const QStringList tokens = line.split(QLatin1Char(' '), QString::SkipEmptyParts);
                if (tokens.size() < 3) {
                    continue;
                }
                const QString mountPoint = QDir::cleanPath(unescapeMountPath(tokens.at(1)));
                const QString fsType = tokens.at(2).trimmed();
                if (isPseudoFilesystem(fsType) || !isSdcardMountPoint(mountPoint)) {
                    continue;
                }
                const QFileInfo info(mountPoint);
                if (info.exists() && info.isDir() && info.isReadable()) {
                    foundLocation = mountPoint;
                    foundType = fsType;
                    break;
                }
            }
            mountsFile.close();
            if (!foundLocation.isEmpty()) {
                break;
            }
        }
    }

    // 3. Fallback for Sailjail sandboxed bind-mounts (RemovableMedia whitelists /run/media and /media/sdcard
    //    even when /proc/self/mounts hides individual card sub-mounts inside the container)
    if (foundLocation.isEmpty()) {
        QStringList userCandidates;
        QSet<QString> seenUsers;
        const QStringList seedUsers = {
            QString::fromLocal8Bit(qgetenv("USER")).trimmed(),
            QDir::home().dirName().trimmed(),
            QStringLiteral("defaultuser"),
            QStringLiteral("nemo")
        };
        foreach (const QString &u, seedUsers) {
            if (!u.isEmpty() && !seenUsers.contains(u)) {
                seenUsers.insert(u);
                userCandidates.append(u);
            }
        }
        appendUserNames(userCandidates, seenUsers, QStringLiteral("/run/media"));
        appendUserNames(userCandidates, seenUsers, QStringLiteral("/opt/alien/run/media"));

        QStringList parentDirs;
        QSet<QString> seenParents;
        foreach (const QString &u, userCandidates) {
            appendUniqueCandidate(parentDirs, seenParents, QStringLiteral("/run/media/") + u);
        }
        appendUniqueCandidate(parentDirs, seenParents, QStringLiteral("/media/sdcard"));
        foreach (const QString &u, userCandidates) {
            appendUniqueCandidate(parentDirs, seenParents, QStringLiteral("/opt/alien/run/media/") + u);
        }

        foreach (const QString &parentPath, parentDirs) {
            QDir parentDir(parentPath);
            if (!parentDir.exists() || !QFileInfo(parentPath).isReadable()) {
                continue;
            }

            const QStorageInfo parentStorage(parentPath);
            const QString parentRoot = QDir::cleanPath(parentStorage.rootPath());
            const QByteArray parentDev = parentStorage.device();
            const qint64 parentTotal = parentStorage.bytesTotal();

            const QStringList entries = parentDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot | QDir::Readable, QDir::Name);
            foreach (const QString &entry, entries) {
                QString candidate = QDir::cleanPath(parentDir.absoluteFilePath(entry));
                QFileInfo info(candidate);
                if (!info.exists() || !info.isDir() || !info.isReadable()) {
                    continue;
                }
                if (info.isSymLink()) {
                    const QString target = QDir::cleanPath(info.symLinkTarget());
                    QFileInfo targetInfo(target);
                    if (!target.isEmpty() && targetInfo.exists() && targetInfo.isDir() && targetInfo.isReadable()) {
                        candidate = target;
                    }
                }

                const QStorageInfo cardStorage(candidate);
                const QString cardRoot = QDir::cleanPath(cardStorage.rootPath());
                const QString fsType = QString::fromLatin1(cardStorage.fileSystemType()).trimmed();
                const bool distinctMount = cardStorage.isValid() && cardStorage.isReady() &&
                        (cardRoot != parentRoot ||
                         cardStorage.device() != parentDev ||
                         cardStorage.bytesTotal() != parentTotal);

                if (distinctMount) {
                    if (!isPseudoFilesystem(fsType)) {
                        foundLocation = candidate;
                        foundType = fsType;
                        break;
                    }
                    continue;
                }

                if (hasDifferentMountDevice(parentPath, candidate)) {
                    // Kernel mount boundary exists even if Sailjail hid the submount from QStorageInfo
                    foundLocation = candidate;
                    foundType = isPseudoFilesystem(fsType) ? QString() : fsType;
                    break;
                }

                const bool isMediaSdcardParent = (parentPath == QLatin1String("/media/sdcard"));
                if (!isMediaSdcardParent) {
                    // udisks2 dynamically creates /run/media/<user>/<card> only when mounted.
                    // Avoid false positives only if it's an empty leftover directory directly on tmpfs/ramfs.
                    if (!isPseudoFilesystem(fsType) ||
                        !QDir(candidate).entryList(QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot | QDir::Hidden).isEmpty()) {
                        foundLocation = candidate;
                        foundType = isPseudoFilesystem(fsType) ? QString() : fsType;
                        break;
                    }
                } else if (!isPseudoFilesystem(fsType) &&
                           !QDir(candidate).entryList(QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot | QDir::Hidden).isEmpty()) {
                    foundLocation = candidate;
                    foundType = fsType;
                    break;
                }
            }

            if (!foundLocation.isEmpty()) {
                break;
            }
        }
    }

    location = foundLocation;
    type = foundType;

    qDebug() << "SdcardLocator : location=" << location << "type=" << type;
}
