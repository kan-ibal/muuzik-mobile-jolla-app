#include "config.h"
#include <qdebug.h>
#include <sailfishapp.h>
#include <QDir>
#include <QFileInfo>
#include <QtQml>
#include <QMetaType>
#include <QQmlEngine>
#include <QStandardPaths>
#include <QCoreApplication>

#include "../vi18n/vi18n.h"
#include "../sdcardlocator/sdcardlocator.h"
#include "../virtualfiles/VFservice.h"

#include <nemonotifications-qt5/notification.h>

/**
 * @brief configService::configService
 * @param parent
 */
configService::configService(QObject *parent)
    : QObject(parent)
    , settings(QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation)
               + "/" + QCoreApplication::applicationName() + ".conf",
               QSettings::NativeFormat)
    , sdcard(QString::null)
    , modeRO(false) {

    configversion=settings.value(NIVEAUCONFIG,0).toInt();;

    initialisation();

    showEmptyDir =settings.value(SHOWEMPTYDIR,SHOWEMPTYDIR_DEF).toBool();
    autoStart =settings.value(AUTOSTART, AUTOSTART_DEF).toBool();
    opacitePochette = settings.value(OPACITE_POCHETTE,OPACITE_POCHETTE_DEF).toUInt();
    voirPochette= settings.value(VOIR_POCHETTE,VOIR_POCHETTE_DEF).toBool();
    controle  =  settings.value(VOIR_CONTROLE,VOIR_CONTROLE_DEF).toBool();
    dirAlbum =  settings.value(DIRALBUM,DIRALBUM_DEF).toInt();
    dirAlbumWithSub = settings.value(DIRALBUMSUB,DIRALBUMSUB_DEF).toInt();
    dirTag = settings.value(DIR_TAG, DIR_TAG_DEF).toBool();
    marge=settings.value(MARGE, MARGE_DEF).toBool();
    wifi=settings.value(C_WIFI, C_WIFI_DEF).toBool();
    lastfm =settings.value(LASTFM_OK,false).toBool();
    paroledirect =settings.value(PAROLE_DIRECT,PAROLE_DIRECT_DEF).toBool();
    paroleexterne =settings.value(PAROLE_EXTERNE,PAROLE_EXTERNE_DEF).toBool();
    nomsdcard =settings.value(NOMSDCARD,NOMSDCARD_DEF).toString();
    pochette =settings.value(POCHETTE,POCHETTE_DEF).toBool();
    pochette_wifi =settings.value(POCHETTE_WIFI,POCHETTE_WIFI_DEF).toBool();
    pochette_feedback =settings.value(POCHETTE_FEEDBACK,POCHETTE_FEEDBACK_DEF).toBool();
    pasdecache =settings.value(PASDECACHE,PASDECACHE_DEF).toBool();
    historique =settings.value(HISTORIQUE,HISTORIQUE_DEF).toBool();
    modeHistorique = settings.value(HISTORIQUE_MODE,HISTORIQUE_MODE_DEF).toInt();
    ordreFavoris = settings.value(FAVORIS_ORDRE,FAVORIS_ORDRE_DEF).toInt();
    nbRecent = settings.value(C_RECENT_MAX_NB,C_RECENT_MAX_NB_DEF).toInt();
    playlistLocation = settings.value(PLAYLIST_LOCATION, PLAYLIST_LOCATION_DEF).toUInt();
    playlistFormat = settings.value(PLAYLIST_FORMAT, PLAYLIST_FORMAT_DEF).toUInt();

    source.append(QString(QDir::homePath()+"/Music"));
    source.append(QString(QDir::homePath()+"/android_storage/Music"));
    //   sdcard = App::detecteSdcard();
    //   initSDcard();

    if(SdcardLocator::getInstance().isPresent()) {
        sdcard = SdcardLocator::getInstance().getLocation();
        initSDcard();
        }

    setModif();
}

/**
 * @brief register the service into the context so it can be used in qml
 * @param context
 * @param name : register service on this name, default to CONFIGSERVICE_QML_NAME
 */
void configService::init(QQmlContext *context, QString name) {
    context->setContextProperty(name,&configService::getInstance());
    qmlRegisterUncreatableType<configService>("Configuration", 1, 0, "Configuration","");
    qRegisterMetaTypeStreamOperators<QList<QString> >("QList<QString>");
}

/**
 * @brief configService::getInstance
 * @return
 */
configService& configService::getInstance() {
    static configService instance;
    return instance;
}

/**
 * @brief configService::setNiveauConfigOk
 */
void configService::setNiveauConfigOk() {
    configversion=NIVEAUCONFIGATTENDU;
    settings.setValue(NIVEAUCONFIG, configversion);
    settings.sync();
}

/**
 * @brief la config a changé, met à jour la date et emision de signal
 */
