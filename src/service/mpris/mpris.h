#ifndef MPRIS_H
#define MPRIS_H

#include <QObject>
#include <QtDBus>
#include <QDBusAbstractAdaptor>

using namespace std;

#define MPRIS_OBJ "/org/mpris/MediaPlayer2"
#define MPRIS_SERVICE_ID "org.mpris.MediaPlayer2.harbour-muuzik"
#define MPRIS_ID "org.mpris.MediaPlayer2"
#define MPRIS_PLAYER_ID "org.mpris.MediaPlayer2.Player"
#define FREEDESKTOP_PROPERTIES "org.freedesktop.DBus.Properties"

class Mpris : public QDBusAbstractAdaptor {
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface",MPRIS_ID)

    //org.mpris.MediaPlayer2
    Q_PROPERTY(bool CanQuit READ getCanQuit  )
    Q_PROPERTY(bool Fullscreen READ getFullscreen WRITE SetFullscreen )
    Q_PROPERTY(bool CanSetFullscreen READ getCanSetFullscreen  )
    Q_PROPERTY(bool CanRaise READ getCanRaise  )
    Q_PROPERTY(bool HasTrackList READ getHasTrackList  )
    Q_PROPERTY(QString Identity READ getIdentity  )
    Q_PROPERTY(QString DesktopEntry READ getDesktopEntry  )
    Q_PROPERTY(QStringList SupportedUriSchemes READ getSupportedUriSchemes  )
    Q_PROPERTY(QStringList SupportedMimeTypes READ getSupportedMimeTypes  )

public:
    Mpris(QObject *parent=0);

    //org.mpris.MediaPlayer2
    bool getCanQuit() const {return false;}
    bool getCanRaise() const {return true;}
    bool getCanSetFullscreen() const {return false;}
    bool getFullscreen() const {return false;}
    void SetFullscreen(bool v){ Q_UNUSED(v); }
    bool getHasTrackList() const {return false;}
    QString getIdentity() const { return "Muuzik! player" ;}
    QString getDesktopEntry() const { return "harbour-muuzik" ;}
    QStringList getSupportedUriSchemes() const {return uri;}
    QStringList getSupportedMimeTypes() const {return mime;}

private :
    QStringList uri;
    QStringList mime;

public slots :
    //org.mpris.MediaPlayer2
    void Raise();
    void Quit();
};

#endif // MPRIS_H
