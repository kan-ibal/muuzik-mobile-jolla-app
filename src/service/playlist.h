#ifndef PLAYLIST_H
#define PLAYLIST_H

#include <QObject>
#include <QQmlContext>
#include <QString>
#include <QList>

#include <qqml.h>
#include "../bean/MetaData.h"
#include "../bean/pls.h"


#define PLAYLISTERVICE_QML_NAME "PlaylistService"

#define PLAYLISTERVICE_LOCALNAME "Playlist"



/**
 * @brief gestion des playlist
 * les playlist stocke des url physique pour être compatible avec les autres appli
  */
class playlistService : public QObject {

    Q_OBJECT
    Q_PROPERTY(QQmlListProperty<Pls> liste READ getListe NOTIFY listeChanged )

public:
    static playlistService& getInstance();
    static void init(QQmlContext *context, QString name = PLAYLISTERVICE_QML_NAME);
    virtual ~playlistService();

    /**
     * @brief retourne la liste de toutes les playlists
     * @return
     */
    Q_INVOKABLE QQmlListProperty<Pls> getListe(){ return QQmlListProperty<Pls>(this, liste);}

    /**
      * @brief retourne les info de la playlist à partir de son id
      * @param uuid
      * @return la playlist
      */
    Q_INVOKABLE QVariant getPls(const qint64 uuid) const;
    /**
     * @brief retourne la liste des medias de la playlist defini par uuid
     * @param uuid
     * @return la liste ou vide si rien (vide)
     */
    QList<MetaData *> ouvrir(const qint64 uuid);

    /**
      * @brief version non blocante de ouvrir
      * la liste sera disponible sur le signal playlistOuverte
      * @param uuid  id de la liste
      * @return false sur erreur, true si lance l'ouverture + signal
      */
    Q_INVOKABLE bool ouvrirAsync(const qint64 uuid);

    /**
      *@brief : cree une nouvelle liste (vide)
      *@return : l'uuid de la nouvelle liste ou -1 sur erreur
      */
    Q_INVOKABLE qint64 creer(const QString &nom);

    /**
     * @brief enregistre la playlist
     * @param uuid
     * @param tracks la liste des titres
     */
    bool ecrire(const qint64 uuid,const QList<MetaData *> tracks);

    /**
     * @brief ecrireASunc
     * @param uuid
     * @param tracks
     * @return
     */
    bool ecrireAsync(const qint64 uuid,const QList<MetaData *> tracks);

    /**
     * @brief enregistre la playlist
     * @param uuid
     * @param tracks la liste des titres
     */
    Q_INVOKABLE bool ecrireAsync(const qint64 uuid,const QList<QString> tracks);

    /**
      * @brief ajoute les titres à la playlist pointé par uuid
      * cette fonction a vocation à être appelé depuis le navigateur
      * donc tracks est des vurl
      * @param uuid : la playlist
      * @param tracks : la listes des pistes
      */
    Q_INVOKABLE bool ajouter(const qint64 uuid, const QList<QString> tracks);

    /**
      * @brief version non blocante d'ajouter
      * resultat sur le signal tracksChanged
      * @param uuid
      * @param tracks la liste des titres
      * @return false sur erreur, true si lance l'ajout + signal
      */
    Q_INVOKABLE bool ajouterAsync(const qint64 uuid, const QList<QString> tracks);

    /**
     * @brief suprimme la playlist avec l'uuid uuid
     * @param uuid
     */
    Q_INVOKABLE void supprimer(const qint64 uuid);

    /**
      * @brief renomme une liste de lecture
      * @param uuid : l'uuid de la liste
      * @param nom : le nouveau non, doit être non null et ne pas exister
      * @return true si ok + envoie le signal , false si non
      */
    Q_INVOKABLE bool renommer(const qint64 uuid,const QString &nom);

private:
    explicit playlistService(QObject *parent = 0);

    QList<Pls *> liste;

    QString repPath;

    /**
     * @brief recharger la liste des playlist
     */
    void recharger();

    /**
     * @brief fonction app par le qfuture
     * @param uuid
     */
    static void doOuvrir(const qint64 uuid);

    /**
     * @brief doEcrite
     * @param tracks
     */
    static void doEcrite(const qint64 uuid, const QList<MetaData *> tracks);

    /**
     * @brief fonction app par le qfuture
     * @param uuid
     */
    static void doAjouter(const qint64 uuid, const QList<QString> tracks);

    /**
     * @brief retourne l'index dans la liste correspondant a l'uuid ou -1
     * @param uuid
     * @return
     */
    qint64 trouverIndex( const qint64 uuid) const;


    /**
     * @brief classerListe
     */
    void classerListe();

    /**
     * @brief ordreAlpha
     * @param a
     * @param b
     * @return
     */
    static bool ordreAlpha(  Pls * a, Pls  * b );


    /**
     * @brief retourne le repertoire ou sont les playlists
     * @return
     */
    static QString getPlaylistLocation();

signals:
    /**
     * @brief ajout / suppression / renommage d'une playlist
     */
    void listeChanged();

    /**
     * @brief une liste à changé de nom
     * @param uuid de la liste
     * @param nouveauNom : le nouveau nom
     */
    void nomChanged(const qint64 uuid, const QString &nouveauNom);

    /**
     * @brief la playliste est ouverte
     * @param nom son non
     * @param liste la liste de <MetaData *>
     */
    void playlistOuverte(const qint64 uuid, QVariantList liste);

    /**
     * @brief la liste des tracks pour uuid a changé
     * @param uuid
     */
    void tracksChanged(const qint64 uuid);

public slots:
    // suppression de l'url ( et de tout les childs)
    void onRemoved(QString vurl);

    void onMoved(QString vurl, QString as);

    void onRenamed(QString vurl, QString name);
};

#endif // PLAYLIST_H
