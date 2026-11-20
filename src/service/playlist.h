#ifndef PLAYLIST_H
#define PLAYLIST_H

#include <QObject>
#include <QQmlContext>
#include <QString>
#include <QStringList>
#include <QVariantList>
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
    QList<MetaData *> open(const qint64 uuid);

    /**
      * @brief version non blocante de open
      * la liste sera disponible sur le signal playlistOuverte
      * @param uuid  id de la liste
      * @return false sur erreur, true si lance l'ouverture + signal
      */
    Q_INVOKABLE bool openAsync(const qint64 uuid);

    /**
      *@brief : cree une nouvelle liste (vide)
      *@return : l'uuid de la nouvelle liste ou -1 sur erreur
      */
    Q_INVOKABLE qint64 create(const QString &nom);

    /**
     * @brief enregistre la playlist
     * @param uuid
     * @param tracks la liste des titres
     */
    bool write(const qint64 uuid,const QList<MetaData *> tracks);

    /**
     * @brief writeAsync
     * @param uuid
     * @param tracks
     * @return
     */
    bool writeAsync(const qint64 uuid,const QList<MetaData *> tracks);

    /**
     * @brief enregistre la playlist
     * @param uuid
     * @param tracks la liste des titres
     */
    Q_INVOKABLE bool writeAsync(const qint64 uuid,const QList<QString> tracks);

    /**
      * @brief ajoute les titres à la playlist pointé par uuid
      * cette fonction a vocation à être appelé depuis le navigateur
      * donc tracks est des vurl
      * @param uuid : la playlist
      * @param tracks : la listes des pistes
      */
    Q_INVOKABLE bool add(const qint64 uuid, const QList<QString> tracks);

    /**
      * @brief version non blocante d'add
      * resultat sur le signal tracksChanged
      * @param uuid
      * @param tracks la liste des titres
      * @return false sur erreur, true si lance l'ajout + signal
      */
    Q_INVOKABLE bool addAsync(const qint64 uuid, const QList<QString> tracks);

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
    Q_INVOKABLE bool rename(const qint64 uuid,const QString &nom);

private:
    explicit playlistService(QObject *parent = 0);

    QList<Pls *> liste;

    QString repPath;

    /**
     * @brief refresh la liste des playlist
     */
    void refresh();

    /**
     * @brief fonction app par le qfuture
     * @param uuid
     */
    static void doOpen(const qint64 uuid);

    /**
     * @brief doWrite
     * @param tracks
     */
    static void doWrite(const qint64 uuid, const QList<MetaData *> tracks);

    /**
     * @brief fonction app par le qfuture
     * @param uuid
     */
    static void doAdd(const qint64 uuid, const QList<QString> tracks);

    /**
     * @brief retourne l'index dans la liste correspondant a l'uuid ou -1
     * @param uuid
     * @return
     */
    qint64 findIndex( const qint64 uuid) const;


    /**
     * @brief sortList
     */
    void sortList();

    /**
     * @brief alphaOrder
     * @param a
     * @param b
     * @return
     */
    static bool alphaOrder(  Pls * a, Pls  * b );


    /**
     * @brief retourne le repertoire ou sont les playlists
     * @return
     */
    static QString getPlaylistLocation();

    /**
     * @brief retourne tous les repertoires contenant des playlists (/Music/playlist + emplacements existants)
     * @return
     */
    static QStringList getPlaylistLocations();

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

private slots:
    void finaliseWrite(const qint64 uuid, const QStringList &chemins, const QVariantList &durees);
    void notifierAdd(int count, const QString &nom);

public slots:
    // suppression de l'url ( et de tout les childs)
    void onRemoved(QString vurl);

    void onMoved(QString vurl, QString as);

    void onRenamed(QString vurl, QString name);

    void onSourceChanged();
};

#endif // PLAYLIST_H
