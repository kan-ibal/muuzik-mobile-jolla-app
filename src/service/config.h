#ifndef CONFIG_H
#define CONFIG_H
#include <QObject>
#include <QQmlContext>
#include <qqml.h>
#include <QSettings>
#include <QDateTime>

using namespace std;

#define NIVEAUCONFIG "config/verison"
#define NIVEAUCONFIGATTENDU 1

#define SHOWEMPTYDIR "navigateur/showemptydir"
#define SHOWEMPTYDIR_DEF false
#define DIRALBUM "navigateur/diralbum"
#define DIRALBUM_DEF 1
#define DIRALBUMSUB "navigateur/diralbumsub"
#define DIRALBUMSUB_DEF 3
#define DIR_TAG "navigateur/dirtag"
#define DIR_TAG_DEF true

#define AUTOSTART "lecteur/autostart"
#define AUTOSTART_DEF true
#define APPARENCE "lecteur/apparence"
#define APPARENCE_DEF 0
#define OPACITE_POCHETTE "lecteur/opacite"
#define OPACITE_POCHETTE_DEF 90
#define VOIR_POCHETTE "lecteur/pochette"
#define VOIR_POCHETTE_DEF true

#define VOIR_CONTROLE "lecteur/controle"
#define VOIR_CONTROLE_DEF false

#define ActionContectMenu_OUVRIR 0
#define ActionContectMenu_JOUER 1
#define ActionContectMenu_AJOUTER 2
#define ActionContectMenu_MENU 3

#define APPARENCE_CLASSIQUE 0
#define APPARENCE_HORLOGUE 1
#define APPARENCE_PAPIERPAINT 2

#define C_WIFI "reseau/wifi"
#define C_WIFI_DEF true

#define MARGE "liste/marge"
#define MARGE_DEF true

#define LASTFM_OK "infos/lastfm"

#define PAROLE_DIRECT "parole/direct"
#define PAROLE_DIRECT_DEF true

#define PAROLE_EXTERNE "parole/externe"
#define PAROLE_EXTERNE_DEF false

#define NOMSDCARD "sdcard/nom"
#define NOMSDCARD_DEF "Music"

#define POCHETTE "pochette/telechargement"
#define POCHETTE_DEF true
#define POCHETTE_WIFI "pochette/wifi"
#define POCHETTE_WIFI_DEF true

#define POCHETTE_FEEDBACK "pochette/feedback"
#define POCHETTE_FEEDBACK_DEF true

#define PASDECACHE "cache/autoriser"
#define PASDECACHE_DEF false

#define HISTORIQUE "historique/autoriser"
#define HISTORIQUE_DEF true

#define HISTORIQUE_MODE "historique/mode"
#define HISTORIQUE_MODE_DEF 0

#define FAVORIS_ORDRE "favoris/ordre"
#define FAVORIS_ORDRE_date 0
#define FAVORIS_ORDRE_date_inv 1
#define FAVORIS_ORDRE_alpha 2
#define FAVORIS_ORDRE_DEF 0

#define C_RECENT_MAX_NB "recent/nb"
#define C_RECENT_MAX_NB_DEF 50

#define PLAYLIST_LOCATION "playlist/location"
#define PLAYLIST_LOCATION_HOME 0
#define PLAYLIST_LOCATION_MUSIC 1
#define PLAYLIST_LOCATION_SDCARD 2
#define PLAYLIST_LOCATION_DEF PLAYLIST_LOCATION_HOME

#define PLAYLIST_FORMAT "playlist/format"
#define PLAYLIST_FORMAT_JOLLA 0
#define PLAYLIST_FORMAT_UNPLAYER 1
#define PLAYLIST_FORMAT_DEF PLAYLIST_FORMAT_JOLLA

#define CONFIGSERVICE_QML_NAME "ConfigService"

class configService : public QObject {

