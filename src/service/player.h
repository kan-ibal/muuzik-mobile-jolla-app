#ifndef PLAYER_H
#define PLAYER_H

#include <QObject>
#include <QQmlContext>
#include <qqml.h>
#include <QMediaPlayer>
#include <QMediaPlaylist>
#include <QList>
#include <QtDBus>
#include <QDebug>
#include <QElapsedTimer>

#include "../bean/MetaData.h"
#include "mpris/mpris.h"
#include "mpris/mprisplayer.h"

#define ACTION_AJOUTER  1
#define ACTION_REMPLACER 2

#define LECTURE_LISTE "lecteur2/liste"
#define LECTURE_LISTE_INDEX "lecteur2/index"
#define LECTURE_POSITION "lecteur2/position"
#define LECTURE_MODE "lecteur2/mode"
#define LECTURE_NOM "lecteur2/nom"

#define PLAYERSERVICE_QML_NAME "playerService"


using namespace std;
class MprisPlayer;
class playerService : public QObject {
    friend class QDBusConnection;
    friend class MprisPlayer;

    Q_OBJECT
    Q_PROPERTY(QQmlListProperty<MetaData> list READ getList NOTIFY listChanged )
    Q_PROPERTY(MetaData *title READ getTitle NOTIFY titleChanged )
    Q_PROPERTY(bool status READ getStatus NOTIFY statusChanged )
    Q_PROPERTY(qint64 index READ getIndex NOTIFY indexChanged )
    Q_PROPERTY(qint64 dureeTotale READ getDureeTotale NOTIFY listChanged )
    Q_PROPERTY(qint64 dureeTitre READ getDureeTitre NOTIFY dureeChanged )
    Q_PROPERTY(qint64 size READ getSize NOTIFY listChanged )
    Q_PROPERTY(bool random READ getRandom NOTIFY modeChanged )
    Q_PROPERTY(bool loop READ getLoop NOTIFY modeChanged )
    Q_PROPERTY(QString nom READ getNom )
    Q_PROPERTY(bool enCours READ estEnCours NOTIFY enCoursChanged )

    Q_PROPERTY(bool showControl READ getShowControl WRITE setShowControl NOTIFY showControlChanged)
    Q_PROPERTY(QString streamArtist READ getStreamArtist NOTIFY streamMetaDataChanged )
    Q_PROPERTY(QString streamTitle READ getStreamTitle NOTIFY streamMetaDataChanged )
    Q_PROPERTY(QString audioCodec READ getAudioCodec NOTIFY streamMetaDataChanged )
    Q_PROPERTY(int audioBitrate READ getAudioBitrate NOTIFY streamMetaDataChanged )
    Q_PROPERTY(bool isStream READ isCurrentStream NOTIFY titleChanged )


public:
    static playerService& getInstance();
    static void init(QQmlContext *context, QString name = PLAYERSERVICE_QML_NAME);

    /**
     * @brief ajouter à la liste de lecture
     * si un audio, ajoute un seul fichier, si rep, ajoute tout les fichiers audio du rep
     * @param vurl la liste des vurl vers des "albums" ou des titres à ajouter
     * @param recursif : si true, ajouter les média récursivement
     */
    Q_INVOKABLE void add(const QList<QString> &vurl, bool recursif = false);

    /**
     * @brief remplace la liste de lecture
     * si un audio, ajoute un seul fichier, si rep, ajoute tout les fichiers audio du rep
     * @param vurl la liste des vurl vers des "albums" ou des titres à ajouter
     * @param recursif : si true, ajouter les média récursivement
     */
    Q_INVOKABLE void replace(const QList<QString> &vurl, bool recursif = false);

    /**
      * recharge la precedente liste de lecture
      */
    Q_INVOKABLE void restoreSession();

    /**
     * @brief playerService::saveSession
     */
    Q_INVOKABLE void saveSession();

    /**
     * @brief retourne le nom de la playlist en cours ou vide si aucune
     */
    Q_INVOKABLE QString getNom() {return nom; }

    /**
     * @brief retourne la liste de lecture
     * @return
     */
    QQmlListProperty<MetaData> getList(){ return QQmlListProperty<MetaData>(this, liste);}
    QList<MetaData *> getRawListe() const { return liste;}

