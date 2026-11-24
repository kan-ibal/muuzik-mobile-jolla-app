#ifndef M3UMANAGER_H
#define M3UMANAGER_H

#include <QObject>
#include <QList>
#include <QString>

#include "../../bean/MetaData.h"
#include "../../bean/pls.h"

// lit / ecrit des playlists au format m3u / m3u8
class m3uManager : public QObject {
    Q_OBJECT

public:
    explicit m3uManager(QObject *parent = 0);

    /**
     * @brief Write data into 'disk' in M3U format
     * @param list list of titles
     * @param path full absolute path to the playlist, if exists, override
     * @return true if list written
     */
    static bool marshaller(const QList<MetaData *> list, const QString &path);

    /**
     * @brief Read M3U/M3U8 playlist and return a Pls object
     * @param source full absolute path to the playlist
     * @return the Pls object; list might be empty
     */
    static Pls *unmarshaller(const QString &source);
};

#endif // M3UMANAGER_H