    Q_OBJECT
    Q_PROPERTY(QList<QString> listeSource READ getSource NOTIFY sourceChanged )
    Q_PROPERTY(bool showEmptyDir READ getShowEmptyDir WRITE setShowEmptyDir NOTIFY showEmptyDirChanged )
    Q_PROPERTY(bool autoStart READ getAutoStart WRITE setAutoStart NOTIFY autoStartChanged )
    Q_PROPERTY(quint64 opacitePochette READ getOpacitePochette WRITE setOpacitePochette NOTIFY opacitePochetteChanged )
    Q_PROPERTY(bool voirPochette READ getVoirPochette WRITE setVoirPochette NOTIFY voirPochetteChanged )
    Q_PROPERTY(bool controle READ getControle WRITE setControle NOTIFY controleChanged )
    Q_PROPERTY(quint64 dirAlbum READ getIsDirAlbum WRITE setIsDirAlbum NOTIFY dirAlbumChanged )
    Q_PROPERTY(quint64 dirAlbumWithSub READ getIsDirAlbumWithSub WRITE setIsDirAlbumWithSub NOTIFY dirAlbumWithSubChanged )
    Q_PROPERTY(quint64 dirTag READ getIsDirTag WRITE setIsDirTag NOTIFY dirTagChanged )
    Q_PROPERTY(quint64 apparence READ getApparence WRITE setApparence NOTIFY apparenceChanged )
    Q_PROPERTY(bool marge READ getMarge WRITE setMarge NOTIFY margeChanged )
    Q_PROPERTY(bool wifi READ getWifi WRITE setWifi NOTIFY wifiChanged )
    Q_PROPERTY(bool lastfm READ getLastfm WRITE setLastfm NOTIFY lastfmChanged  )
    Q_PROPERTY(bool paroledirect READ getParoleDirect WRITE setParoleDirect NOTIFY paroleChanged )
    Q_PROPERTY(bool paroleexterne READ getParoleExterne() WRITE setParoleExterne NOTIFY paroleExterneChanged )
    Q_PROPERTY(QString nomsdcard READ getNomsdcard NOTIFY nomsdcardChanged )
    Q_PROPERTY(bool pasdecache READ getPasDecache WRITE setPasDecache NOTIFY pasdecacheChanged )
    Q_PROPERTY(bool pochette READ getPochette WRITE setPochette NOTIFY pochetteChanged )
    Q_PROPERTY(bool pochetteWifi READ getPochetteWifi WRITE setPochetteWifi NOTIFY pochetteWifiChanged )
    Q_PROPERTY(bool pochetteFeedback READ getPochetteFeedback WRITE setPochetteFeedback NOTIFY pochetteFeedbackChanged )
    Q_PROPERTY(bool historique READ getHistorique WRITE setHistorique NOTIFY historiqueChanged )
    Q_PROPERTY(quint16 modeHistorique READ getModeHistorique WRITE setModeHistorique NOTIFY modeHistoriqueChanged )
    Q_PROPERTY(quint16 ordreFavoris READ getOrdreFavoris WRITE setOrdreFavoris NOTIFY ordreFavorisChanged ) 

    Q_PROPERTY(quint16 nbRecent READ getNbRecent WRITE setNbRecent NOTIFY nbRecentChanged )
    Q_PROPERTY(quint16 playlistLocation READ getPlaylistLocation WRITE setPlaylistLocation NOTIFY playlistLocationChanged )
    Q_PROPERTY(quint16 playlistFormat READ getPlaylistFormat WRITE setPlaylistFormat NOTIFY playlistFormatChanged )


public:
    static configService& getInstance();

    /**
     * @brief register the service into the context so it can be used in qml
     * @param context
     * @param name : register service on this name, default to CONFIGSERVICE_QML_NAME
     */
    static void init(QQmlContext *context, QString name=CONFIGSERVICE_QML_NAME);

    // remplace la config utilisateur par les valeurs par defaut
    Q_INVOKABLE void parDefaut();

