#ifndef PAROLESERVICE_H
#define PAROLESERVICE_H

#include <QObject>
#include <QQmlContext>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <qqml.h>

#define PAROLESERVICE_QML_NAME "ParoleService"

class paroleService : public QObject{
    friend class QNetworkAccessManager;

    Q_OBJECT
    Q_PROPERTY(bool enCours READ estEnCours NOTIFY enCoursChanged)

public:
    static paroleService& getInstance();

    /**
     * @brief register the service into the context so it can be used in qml
     * @param context
     * @param name : register service on this name, default to PAROLESERVICE_QML_NAME
     */
    static void init(QQmlContext *context, QString name=PAROLESERVICE_QML_NAME);

    /**
     * @brief retrouve les parole pour chemin, artiste, titre
     * Si trouve en local, retourne sinon lance une requette web
     * @param chemin virtuel vers le fichier audio, utilisé pour le cache local
     * @param artiste
     * @praram titre
     * @return les texte ou vide si en recherche web
     */
    Q_INVOKABLE QString chercher(const QString & url,const QString & artiste, const QString & titre);

    /**
     * @brief status du service recherche de parole
     * @return vrai si un scan est en cours
     */
    Q_INVOKABLE bool estEnCours() const {
        return raf;
    }

    /**
     * @brief annule une requete (web) en cours
     */
    Q_INVOKABLE void annuler() {

    }

    /**
     * @brief efface la version locale
     * @param chemin virtuel vers le fichier audio
     */
    Q_INVOKABLE void effacer(const QString & url);

private:
    explicit paroleService(QObject *parent = 0);

    bool raf;
    QNetworkAccessManager *manager;

    /**
     * @brief decode une base &#xxx;
     * @return
     */
    static QString *decode(QString const);

    /**
   * @brief ecrit les paroles en local pour le fichier chemin
   * @param url virtuel vers le fichier audio
   * @param donnee
   */
    static void ecriture(QString const &url, QString const &donnee);

    /**
     * @brief charge les paroles à partir d'une version locale
     * @param chemin complet vers le fichier audio
     * @return empty( si aucune
     */
    static QString lecture(QString const &url);

    /**
     * @brief  all words starting with a uppercase letter
     * @param str
     * @param sep
     * @return
     */
    static QString versMajuscule(QString const &str, QString const &sep="_");

    /**
     * @brief all words starting with a lowercase letter
     * @param str
     * @param sep
     * @return
     */
    static QString versMinuscule(QString const &str, QString const &sep="-");

signals:

    void enCoursChanged();

    /**
     * @brief trouve des paroles par rapport à la derniere requette
     * null si pas de reponse valide
     */
    void trouve(QString donnee);

private slots:

    /**
     * @brief reponse reseau
     * @param reply
     */
    void reponse(QNetworkReply *reply);
};

#endif // PAROLESERVICE_H
