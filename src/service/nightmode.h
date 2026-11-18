#ifndef NIGHTMODE_H
#define NIGHTMODE_H

#include <QObject>
#include <QTimer>
#include <qqml.h>
#include <QQmlContext>

#include "player.h"
#include "pulse/volumeProxy.h"

#include <nemonotifications-qt5/notification.h>

#define NIGHTMODESERVICE_QML_NAME "nightModeService"

using namespace std;

class nightModeService : public QObject {
    friend class Notification;

    Q_OBJECT
    Q_PROPERTY(bool active READ getActive NOTIFY activeChanged )
    Q_PROPERTY(bool possible READ getPossible CONSTANT )
    Q_PROPERTY(quint64 endTime READ getEndTime WRITE setEndTime NOTIFY endTimeChanged )
    Q_PROPERTY(bool reductionVolume READ getReductionVolume WRITE setReductionVolume NOTIFY reductionVolumeChanged )
    Q_PROPERTY(quint32 finalVolume READ getFinalVolume WRITE setFinalVolume NOTIFY finalVolumeChanged )

public:
    static nightModeService& getInstance();
    static void init(QQmlContext *context, QString name = NIGHTMODESERVICE_QML_NAME);

    // true si mode nuit possible (connection dbus active...)
    bool getPossible() const {return volume->isOk();}

    // true si le mode nuit est en cours
    bool getActive() const {return active;}

    void setEndTime(quint64 const v);
    quint64 getEndTime() const {return endTime;}

    void setReductionVolume(bool const v);
    bool getReductionVolume() const {return reductionVolume;}

    Q_INVOKABLE void setFinalVolume(quint32 const v);
    quint32 getFinalVolume() const {return finalVolume;}

    /**
      *@brief active le mode nuit (si possible)
      */
    Q_INVOKABLE void start();

    /**
      * @brief arrete le mode nuit
      */
    Q_INVOKABLE void stop();

    /**
      * @brief temps restant avant arret, en seconde
      * @return le temps en secondes
      */
    Q_INVOKABLE quint32 remainingTime();

private:
    explicit nightModeService(QObject *parent = 0);
    ~nightModeService();

    playerService *lecteur;
    VolumeProxy *volume;

    bool active; // true si le mode nuit est en cours

    quint64 endTime;

    bool reductionVolume;
    quint32 finalVolume;
    // doit garder un cache du volume actuel pour eviter les boucles initvolume/volumeChanged
    quint32 volumeActuel;

    QTimer tduree;
    QTimer tvolume;

    quint64 backduree;

    void initTimerVolume();

signals:
    void activeChanged();
    void endTimeChanged();
    void reductionVolumeChanged();
    void finalVolumeChanged();

private slots:
    void changementPlaylist();
    void changementEtat(bool s);

    void finTemps();
    void finVolume();

    void changementCurrentStep(quint32 v);
};

#endif // NIGHTMODE_H
