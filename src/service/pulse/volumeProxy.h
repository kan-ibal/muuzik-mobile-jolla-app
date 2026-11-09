#ifndef VOLUMEPROXY_H
#define VOLUMEPROXY_H

#include <QObject>
#include <QDBusConnection>
#include <QDBusInterface>
#include <QQmlContext>

#define PULSE_OBJ                   "/org/pulseaudio/server_lookup1"
#define PULSE_SERVICE               "org.PulseAudio1"
#define PULSE_SERVICE_IF            "org.PulseAudio.ServerLookup1"
#define PULSE_PEER                  "org.PulseAudio1"

#define PULSE_CORE_PATH                     "/org/pulseaudio/core1"
#define PULSE_CORE_IF                       "org.PulseAudio.Core1"
#define PULSE_CORE_ListenForSignal          "ListenForSignal"
#define PULSE_CORE_StopListeningForSignal   "StopListeningForSignal"

#define MEEGO_VOLUME_OBJ            "/com/meego/mainvolume2"
#define MEEGO_VOLUME_SERVICE_IF     "com.Meego.MainVolume2"
#define MEEGO_VOLUME_StepsUpdated   "StepsUpdated"
#define MEEGO_VOLUME_CurrentStep    "CurrentStep"
#define MEEGO_VOLUME_StepCount      "StepCount"
#define MEEGO_VOLUME_HighVolumeStep "HighVolumeStep"

#ifndef FREEDESKTOP_PROPERTIES
#define FREEDESKTOP_PROPERTIES      "org.freedesktop.DBus.Properties"
#endif

#define VOLUMESERVICE_QML_NAME "VolumeService"


class VolumeProxy : public QObject {
    friend class QDBusInterface;
    friend class QDBusConnection;

    Q_OBJECT
    Q_PROPERTY(bool ok READ isOk CONSTANT )
    Q_PROPERTY(quint32 currentStep READ getCurrentStep WRITE setCurrentStep NOTIFY currentStepChanged )
    Q_PROPERTY(quint32 stepCount READ getStepCount  NOTIFY stepCountChanged )
    Q_PROPERTY(quint32 highVolumeStep READ getHighVolumeStep  NOTIFY highVolumeStepChanged )

public:
    static VolumeProxy& getInstance();

    /**
     * @brief register the service into the context so it can be used in qml
     * @param context
     * @param name : register service on this name, default to VOLUMESERVICE_QML_NAME
     */
    static void init(QQmlContext *context, QString name=VOLUMESERVICE_QML_NAME);

    /**
     * @brief true if service is available (connected to com.Meego.MEEGO_VOLUME2)
     * @return
     */
    Q_INVOKABLE bool isOk() { return ok;}

    /**
     * @brief getCurrentStep
     * @return
     */
    Q_INVOKABLE quint32 getCurrentStep() { return currentStep;}
    Q_INVOKABLE void setCurrentStep(quint32 v);

    /**
     * @brief getStepCount
     * @return
     */
    Q_INVOKABLE quint32 getStepCount() { return stepCount;}

    /**
     * @brief getHighVolumeStep
     * @return
     */
    Q_INVOKABLE quint32 getHighVolumeStep() { return highVolumeStep;}

    void cleanup();

private :
    explicit VolumeProxy(QObject *parent = 0);
    ~VolumeProxy();

    QDBusConnection *pulse;
    QDBusInterface *iface;
    QDBusInterface *core;

    bool ok;
    quint32 currentStep;
    quint32 stepCount;
    quint32 highVolumeStep;

    quint32 getCurrentStepImpl();
    quint32 getStepCountImpl();
    quint32 getHighVolumeStepImpl();

    bool listenForSignalImpl(const QString &name);
    bool stopListeningForSignalImpl(const QString &name);

signals:
    void currentStepChanged(quint32 v);
    void stepCountChanged(quint32 v);
    void highVolumeStepChanged(quint32 v);

public slots:
    // declared public for only for dbus ; call setCurrentStep instead
    void StepsUpdatedImpl(quint32 step, quint32 current);
    void NotifyHighVolumeImpl(quint32 step);
};

#endif // VOLUMEPROXY_H