    /**
     * @brief derniere date de modification de la config
     * @return
     */
    QDateTime dateModif() const { return modif; }

    /**
     * @brief retourne la liste des repertoires contenant des mp3 pour le disque virtuel
     * @return
     */
   Q_INVOKABLE QList<QString> getSource() { return source; }

    // si autorise les repertoires vides
    bool getShowEmptyDir() const { return showEmptyDir; }
    void setShowEmptyDir( const bool );

    // si demarre automatiquement a lecture // selement pour 'remplacer', pas 'ajouter'
    bool getAutoStart() const { return autoStart; }
    void setAutoStart( const bool );

    // opacite de la pochette dans la liste de lecture
    quint64 getOpacitePochette() const { return opacitePochette; }
    void setOpacitePochette( const quint64 );

    // true si les controles doivent être affiché à l'ouverture du player
    bool getControle() const { return controle; }
    void setControle( const bool );

    // si affiche la pochette dans la liste de lecture
    bool getVoirPochette() const { return voirPochette; }
    void setVoirPochette( const bool );

    // si un repertoire ne contient que des fichiers audio : jouer
    qint64 getIsDirAlbum() const { return dirAlbum; }
    void setIsDirAlbum( const qint64 );

    // si un repertoire contient des fichiers audio mais aussi des sub dir
    qint64 getIsDirAlbumWithSub() const { return dirAlbumWithSub; }
    void setIsDirAlbumWithSub( const qint64 );

    // si fait une recherche année, groupe album ou affiche le nom simple
    bool getIsDirTag() const { return dirTag; }
    void setIsDirTag( const bool );




    // apparence du player
    qint64 getApparence() const { return apparence; }
    void setApparence(const qint64);

    // si large dans les listes
    bool getMarge() const {return marge; }
    void setMarge(bool v);

    // si seulement en wifi
    bool getWifi() const {return wifi; }
    void setWifi(bool v);

    // auth api last.fm
    bool getLastfm() const {return lastfm; }
    Q_INVOKABLE void setLastfm(bool v=true);

    // si telecharge directemet les paroles
    bool getParoleDirect() const {return paroledirect; }
    void setParoleDirect(bool v);

    // vrai si prefere utiliser un naviagateur externe
    bool getParoleExterne() const {return paroleexterne; }
    void setParoleExterne(bool v);

    // si n'utilise pas de cahce
    bool getPasDecache() const {return pasdecache; }
    void setPasDecache(bool v);

    // le rep sous sdcard
    QString getNomsdcard() const {return nomsdcard; }
    Q_INVOKABLE void setNomsdcard(QString v);

    // téléchanrgement des pochettes manquates
    bool getPochette() const {return pochette; }
    void setPochette(bool v);

    // téléchargement des pochette seulement en wifi
    bool getPochetteWifi() const {return pochette_wifi; }
    void setPochetteWifi(bool v);

    // si affiche une notification en cas de récup de pochette
    bool getPochetteFeedback() const {return pochette_feedback; }
    void setPochetteFeedback(bool v);

    // si utlise l'historique
    bool getHistorique() const {return historique; }
    void setHistorique(bool v);

    // 0 : 1 semaine, 1 : 1 mois, 2 : tout
    quint16 getModeHistorique() const {return modeHistorique; }
    void setModeHistorique(quint16 v);

    quint16 getOrdreFavoris() const {return ordreFavoris; }
    void setOrdreFavoris(quint16 v);

    // max album à afficher en recent
    quint16 getNbRecent() const {return nbRecent; }
    void setNbRecent(quint16 v);

    // playlist location (0: ~/Playlists, 1: ~/Music/playlists, 2: sdcard/Music/playlists)
    quint16 getPlaylistLocation() const { return playlistLocation; }
    void setPlaylistLocation(quint16 v);

    // playlist write format (0: Jolla Media player file://, 1: Unplayer /path)
    quint16 getPlaylistFormat() const { return playlistFormat; }
    void setPlaylistFormat(quint16 v);


