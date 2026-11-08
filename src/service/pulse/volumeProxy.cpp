#include "volumeProxy.h"
#include <QDebug>
#include <QtQml>
#include <QMetaType>
#include <QQmlEngine>

/**
 * @brief controleVolume::controleVolume
 * @param parent
 */
VolumeProxy::VolumeProxy(QObject *parent) : QObject(parent),
    pulse(0), iface(0), core(0),
    ok(false),currentStep(0),stepCount(0),highVolumeStep(0){

    QDBusInterface ipulse( PULSE_SERVICE,
                           PULSE_OBJ,
                           FREEDESKTOP_PROPERTIES,
                           QDBusConnection::sessionBus(), 0 );

    QDBusMessage r1 = ipulse.call( "Get", PULSE_SERVICE_IF, "Address");
    if(r1.type() == QDBusMessage::ErrorMessage) {
        qDebug() << "get pulse addr : " << r1.errorMessage();
        return;
    }

    QString addr = r1.arguments()[0].value<QDBusVariant>().variant().toString();
    qDebug() << "Pulse addr : " << addr;
    pulse = new QDBusConnection(QDBusConnection::connectToPeer(addr, PULSE_PEER));
    iface = new QDBusInterface( "",
                                MEEGO_VOLUME_OBJ,
                                FREEDESKTOP_PROPERTIES,
                                *pulse, 0 );
    if( iface->isValid()==false)  {
        qDebug() << "can't create pulse interface";
        delete iface;
        iface = 0;
        delete pulse;
        pulse = 0;
        return;
    }

    core = new QDBusInterface( "",
                               PULSE_CORE_PATH,
                               PULSE_CORE_IF,
                               *pulse, 0 );
    if( core->isValid()==false)  {
        qDebug() << "can't create pulse core interface";
        delete core;
        core = 0;
        delete iface;
        iface = 0;
        delete pulse;
        pulse = 0;
        return;
    }

    // register signal listening in core and proxy
    if(listenForSignalImpl(MEEGO_VOLUME_StepsUpdated)) {
    pulse->connect("",
                   MEEGO_VOLUME_OBJ,MEEGO_VOLUME_SERVICE_IF,MEEGO_VOLUME_StepsUpdated,
                   this,SLOT(StepsUpdatedImpl(quint32,quint32))
                   );
    }
    if(listenForSignalImpl(MEEGO_VOLUME_HighVolumeStep)) {
    pulse->connect("",
                   MEEGO_VOLUME_OBJ,MEEGO_VOLUME_SERVICE_IF,MEEGO_VOLUME_HighVolumeStep,
                   this,SLOT(NotifyHighVolumeImpl(quint32))
                   );
    }

    // initial values
    getStepCountImpl();
    getCurrentStepImpl();
    getHighVolumeStepImpl();

    ok=true;
/*
    qDebug() << "---------------------------------";


    QDBusInterface iii( "",
                            "/com/meego/mainvolume2",
                            "org.freedesktop.DBus.Properties",
                            *pulse, 0 );
      qDebug() << "interface valide ? " <<  iii.isValid();
      if( iii.isValid()==false)  {
          qDebug() << "---------------------------------";
          return;
      }
      qDebug() << "lecture du volume par interface free desk";
      QDBusMessage riii = iii.call( "Get","com.Meego.MainVolume2", "CurrentStep");
      if(riii.type() != QDBusMessage::ErrorMessage) {
          qDebug() << "Mainteant lecture volume " <<
                      riii.arguments()[0].value<QDBusVariant>().variant().value<quint32>();
      } else {
          qDebug() << "ben non";
      }




    QDBusInterface i2( "",
                       "/com/meego/mainvolume2",
                       "com.Meego.MainVolume2",
                       *pulse, 0 );
    qDebug() << "i2 valide ? " <<  i2.isValid();
    if( i2.isValid()==false)  {
        qDebug() << "interface PAS ok";
        qDebug() << "---------------------------------";
        return;
    }
    qDebug() << "lecture du volume par i2";
    QDBusMessage r2 = i2.call("CurrentStep");
    if(r2.type() != QDBusMessage::ErrorMessage) {
        qDebug() << "Mainteant lecture volume " <<
                    r2.arguments()[0].value<QDBusVariant>().variant().value<quint32>();
    } else {
        qDebug() << "ben non";
    }


    qDebug() << "---------------------------------";
*/

}

/**
 * @brief VolumeProxy::~VolumeProxy
 */
VolumeProxy::~VolumeProxy() {
    if(pulse) {
        pulse->disconnect("",
                       MEEGO_VOLUME_OBJ,MEEGO_VOLUME_SERVICE_IF,MEEGO_VOLUME_StepsUpdated,
                       this,SLOT(StepsUpdatedImpl(quint32,quint32))
                       );
        pulse->disconnect("",
                       MEEGO_VOLUME_OBJ,MEEGO_VOLUME_SERVICE_IF,MEEGO_VOLUME_HighVolumeStep,
                       this,SLOT(NotifyHighVolumeImpl(quint32))
                       );
    }
    stopListeningForSignalImpl(MEEGO_VOLUME_StepsUpdated);
    stopListeningForSignalImpl(MEEGO_VOLUME_HighVolumeStep);

    delete iface;
    iface = 0;
    delete core;
    core = 0;
    delete pulse;
    pulse = 0;
}