void configService::setModif() {
    modif = QDateTime::currentDateTime();
}

/**
 * @brief configService::setRepertoireVide
 */
void configService::setShowEmptyDir( const bool v) {
    showEmptyDir = v;
    settings.setValue(SHOWEMPTYDIR, v);
    settings.sync();
    setModif();
    emit showEmptyDirChanged();
}

/**
 * @brief configService::setDemarrageAuto
 * @param v
 */
void configService::setAutoStart(const bool v) {
    autoStart=v;
    settings.setValue(AUTOSTART, v);
    settings.sync();
    setModif();
    emit showEmptyDirChanged();
}

/**
 * @brief configService::setOpacitePochette
 */
void configService::setOpacitePochette( const quint64 v) {
    opacitePochette=v;
    settings.setValue(OPACITE_POCHETTE, v);
    settings.sync();
    setModif();
    emit opacitePochetteChanged();
}

/**
 * @brief configService::setVoirPochette
 */
void configService::setVoirPochette( const bool v ) {
    voirPochette=v;
    settings.setValue(VOIR_POCHETTE, v);
    settings.sync();
    setModif();
    emit voirPochetteChanged();
}

/**
 * @brief configService::setrepAlbum
 */
void configService::setIsDirAlbum( const qint64 v) {
    dirAlbum=v;
    settings.setValue(DIRALBUM, v);
    settings.sync();
    setModif();
    emit dirAlbumChanged();
}

/**
 * @brief configService::setControle
 * @param v
 */
void configService::setControle( const bool v) {
    controle=v;
    settings.setValue(VOIR_CONTROLE, v);
    settings.sync();
    setModif();
    emit controleChanged();
}


/**
 * @brief configService::setrepAlbumSub
 */
void configService::setIsDirAlbumWithSub( const qint64 v) {
    dirAlbumWithSub=v;
    settings.setValue(DIRALBUMSUB, v);
    settings.sync();
    setModif();
    emit dirAlbumWithSubChanged();
}

/**
 * @brief si fait une recherche année, groupe album ou affiche le nom simple
 * @param v
 */
void configService::setIsDirTag( const bool v) {
    dirTag = v;
    settings.setValue(DIR_TAG, v);
    settings.sync();
    setModif();
    emit dirTagChanged();
}

/**
 * @brief configService::setApparence
 * @param v
 */
void configService::setApparence(const qint64 v) {
    apparence=v;
    settings.setValue(APPARENCE, v);
    settings.sync();
    setModif();
    emit apparenceChanged();
}

/**
 * @brief configService::setMarge
 * @param v
 */
void configService::setMarge(bool v) {
    marge=v;
    settings.setValue(MARGE, v);
    settings.sync();
    setModif();
    emit margeChanged();
}

/**
 * @brief configService::setWifi
 * @param v
 */
void configService::setWifi(bool v) {
    wifi=v;
    settings.setValue(C_WIFI, v);
    settings.sync();
    setModif();
    emit wifiChanged();
}

/**
 * @brief configService::grantLastfm
 */
void configService::setLastfm(bool v) {
    lastfm=v;
    settings.setValue(LASTFM_OK, v);
    settings.sync();
    setModif();
    emit lastfmChanged();
}

/**
 * @brief configService::setParoleDirect
 * @param v
 */
void configService::setParoleDirect(bool v) {
    paroledirect=v;
    settings.setValue(PAROLE_DIRECT, v);
    settings.sync();
    setModif();
    emit paroleChanged();
}

/**
 * @brief configService::setParoleExterne
 * @param v
 */
void configService::setParoleExterne(bool v) {
    paroleexterne=v;
    settings.setValue(PAROLE_EXTERNE, v);
    settings.sync();
    setModif();
    emit paroleExterneChanged();
}

/**
 * @brief configService::setPasDecache
 * @param v
 */
void configService::setPasDecache(bool v) {
    pasdecache=modeRO ? true :v;
    settings.setValue(PASDECACHE, pasdecache);
    settings.sync();
    setModif();
    emit pasdecacheChanged();
}

/**
 * @brief configService::setNomsdcard
 * @param v
 */
void configService::setNomsdcard(QString v) {
    if(nomsdcard.compare(v)==0) {
        return;
    }

    if(!sdcard.isEmpty()) {
        source.removeAll(QString(sdcard + "/" + nomsdcard));
    }

    nomsdcard=v;
    settings.setValue(NOMSDCARD, v);
    settings.sync();
    setModif();
    initSDcard();
    VFI.setSources(source);
    emit nomsdcardChanged();
    emit sourceChanged();
}

/**
 * @brief configService::setPochette
 * @param v
 */