    /**
     * @brief retourne le titre en cours de lecture ou null si aucun
     * @return le titre ou null
     */
    Q_INVOKABLE MetaData *getTitle() const;

    /**
     * @brief duree de la playlist en seconde
     * @return
     */
    Q_INVOKABLE qint64 getDureeTotale() const {return dureeTotale;}

    /**
      * @brief temps restant de lecture
      * calculé à chasue appel !
      */
 //   Q_INVOKABLE qint64 getDureRestante();

    /**
     * @brief duree dans le titre en cours
     * @return
     */
    Q_INVOKABLE qint64 getDureeTitre() const { return media ? media->duration()/1000 : 0; }

    /**
     * @brief taille de la playlist
     * @return
     */
    Q_INVOKABLE qint64 getSize() const { return playlist ? playlist->mediaCount() : 0; }

    /**
     * @brief effacer la liste et eventuellement arreter la lecture
     * @return
     */
    Q_INVOKABLE void effacer();

    /**
     * @brief retire le titre qui a l'uuid uuid
     * @param uuid
     */
    Q_INVOKABLE void retirerUuid(const qint64 uuid);

    /**
     * @brief deplace vers le haut (debut de liste) le titre à l'index index
     * @param index
     */
    Q_INVOKABLE void monterUuid(const qint64 uuid);

    /**
     * @brief deplace vers le bas le titre à l'index index
     * @param index
     */
    Q_INVOKABLE void descendreUuid(const qint64 uuid);

    /**
     * @brief lire ou mettre en pause
     */
    Q_INVOKABLE void play(qint64 index = -1);

    /**
     * @brief mettre en pause la lecture si en cours
     */
    Q_INVOKABLE void pause();

    /**
     * @brief stopper la lecture
     */
    Q_INVOKABLE void stop();

    /**
     * @brief arrete et libere proprement le media player et dbus
     */
    Q_INVOKABLE void arreter();

    /**
     * @brief passe à la piste suivant si possible.
     * si en pause, lance la lecture
     * @return vrai si passe à la piste suivant
     */
    Q_INVOKABLE void next();
    Q_INVOKABLE bool aSuivant() {return playlist ? (playlist->nextIndex() != -1) : false;}

    /**
     * @brief passe à la piste precedente si possible.
     * si en pause, lance la lecture
     * @return vrai si passe à la piste precedente
     */
    Q_INVOKABLE void previous();
    Q_INVOKABLE bool aPrecedent() {return playlist ? (playlist->mediaCount() > 0 && (playlist->previousIndex() != -1 || !playHistory.isEmpty() || playlist->playbackMode() == QMediaPlaylist::Random || playlist->currentIndex() >= 0)) : false;}

    /**
     * @brief avance la position de la lecture dnas le morceau en cours
     */
    Q_INVOKABLE void avancer(qint64);

    /**
      * @brief retourne l'etat du player
      * @return true si en lecture, sinon false
      */
    Q_INVOKABLE bool getStatus() const { return media ? (media->state() == QMediaPlayer::PlayingState) : false; }

    /**
      * @brief retourne la piste en position de lecture
      * @return
      */
    Q_INVOKABLE int getIndex() const { return playlist ? playlist->currentIndex() : -1; }

    /**
      * @brief retourne la position en seconde du titre en cours
      * @return
      */
    Q_INVOKABLE int getPosition() const;
    qint64 getPositionMs() const;

    /**
     * @brief getRandom
     * @return
     */
    bool getRandom() { return playlist ? (playlist->playbackMode() == QMediaPlaylist::Random) : false; }

    /**
      * @brief switchRandom
      */
    Q_INVOKABLE void switchRandom();

    /**
     * @brief getLoop
     * @return
     */
    bool getLoop() { return playlist ? (playlist->playbackMode() == QMediaPlaylist::Loop) : false; }

    /**
      * @brief switchLoop
      */
    Q_INVOKABLE void switchLoop();

    /**
      * @brief sauvegarde la playliste actuelle sous le non nom
      */
   // Q_INVOKABLE void sauvegarder(const QString & nom);

    /**
     * @return vrai si un scan est en cours
     */
    Q_INVOKABLE bool estEnCours() const {
        return raf;
    }

    // gestion du panneau des controle plein ecran
    bool getShowControl() const { return showControl; }
    void setShowControl(const bool val);

