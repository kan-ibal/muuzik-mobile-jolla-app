#include "nightmode.h"
#include "mpris/mpris.h"
#include <QDebug>
#include "../app.h"

#include "pulse/volumeProxy.h"
#include "player.h"

#include "../vi18n/vi18n.h"

#include <nemonotifications-qt5/notification.h>

/**
 * @brief nightModeService::nightModeService
 * @param parent
 */
nightModeService::nightModeService(QObject *parent) : QObject(parent),
    active(false),
    endTime(3600),reductionVolume(true),finalVolume(0),volumeActuel(0)
{

    lecteur = &playerService::getInstance();
    connect(lecteur,SIGNAL(listChanged()),this, SLOT(changementPlaylist()));
    connect(lecteur,SIGNAL(statusChanged(bool)),this, SLOT(changementEtat(bool)));

    volume = &VolumeProxy::getInstance();
    connect(volume,SIGNAL(currentStepChanged(quint32)),this, SLOT(changementCurrentStep(quint32)));

    finalVolume = volume->getCurrentStep();
    volumeActuel = finalVolume;

    connect(&tduree, SIGNAL(timeout()), this, SLOT(finTemps()));
    connect(&tvolume, SIGNAL(timeout()), this, SLOT(finVolume()));
}

/**
 * @brief nightModeService::~nightModeService
 */
nightModeService::~nightModeService() {

}

/**
 * @brief nightModeService::getInstance
 * @return
 */
nightModeService& nightModeService::getInstance() {
    static nightModeService instance;
    return instance;
}

/**
 * @brief nightModeService::init
 * @param context
 * @param name
 */
void nightModeService::init(QQmlContext *context,QString name) {
    context->setContextProperty(name, &nightModeService::getInstance());
}

/**
 * @brief nightModeService::setEndTime
 * @param v
 */
void nightModeService::setEndTime(quint64 const v) {
    endTime=v;
    backduree=endTime *1000;
    emit endTimeChanged();
}

/**
 * @brief nightModeService::setReductionVolume
 * @param v
 */
void nightModeService::setReductionVolume(bool const v) {

    reductionVolume=v;
    emit reductionVolumeChanged();
}

/**
 * @brief nightModeService::setFinalVolume
 * @param v
 */
void nightModeService::setFinalVolume(const quint32 v) {

    if(v>=volume->getCurrentStep()) {
        return;
    }

    finalVolume=v;
    emit finalVolumeChanged();
}

/**
  *@brief active le mode nuit (si possible)
  */
void nightModeService::start() {

    if(active) {
        return;
    }

    active=true;
    emit activeChanged();

    if(lecteur->getStatus()==false) {
        // le callback s'occupera de lancer les timers
        lecteur->play();
    } else {
        tduree.setInterval(backduree);
        tduree.start();

        initTimerVolume();
    }

    Notification n;
    n.setPreviewBody(VI18N.get( "nuit.demarrer" ).arg(App::tempsHumaine(endTime)));
    n.setHintValue("x-nemo-icon", "image://theme/icon-s-timer");
    n.publish();
}

/**
  * @brief arrete le mode nuit
  */
void nightModeService::stop() {

    if(active==false) {
        return;
    }

    tduree.stop();
    tvolume.stop();
    backduree = endTime*1000;
    active=false;
    emit activeChanged();

    Notification n;
    n.setPreviewBody(VI18N.get( "nuit.arrete" ));
    n.setHintValue("x-nemo-icon", "image://theme/icon-s-message");
    n.publish();

    lecteur->stop();
}

/**
  * @brief temps restant avant arret, en seconde
  * @return le temps en secondes
  */
quint32 nightModeService::remainingTime() {
    return (tduree.remainingTime() > 0 ? tduree.remainingTime() : backduree) / 1000;
}

/**
 * @brief nightModeService::changementPlaylist
 */
void nightModeService::changementPlaylist() {

    quint64 t = lecteur->getDureeTotale();
    if(t==0) {
        // si en cours : arrete
        if(active) {
            stop();
        }

        endTime=0;
        emit endTimeChanged();
        return;
    }

    // @FIX ? on est à la seconde pres ?
    if( endTime > 0 && endTime <= t) {
        // a deja defini un temps utilisateur, skip
        return;
    }

    // def le temp ou clamp
    endTime = t;
    emit endTimeChanged();

    if(active==false) {
        return;
    }

    if(lecteur->getStatus()) {
        // en lecture, verif si pas trop long
        quint64 v = tduree.remainingTime();

        if(v> endTime*1000) {
            // liste a été réduite...
            tduree.stop();
            tduree.setInterval(endTime*1000);
            tduree.start();
            initTimerVolume();
        }

        return;
    }

    // en pause
    // clamp la dure de timeout
    backduree = endTime*1000 > backduree ? backduree : endTime*1000;
}

/**
 * @brief nightModeService::changementEtat
 * @param s
 */
void nightModeService::changementEtat(bool s) {

    if(active==false) {
        return;
    }

    // passe en lecture
    if(s) {
        tduree.setInterval(backduree);
        tduree.start();

        initTimerVolume();
        return;
    }

    // mettre en pause : arreter les timers
    backduree = tduree.remainingTime();
    tduree.stop();
    tvolume.stop();
}

/**
 * @brief signal du timer de temps
 */
void nightModeService::finTemps() {

    stop();
}

/**
 * @brief signal du timer de volume
 */
void nightModeService::finVolume() {

    quint32 cv = volume->getCurrentStep();

    if( cv<= finalVolume) {
        // fin, on ne touche plus au volume
        // on laisse le tduree s'occuper de stopper la lecture : eviter les croisements start/stop
        return;
    }

    if(tduree.isActive()==false) {
        return;
    }

    volume->setCurrentStep(cv-1);

    // NOTA : pas la peine, changementCurrentStep s'en charge
    // initTimerVolume();
}

/**
 * @brief nightModeService::initTimerVolume
 */
void nightModeService::initTimerVolume() {
    if(active==false) {
        return;
    }

    if(reductionVolume==false) {
        return;
    }

    tvolume.stop();

    if(tduree.isActive()==false) {
        return;
    }

    qint32 d =  volume->getCurrentStep() - finalVolume;
    if(d<0) {
        return;
    }

    quint64 v = tduree.remainingTime();
    tvolume.setInterval(v/(d+1));
    tvolume.start();
}

/**
 * @brief nightModeService::changementCurrentStep
 * @param v
 */
void nightModeService::changementCurrentStep(quint32 v) {
    if(active==false) {
        return;
    }

    initTimerVolume();
}