void configService::setPochette(bool v) {
    pochette= modeRO ? false : v;
    settings.setValue(POCHETTE, pochette);
    settings.sync();
    setModif();
    emit pochetteChanged();
}

/**
 * @brief configService::setPochetteWifi
 * @param v
 */
void configService::setPochetteWifi(bool v) {
    pochette_wifi=v;
    settings.setValue(POCHETTE_WIFI, v);
    settings.sync();
    setModif();
    emit pochetteWifiChanged();
}

/**
 * @brief configService::setPochetteFeedback
 * @param v
 */
void configService::setPochetteFeedback(bool v) {
    pochette_feedback=v;
    settings.setValue(POCHETTE_FEEDBACK, v);
    settings.sync();
    setModif();
    emit pochetteFeedbackChanged();
}

/**
 * @brief configService::setHistorique
 * @param v
 */
void configService::setHistorique(bool v) {
    historique=v;
    settings.setValue(HISTORIQUE, v);
    settings.sync();
    setModif();
    emit historiqueChanged();
}

/**
 * @brief configService::setModeHistorique
 * @param v
 */
void configService::setModeHistorique(quint16 v) {
    if(modeHistorique==v) {
        return;
    }
    modeHistorique=v;
    settings.setValue(HISTORIQUE_MODE, v);
    settings.sync();
    setModif();
    emit modeHistoriqueChanged();
}

/**
 * @brief configService::setOrdreFavoris
 * @param v
 */
void configService::setOrdreFavoris(quint16 v) {
    if(ordreFavoris==v) {
        return;
    }
    ordreFavoris=v;
    settings.setValue(FAVORIS_ORDRE, v);
    settings.sync();
    setModif();
    emit ordreFavorisChanged();
}

/**
 * @brief configService::setNbRecent
 * @param v
 */
void configService::setNbRecent(quint16 v) {
    if(nbRecent==v) {
        return;
    }
    nbRecent=v;
    settings.setValue(C_RECENT_MAX_NB, v);
    settings.sync();
    setModif();
    emit nbRecentChanged();
}

void configService::setPlaylistLocation(quint16 v) {
    if(playlistLocation == v) {
        return;
    }
    playlistLocation = v;
    settings.setValue(PLAYLIST_LOCATION, v);
    settings.sync();
    setModif();
    emit playlistLocationChanged();
}

void configService::setPlaylistFormat(quint16 v) {
    if(playlistFormat == v) {
        return;
    }
    playlistFormat = v;
    settings.setValue(PLAYLIST_FORMAT, v);
    settings.sync();
    setModif();
    emit playlistFormatChanged();
}

/**
 * @brief recharge la valeur par defaut
 */
void configService::parDefaut() {

    initialisation();

    settings.setValue(SHOWEMPTYDIR, SHOWEMPTYDIR_DEF);
    settings.setValue(AUTOSTART, AUTOSTART_DEF);
    settings.setValue(OPACITE_POCHETTE, OPACITE_POCHETTE_DEF);
    settings.setValue(VOIR_POCHETTE, VOIR_POCHETTE_DEF);
    settings.setValue(VOIR_CONTROLE, VOIR_CONTROLE_DEF);
    settings.setValue(DIRALBUM, DIRALBUM_DEF);
    settings.setValue(DIRALBUMSUB, DIRALBUMSUB_DEF);
    settings.setValue(DIR_TAG,DIR_TAG_DEF);
    settings.setValue(APPARENCE,APPARENCE_DEF);
    settings.setValue(MARGE,MARGE_DEF);
    settings.setValue(C_WIFI,C_WIFI_DEF);
    settings.setValue(POCHETTE,POCHETTE_DEF);
    settings.setValue(POCHETTE_WIFI,POCHETTE_WIFI_DEF);
    settings.setValue(POCHETTE_FEEDBACK,POCHETTE_FEEDBACK_DEF);
    settings.setValue(PAROLE_DIRECT,PAROLE_DIRECT_DEF);
    settings.setValue(PAROLE_EXTERNE,PAROLE_EXTERNE_DEF);
    settings.setValue(PASDECACHE,PASDECACHE_DEF);
    settings.setValue(HISTORIQUE,HISTORIQUE_DEF);
    settings.setValue(HISTORIQUE_MODE,HISTORIQUE_MODE_DEF);
    settings.setValue(FAVORIS_ORDRE,FAVORIS_ORDRE_DEF);
    settings.setValue(C_RECENT_MAX_NB,C_RECENT_MAX_NB_DEF);
    settings.setValue(PLAYLIST_LOCATION, PLAYLIST_LOCATION_DEF);
    settings.setValue(PLAYLIST_FORMAT, PLAYLIST_FORMAT_DEF);

    settings.sync();

    emit showEmptyDirChanged();
    emit autoStartChanged();
    emit opacitePochetteChanged();
    emit voirPochetteChanged();
    emit dirAlbumChanged();
    emit dirAlbumWithSubChanged();
    emit dirTagChanged();
    emit apparenceChanged();
    emit wifiChanged();
    emit pochetteChanged();
    emit lastfmChanged();
    emit paroleChanged();
    emit paroleExterneChanged();
    emit pochetteWifiChanged();
    emit pasdecacheChanged();
    emit historiqueChanged();
    emit modeHistoriqueChanged();
    emit ordreFavorisChanged();
    emit nbRecentChanged();
    emit playlistLocationChanged();
    emit playlistFormatChanged();
}