    /**
     * @brief vrai si le rep sous sdcard existe
     * @return
     */
    Q_INVOKABLE bool getRepsdcardExist() const;

    // gestion du niveau de la config
    Q_INVOKABLE bool getNiveauConfigOk() const {return configversion==NIVEAUCONFIGATTENDU; }
    Q_INVOKABLE void setNiveauConfigOk();

private :
    explicit configService(QObject *parent = 0);

    QSettings settings;
    QDateTime modif; // derniere date de modfif de la config

    // niveau de la config, savoir si le soft est à jour
    quint64 configversion;

    QList<QString> source; // source des reperoires contenant les mp3 pour le disque virtuel
    bool showEmptyDir; // vrai si autorise les repertoires vides dans l'explorateur

    bool autoStart; // vrai pour demarrer automatiquement la lesture
    quint64 opacitePochette; // opacite de la pochette dans la liste de lecture
    bool voirPochette;// true si affiche la pochette dans la liste de lecture
    bool controle; // true si les controles doivent être affiché à l'ouverture du player

    quint64 dirAlbum; // si ne contient que des fichiers audio
    quint64 dirAlbumWithSub; // si contient des fichiers audio et des sous repertoires
    bool dirTag;

    quint64 apparence; // apparence du player

    bool wifi; // si vrai ne telecharge qu'en wifi

    bool marge; // vrai si ajoute des marge dans les listes
    bool lastfm; // si last.fm autorisé
    bool paroledirect; // vrai si telecharge directement les paroles
    bool paroleexterne; // vrai si prefere charger dans un navigateur externe
    bool pasdecache; // si autorise le cache

    bool pochette; // si autorise le telechargement des pochettes
    bool pochette_wifi; // si autorise le telechargement des pochettes
    bool pochette_feedback; // si affiche une notification en cas de récup de pochette

    bool historique; // si utilise l'historique
    quint16 modeHistorique; // 0 : 1 semaine, 1 : 1 mois, 2 : tout

    quint16 ordreFavoris;

    quint16 nbRecent;   // nombre maximal d'album a afficher dans l'onglet recent

    quint16 playlistLocation;
    quint16 playlistFormat;

    QString nomsdcard; // le rep our les fichiers audio sur la sdcard

    QString sdcard; // url vers la sdcard (sipresente ou null)

bool modeRO; // si en read only, ne pas autoriser les service de cache / recup

    /**
     * @brief changement dans la config -> sauvegarde
     */
    void setModif();

    /**
     * @brief initSDcard
     * @return vrai si source pour sdcard ok
     */
    bool initSDcard();

    /**
     * @brief initialise la config avec les valeur par defaut,
     *  pas d'emit, donc permet l'override par exemple depuis un settings
     */
    void initialisation();

private slots:
    void onSdcardChanged();

signals:
    void sourceChanged();
    void showEmptyDirChanged();
    void autoStartChanged();
    void opacitePochetteChanged();
    void voirPochetteChanged();
    void controleChanged();
    void valeurParDefautChanged();

    void dirAlbumChanged();
    void dirAlbumWithSubChanged();
    void dirTagChanged();

    void apparenceChanged();
    void margeChanged();

    void dumpArtworkChanged();
    void foundArtworkChanged();
    void dumpLengthChanged();
    void wifiChanged();
    void lastfmChanged();
    void paroleChanged();
    void paroleExterneChanged();

    void pasdecacheChanged();

    void pochetteChanged();
    void pochetteWifiChanged();
    void pochetteFeedbackChanged();

    void historiqueChanged();
    void modeHistoriqueChanged();

    void ordreFavorisChanged();

    void nbRecentChanged();
    void nomsdcardChanged();
    void playlistLocationChanged();
    void playlistFormatChanged();
};

//Q_DECLARE_METATYPE(configService::ActionContectMenu)

#endif // CONFIG_H