    QString getStreamArtist() const { return currentStreamArtist; }
    QString getStreamTitle() const { return currentStreamTitle; }
    QString getAudioCodec() const { return currentAudioCodec; }
    int getAudioBitrate() const { return currentAudioBitrate; }
    Q_INVOKABLE bool isCurrentStream() const;


protected :
    // on en a besoin depuis doChargement
    bool isSessionRestaurePlaylist() { return sessionRestaurePlaylist;}

private:
    explicit playerService(QObject *parent = 0);
    ~playerService();

    QMediaPlayer *media;
    QMediaPlaylist *playlist;
    QList<MetaData *> liste;
    qint64 dureeTotale;

    QString nom; // nom de la palylist en cours, vide si aucune

    Mpris *mpris;
    MprisPlayer *mprisplayer;
    QString mprisServiceName;
    bool sessionRestaure;
    bool sessionRestaurePlaylist;

    bool raf;
    qint64 pendingRestorePositionMs;
    QElapsedTimer lastSaveTimer;
    QList<int> playHistory;
    int lastCurrentIndex;
    bool navigatingHistory;

    QSettings settings;

    bool showControl;
    QString currentStreamArtist;
    QString currentStreamTitle;
    QString currentAudioCodec;
    int currentAudioBitrate;

    void persistSession();
    void applyPendingRestorePosition();
    void updateStreamMetaData(bool clearFirst = false);
    void resetStreamMetaData();

    /**
     * @brief parcours la liste et retourne l'index du titre qui a l'uuid ; -1 si trouve pas
     * @param uuid
     * @return
     */
    qint64 trouverIndex( const qint64 uuid) const;

    /**
     * @brief créé la liste de lecture à ajouter au player
     * la liste sera retourné au service par le signal chargementFaitIntr
     * @param vurl liste des sources : album (rep) ou audio (file)
     * @param recursif: true si doit passer les rep recursivement
     * @param position : utiliser lors de la restauration d'une session pour le titre a démmarrer
     * si -1, ne faut pas de changement d'index
     * @param autoplay : si true et que le lecteur n'est pas en play, le lance automatiquement
     */
    static void doChargement(const QList<QString> &vurl,  bool recursif, qint64 position=-1, bool autoplay=false);

signals:
    void listChanged(); // ajout / suppression de titre dans la liste
    void statusChanged(bool s); // passe en lecture, pause, arret
    void indexChanged(qint64 i); // position de la liste de lecture a changé
    void titleChanged(MetaData *t); // nouveau titre en lecture, peut être null si aucun
    void dureeChanged(qint64 value); // avancement dans la lecture du media, en s
    void modeChanged(); // quand le type de lecture change (normal, ramdon, boucle)

    void enCoursChanged();

    /**
     * @brief envoyer par le qfurute pour connecter sur le service par finChargementImp
     * @param liste
     * @param position utilisé lors de la restauration d'une session
     */
    void chargementFaitIntr( QList<MetaData *> *liste, const qint64 position, bool autoplay);

    void showControlChanged(); // si doit afficher ou masques les controles en plein ecran
    void streamMetaDataChanged();


public slots  :
    void handleMprisPropertiesChanged(const QString &interface, const QMap<QString, QVariant> &changed, const QStringList &invalidated);

    // si change l'affichage des controle en plein ecran dans la config
    void ControlChanged_impl();

public slots:
    // suppression de l'url ( et de tout les childs)
    void onRemoved(QString vurl);
    void onMoved(QString vurl, QString as);
    void onRenamed(QString vurl, QString name);

private slots :
    void changementEtat(QMediaPlayer::State state);
    void changementPosition(qint64 position); // changenent position en miliseconde
    void changementDureeMedia(qint64 duration);
    void changementIndex(int position); // changement de media
    void changementMediaStatus(QMediaPlayer::MediaStatus status);
    void changementSeekable(bool seekable);
    void changementMetaData();


    void appelRecu(const QDBusMessage &a);

protected slots :
    /**
     * @brief la liste de lecture est créé et disponible pour affichage
     * @param liste
     * @param position utilisé lors de la restauration d'une session
     */
    void finChargementImp(QList<MetaData *>*liste, const qint64 position, bool autoplay);

    /**
     * @brief affiche la notification de taille max sur le thread principal
     */
    void notifierTailleMax();

};

#endif // PLAYER_H
