#ifndef MPRISLECTEUR_H
#define MPRISLECTEUR_H

#include <QObject>
#include <QtDBus>
#include <QDBusAbstractAdaptor>
#include "mpris.h"
#include "../lecteur.h"

using namespace std;

class lecteurService;
class MprisLecteur : public QDBusAbstractAdaptor {
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface",MPRIS_PLAYER_ID)

    //org.mpris.MediaPlayer2.Player
    Q_PROPERTY(QString PlaybackStatus READ getPlaybackStatus  )
    Q_PROPERTY(QString LoopStatus READ getLoopStatus WRITE setLoopStatus  )
    Q_PROPERTY(double Rate READ getRate WRITE setRate )
    Q_PROPERTY(bool Shuffle READ getShuffle WRITE setShuffle  )
    Q_PROPERTY(QVariantMap Metadata READ getMetadata  )
    Q_PROPERTY(double Volume READ getVolume WRITE setVolume  )
    Q_PROPERTY(qlonglong Position READ getPosition  )
    Q_PROPERTY(double MinimumRate READ getMinimumRate  )
    Q_PROPERTY(double MaximumRate READ getMaximumRate  )
    Q_PROPERTY(bool CanGoNext READ getCanGoNext  )
    Q_PROPERTY(bool CanGoPrevious READ getCanGoPrevious  )
    Q_PROPERTY(bool CanPlay READ getCanPlay  )
    Q_PROPERTY(bool CanPause READ getCanPlay  )
    Q_PROPERTY(bool CanSeek READ getCanPlay  )
    //   Q_PROPERTY(bool CanPause READ getCanPause  )
    //    Q_PROPERTY(bool CanSeek READ getCanSeek  )
    Q_PROPERTY(bool CanControl READ getCanControl  )

public:
    MprisLecteur(lecteurService *s);

    //org.mpris.MediaPlayer2.Player
    QString getPlaybackStatus();
    QString getLoopStatus();
    void  setLoopStatus(QString v);
    double getRate() {return 1.0;}
    void setRate(double r){}
    bool getShuffle();
    void setShuffle(bool v);
    QVariantMap getMetadata();
    double getVolume();
    void setVolume(double v);
    qlonglong  getPosition();
    double getMinimumRate() {return 1.0;}
    double getMaximumRate() {return 1.0;}

    bool getCanGoNext();
    bool getCanGoPrevious();
    bool getCanPlay();
    //   bool getCanPause();
    //   bool getCanSeek();
    bool getCanControl(){return true;}

    void changementDuree(qint64 durationMs);

private :
    QVariantMap metadata;
    QString currentTrackChemin;
    lecteurService *service;

    quint64 taille; // cache taiile de la playlist

    void buildMetadata(MetaData *t);
    void sendDBusProperty(const QVariantMap value);

signals:
    //org.mpris.MediaPlayer2.Player
    void Seeked(qlonglong Position);

public slots :

    //org.mpris.MediaPlayer2.Player
    Q_INVOKABLE void  Next();
    Q_INVOKABLE void  Previous();
    Q_INVOKABLE void  Pause();
    Q_INVOKABLE  void  PlayPause();
    Q_INVOKABLE void  Stop();
    Q_INVOKABLE  void  Play();
    Q_INVOKABLE  void Seek(qlonglong offset);
    Q_INVOKABLE  void SetPosition(const QDBusObjectPath &TrackId, qlonglong Position);


    void  OpenUri(QString uri);

protected slots:
    void changementTitre(MetaData *t);
    void changementPlaylist();
    void changementEtat(bool s);
    void changementMode();
    void changementPochette(const QString &vurl, const QString &artPath);

private slots:

};

#endif // MPRISLECTEUR_H