/**
  * @brief VolumeProxy::getInstance
  * @return
  */
VolumeProxy& VolumeProxy::getInstance() {
    static VolumeProxy instance;
    return instance;
}

/**
 * @brief register the service into the context so it can be used in qml
 * @param context
 * @param name : register service on this name, default to VOLUMESERVICE_QML_NAME
 */
void VolumeProxy::init(QQmlContext *context, QString name) {
    context->setContextProperty(name,&VolumeProxy::getInstance());
}

/**
 * @brief VolumeProxy::setCurrentStep
 * @param v
 */
void VolumeProxy::setCurrentStep(quint32 v) {
    if(ok==false) {
        return;
    }

    v = v > stepCount ? stepCount : v;
    if(currentStep==v) {
        return;
    }

    QDBusVariant dv(v);
    QDBusMessage r3 = iface->call( "Set", MEEGO_VOLUME_SERVICE_IF,MEEGO_VOLUME_CurrentStep,
                                   QVariant::fromValue(dv) );

    if(r3.type() == QDBusMessage::ErrorMessage) {
        qDebug() << "SET CurrentStep : " << r3.errorMessage();
        return;
    }

    currentStep=v;
    emit currentStepChanged(currentStep);
}

/**
 * @brief VolumeProxy::getCurrentStepImpl
 * @return
 */
quint32 VolumeProxy::getCurrentStepImpl() {

    QDBusMessage r1 = iface->call( "Get",MEEGO_VOLUME_SERVICE_IF,MEEGO_VOLUME_CurrentStep );
    if(r1.type() == QDBusMessage::ErrorMessage) {
        qDebug() << "GET CurrentStep : " << r1.errorMessage();
        return 0;
    }

    currentStep = r1.arguments()[0].value<QDBusVariant>().variant().value<quint32>();
    emit currentStepChanged(currentStep);
    return currentStep;
}

/**
 * @brief VolumeProxy::getStepCountImpl
 * @return
 */
quint32 VolumeProxy::getStepCountImpl() {

    QDBusMessage r1 = iface->call( "Get",MEEGO_VOLUME_SERVICE_IF, MEEGO_VOLUME_StepCount);
    if(r1.type() == QDBusMessage::ErrorMessage) {
        qDebug() << "GET StepCount : " << r1.errorMessage();
        return 0;
    }

    stepCount = r1.arguments()[0].value<QDBusVariant>().variant().value<quint32>();
    emit stepCountChanged(stepCount);
    return stepCount;
}

/**
 * @brief VolumeProxy::getHighVolumeStepImpl
 * @return
 */
quint32 VolumeProxy::getHighVolumeStepImpl() {
    QDBusMessage r1 = iface->call( "Get",MEEGO_VOLUME_SERVICE_IF, MEEGO_VOLUME_HighVolumeStep);
    if(r1.type() == QDBusMessage::ErrorMessage) {
        qDebug() << "GET HighVolumeStep : " << r1.errorMessage();
        return 0;
    }

    highVolumeStep = r1.arguments()[0].value<QDBusVariant>().variant().value<quint32>();
    emit highVolumeStepChanged(highVolumeStep);
    return highVolumeStep;
}

/**
 * @brief VolumeProxy::StepsUpdatedImpl
 * @param step
 * @param current
 */
void VolumeProxy::StepsUpdatedImpl(quint32 step, quint32 current) {
  //  qDebug() <<"proxy StepsUpdatedImpl" << step << " " << current;

    if( currentStep != current) {
        currentStep=current;
        emit currentStepChanged(currentStep);
    }

    if(stepCount != step) {
        stepCount=step;
        emit stepCountChanged(stepCount);
    }
}

/**
 * @brief VolumeProxy::NotifyHighVolumeImpl
 * @param step
 */
void VolumeProxy::NotifyHighVolumeImpl(quint32 step) {
    if( highVolumeStep != step) {
        highVolumeStep = step;
        emit highVolumeStepChanged(highVolumeStep);
    }
}

/**
 * @brief VolumeProxy::listenForSignalImpl
 * @return
 */
bool VolumeProxy::listenForSignalImpl(const QString &name) {
    if( !core) {
        return false;
    }

    QList<QDBusObjectPath> obj_l;
    QString str(MEEGO_VOLUME_SERVICE_IF);
    str.append( "." + name);
    QDBusMessage r2 = core->call( PULSE_CORE_ListenForSignal,
                                  str,
                                  QVariant::fromValue(obj_l)
                                  );
    if(r2.type() == QDBusMessage::ErrorMessage ) {
        qDebug() << "ListenForSignal : " << r2.errorMessage();
        return false;
    }

    return true;
}

/**
 * @brief VolumeProxy::unlistenForSignalImpl
 * @param name
 * @return
 */
bool VolumeProxy::stopListeningForSignalImpl(const QString &name) {
    if( !core) {
        return false;
    }

    QString str( MEEGO_VOLUME_SERVICE_IF);
    str.append( "." + name);
    core->call( PULSE_CORE_StopListeningForSignal,str );
    return true;
}
