#ifndef LECTEUR_H
#define LECTEUR_H

#include <QObject>
#include <QQmlContext>
#include <qqml.h>
#include <QMediaPlayer>
#include <QMediaPlaylist>
#include <QList>
#include <QtDBus>
#include <QDebug>

#include "../bean/MetaData.h"
#include "mpris/mpris.h"
#include "mpris/mprislecteur.h"

#define ACTION_AJOUTER  1
#define ACTION_REMPLACER 2

#define LECTURE_LISTE "lecteur2/liste"
#define LECTURE_LISTE_INDEX "lecteur2/index"

#define LECTEURSERVICE_QML_NAME "lecteurService"


using namespace std;
class MprisLecteur;
class lecteurService : public QObject {
    friend class QDBusConnection;
    friend class MprisLecteur;

    Q_OBJECT
    Q_PROPERTY(QQmlListProperty<MetaData> liste READ getListe NOTIFY listeChanged )
    Q_PROPERTY(MetaData *titre READ getTitre NOTIFY titreChanged )
    Q_PROPERTY(bool etat READ getEtat NOTIFY etatChanged )
    Q_PROPERTY(qint64 index READ getIndex NOTIFY indexChanged )
    Q_PROPERTY(qint64 dureeTotale READ getDureeTotale NOTIFY listeChanged )
    Q_PROPERTY(qint64 dureeTitre READ getDureeTitre NOTIFY dureeChanged )
    Q_PROPERTY(qint64 taille READ getTaille NOTIFY listeChanged )
    Q_PROPERTY(bool aleatoire READ getAleatoire NOTIFY modeChanged )
    Q_PROPERTY(bool boucle READ getBoucle NOTIFY modeChanged )
    Q_PROPERTY(QString nom READ getNom )
    Q_PROPERTY(bool enCours READ estEnCours NOTIFY enCoursChanged )

    Q_PROPERTY(bool showControl READ getShowControl WRITE setShowControl NOTIFY showControlChanged)


public:
    static lecteurService& getInstance();
    static void init(QQmlContext *context, QString name = LECTEURSERVICE_QML_NAME);

    /**
     * @brief ajouter à la liste de lecture
     * si un audio, ajoute un seul fichier, si rep, ajoute tout les fichiers audio du rep
     * @param vurl la liste des vurl vers des "albums" ou des titres à ajouter
     * @param recursif : si true, ajouter les média récursivement
     */
    Q_INVOKABLE void ajouter(const QList<QString> &vurl, bool recursif = false);

    /**
     * @brief remplace la liste de lecture
     * si un audio, ajoute un seul fichier, si rep, ajoute tout les fichiers audio du rep
     * @param vurl la liste des vurl vers des "albums" ou des titres à ajouter
     * @param recursif : si true, ajouter les média récursivement
     */
    Q_INVOKABLE void remplacer(const QList<QString> &vurl, bool recursif = false);

    /**
      * recharge la precedente liste de lecture
      */
    Q_INVOKABLE void restaurerSession();

    /**
     * @brief lecteurService::sauverSession
     */
    Q_INVOKABLE void sauverSession();

    /**
     * @brief retourne le nom de la playlist en cours ou vide si aucune
     */
    Q_INVOKABLE QString getNom() {return nom; }

    /**
     * @brief retourne la liste de lecture
     * @return
     */
    QQmlListProperty<MetaData> getListe(){ return QQmlListProperty<MetaData>(this, liste);}
    QList<MetaData *> getRawListe() const { return liste;}

    /**
     * @brief retourne le titre en cours de lecture ou null si aucun
     * @return le titre ou null
     */
    Q_INVOKABLE MetaData *getTitre() const;

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
    Q_INVOKABLE qint64 getTaille() const { return playlist ? playlist->mediaCount() : 0; }

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
    Q_INVOKABLE void lire(qint64 index = -1);

    /**
     * @brief stopper la lecture
     */
    Q_INVOKABLE void stopper();

    /**
     * @brief arrete et libere proprement le media player et dbus
     */
    Q_INVOKABLE void arreter();

    /**
     * @brief passe à la piste suivant si possible.
     * si en pause, lance la lecture
     * @return vrai si passe à la piste suivant
     */
    Q_INVOKABLE void suivant();
    Q_INVOKABLE bool aSuivant() {return getIndex() < getTaille() - 1;}

    /**
     * @brief passe à la piste precedente si possible.
     * si en pause, lance la lecture
     * @return vrai si passe à la piste precedente
     */
    Q_INVOKABLE void precedent();
    Q_INVOKABLE bool aPrecedent() {return getIndex()>0;}

    /**
     * @brief avance la position de la lecture dnas le morceau en cours
     */
    Q_INVOKABLE void avancer(qint64);

    /**
      * @brief retourne l'etat du player
      * @return true si en lecture, sinon false
      */
    Q_INVOKABLE bool getEtat() const { return media ? (media->state() == QMediaPlayer::PlayingState) : false; }

    /**
      * @brief retourne la piste en position de lecture
      * @return
      */
    Q_INVOKABLE int getIndex() const { return playlist ? playlist->currentIndex() : -1; }

    /**
      * @brief retourne la position en seconde du titre en cours
      * @return
      */
    Q_INVOKABLE int getPosition() const { return media ? media->position()/1000 : 0; }

    /**
     * @brief getAleatoire
     * @return
     */
    bool getAleatoire() { return playlist ? (playlist->playbackMode() == QMediaPlaylist::Random) : false; }

    /**
      * @brief setAleatoire
      */
    Q_INVOKABLE void switchAleatoire();

    /**
     * @brief getBoucle
     * @return
     */
    bool getBoucle() { return playlist ? (playlist->playbackMode() == QMediaPlaylist::Loop) : false; }

    /**
      * @brief setBoucle
      */
    Q_INVOKABLE void switchBoucle();

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


protected :
    // on en a besoin depuis doChargement
    bool isSessionRestaurePlaylist() { return sessionRestaurePlaylist;}

private:
    explicit lecteurService(QObject *parent = 0);
    ~lecteurService();

    QMediaPlayer *media;
    QMediaPlaylist *playlist;
    QList<MetaData *> liste;
    qint64 dureeTotale;

    QString nom; // nom de la palylist en cours, vide si aucune

    Mpris *mpris;
    MprisLecteur *mprislecteur;
    bool sessionRestaure;
    bool sessionRestaurePlaylist;

    bool raf;

    QSettings settings;

    bool showControl;

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
    void listeChanged(); // ajout / suppression de titre dans la liste
    void etatChanged(bool s); // passe en lecture, pause, arret
    void indexChanged(qint64 i); // position de la liste de lecture a changé
    void titreChanged(MetaData *t); // nouveau titre en lecture, peut être null si aucun
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
    void changementIndex(int position); // changement de media


    void appelRecu(const QDBusMessage &a);

protected slots :
    /**
     * @brief la liste de lecture est créé et disponible pour affichage
     * @param liste
     * @param position utilisé lors de la restauration d'une session
     */
    void finChargementImp(QList<MetaData *>*liste, const qint64 position, bool autoplay);


};

#endif // LECTEUR_H