/**
 * @brief initialise la config avec les valeur par defaut,
 *  pas d'emit, donc permet l'override par exemple depuis un settings
 */
void configService::initialisation() {

    // repertoires de sources
    //   source.append(new QString(QDir::homePath()+"/Music"));
    //   source.append(new QString(QDir::homePath()+"/android_storage/Music"));


    /*
    // source.append(new QString("/media/sdcard/1E24-8DFF/Music"));

    QString sdcard = detecteSdcard();
    if( sdcard.isNull() == false) {
        QFileInfo info(sdcard+"/Music");
        if(info.isDir()||info.isReadable()) {
            source.append(new QString(sdcard+"/Music"));
        } else {
            source.append(new QString(sdcard));
        }
    }

    // verif si les sources existe, important si chargé depuis config
    QList<QString *>::iterator it = source.begin();
    while (it != source.end()) {

        QFileInfo info(*(*it));
        if( info.exists() && info.isReadable()) {
            ++it;
            continue;
        }

        it = source.erase(it);
    }
    */

    // les flag
    showEmptyDir = SHOWEMPTYDIR_DEF;
    autoStart = AUTOSTART_DEF;
    opacitePochette = OPACITE_POCHETTE_DEF;
    voirPochette = VOIR_POCHETTE_DEF;
    controle= VOIR_CONTROLE;
    dirAlbum = ActionContectMenu_JOUER;
    dirAlbumWithSub = ActionContectMenu_MENU;
    dirTag=DIR_TAG_DEF;
    apparence = APPARENCE_CLASSIQUE;

    marge=MARGE_DEF;
    wifi=C_WIFI_DEF;
    paroledirect=PAROLE_DIRECT_DEF;
    paroleexterne=PAROLE_EXTERNE_DEF;
    pochette=POCHETTE_DEF;
    pasdecache=PASDECACHE_DEF;
    historique=HISTORIQUE_DEF;
    modeHistorique=HISTORIQUE_MODE_DEF;
    ordreFavoris=FAVORIS_ORDRE_DEF;
    nbRecent = C_RECENT_MAX_NB_DEF;
    playlistLocation = PLAYLIST_LOCATION_DEF;
    playlistFormat = PLAYLIST_FORMAT_DEF;
    lastfm=false;
}

/**
 * @brief vrai si le rep sous sdcard existe
 * @return
 */
bool configService::getRepsdcardExist() const {

    if(SdcardLocator::getInstance().isPresent()==false) {
        return false;
    }

//    if( sdcard == QString::null) {
//        return false; // pas de sdcard
//    }

    QFileInfo f(sdcard + "/" + nomsdcard);
    return f.exists() && f.isReadable();
}

/**
 * @brief initSDcard
 * @return vrai si source pour sdcard ok
 */
bool configService::initSDcard()  {
    modeRO=false;

    if(SdcardLocator::getInstance().isPresent()==false) {
        return false;
    }

    // une sdcard et pas de répertoire
    if( getRepsdcardExist() ==false) {
        QDir d(sdcard + "/" + nomsdcard);
        Notification n;

        if( d.mkpath(".") == false) {
            n.setPreviewBody(vI18n::getInstance().get( "options.sdcard.erreur").arg(nomsdcard) );
            n.setHintValue("x-nemo-icon", "image://theme/icon-s-high-importance");
            n.publish();

            return false;
        }

        n.setPreviewBody(vI18n::getInstance().get( "options.sdcard.ok").arg(nomsdcard) );
        n.setHintValue("x-nemo-icon", "image://theme/icon-s-edit");
        n.publish();
    }

    source.append(QString(sdcard+"/"+ nomsdcard));

    // verif si ecriture possible
    QFileInfo f(sdcard+"/"+ nomsdcard);
    if(f.isWritable()==false) {
        modeRO=true;
        Notification n;

        n.setPreviewBody(vI18n::getInstance().get( "options.sdcard.ro") );
        n.setHintValue("x-nemo-icon", "image://theme/icon-high-importance");
        n.publish();

        pasdecache=true;
        pochette=false;
        emit pasdecacheChanged();
        emit pochetteChanged();
    }

    return true;
}
