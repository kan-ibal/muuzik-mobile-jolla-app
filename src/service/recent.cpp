#include "recent.h"
#include <QtQml>
#include <QMetaType>
#include <QDebug>
#include <QList>
#include <qregularexpression.h>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFuture>
#include <QtConcurrent/QtConcurrentRun>
#include <QtAlgorithms>
#include <QStack>

#include "config.h"
#include "artwork.h"

#include "../virtualfiles/VFservice.h"

/**
 * @brief classement de la liste par order de timestamp
 * @param a
 * @param b
 */
bool classerItem(recentService::Item & a, recentService::Item & b) {
    return b.timestamp < a.timestamp;
}

/**
 * @brief recentService::recentService
 * @param parent
 */
recentService::recentService(QObject *parent) : QObject(parent), raf(false) {
    qRegisterMetaType<QList<Item> >( "QList<Item>" );

    connect(this,SIGNAL(rechargementFaitIntr(QList<Item>)), this, SLOT(finRechargementImpl(QList<Item>)), Qt::QueuedConnection);

    connect(&VFService::getInstance(), SIGNAL(removed(QString)),this, SLOT(onRemoved(QString)));
    connect(&VFService::getInstance(), SIGNAL(moved(QString,QString)), this, SLOT(onMoved(QString,QString)));
    connect(&VFService::getInstance(), SIGNAL(renamed(QString,QString)), this, SLOT(onRenamed(QString,QString)));
    connect(&VFService::getInstance(), SIGNAL(dateChanged(QString,quint64)),this, SLOT(onDateChanged(QString,quint64)));

    qsrand(QDateTime::currentMSecsSinceEpoch() / 1000);
}

/**
 * @brief rechercheService::getInstance
 * @r
 */
recentService& recentService::getInstance() {
    static recentService instance;
    return instance;
}

/**
 * @brief RecentService::init
 * @param context
 * @param name
 */
void recentService::init(QQmlContext *context,QString name) {
    context->setContextProperty(name, &recentService::getInstance());
}

/**
 * @brief recentService::getListe
 * @return
 */
const QStringList recentService::getListe() {
    if( (liste.length()==0) && (raf==false) ) {
        recharger();
    }

    return  liste;
}

/**
  *  @brief rechearger la liste des albums en fonction de leurs date de publication
  */
void recentService::recharger() {

    if(raf == true) {
        return;
    }

    raf = true;
    emit enCoursChanged();

    // lancement du scan
    QFuture<void> f = QtConcurrent::run(doRechargement);
}

/**
 * @brief effectue le  scan souis qfuture
 */
void recentService::doRechargement() {

    QHash<QString, Item> hash;

    QStringList travail = recentService::recur("/");
    foreach (QString str, travail) {

        QFileInfo info(VFI.getPathFromVUrl(str));
        Item data = { str, info.lastModified().toMSecsSinceEpoch() };

        // ajoute nouveau ?
        if(hash.contains(str) == false ) {
            hash.insert(str, data);
            continue;
        }

        // met à jour ?
        if(hash.value(str).timestamp < data.timestamp) {
            hash.insert(str, data);
        }
    }

    // création de la liste "classé"
    QList<Item> l = hash.values();

    //ok, fin
    emit recentService::getInstance().rechargementFaitIntr(l);
}

/**
 * @brief boucle récursive sur les répertoires
 * @param url : url virtuelle
 * @return liste des "albums" -> url VIRTUELLE
 */
QStringList recentService::recur(const QString &vurl) {
    QStringList res;

    // est un album ? on n'ajoute pas les rep qui ont aussi des sous rep
    if(VFI.countFiles(vurl,VFService::FILTERAUDIO) >0 && VFI.countDirectories(vurl) == 0 ) {
        res.append(vurl);
    }

    QList<VFInfo *> l = VFI.getSynch(vurl, VFService::FILTERNONE, VFService::VFType::DIR);
    foreach (VFInfo *info, l) {
        res << recur(info->getVUrl());
    }

    if(l.length()>0) {
        qDeleteAll(l);
    }
    //    delete l;

    return res;
}

/**
 * @brief slot ; retourne la liste des albums vers le service
 * @param l
 */
void recentService::finRechargementImpl(const QList<Item> l ) {
    cache.clear();

    // NOTA : comme on utilise la liste recent pour le choix
    // aleatoire il ne faut pas faire de "cap" ici
    // quint16 max = configService::getInstance().getNbRecent();
    //quint16 nb = 0;

    // créé la liste des albums à partir de la liste des vurl
    foreach (Item s, l) {

        cache.append(s);
    }

    // et on construit la liste "publique"
    constuireListePublique();

    raf = false;
    emit enCoursChanged();
    emit listeChanged();
}

void recentService::constuireListePublique() {
    qSort(cache.begin(), cache.end(), classerItem);

    liste.clear();
    foreach (Item i, cache) {
        liste.append(i.vurl);
    }
}

/**
  * @brief retroune un "vrai" nombre aleatoire \ Math.random
  * ici pour eviter de créer un nouveau service
  * @param max : valeur max
  * @return val entre 0 et max
  */
quint64 recentService::random(quint64 max) {
    return qrand() % max;
}

/**
  * @brief suppression de l'url ( et de tout les childs)
  * @param vurl
  */
void recentService::onRemoved(QString vurl) {

    // on peut avoir supprimé le rep / album parent d'un favoris
    QStack<quint64> stack;

    // on recherche les index
    quint64 index = 0;
    foreach (QString s, liste) {
        if ( s.startsWith(vurl) ) {
            stack.push(index);
        }
        index++;
    }

    // on supprime en lifo
    while( stack.isEmpty() == false) {
        quint64 val = stack.pop();
        liste.removeAt(val);
    }
}

/**
  * @brief recentService::onMoved
  * @param vurl
  * @param into
  */
void recentService::onMoved(QString vurl, QString as) {

    // pour recent on se fout du renommage de fichier
    if(VFI.isDirectory(as) == false) {
        return;
    }

    for (int i = 0 ; i < liste.size() ; i++)    {
        QString s = liste.at(i);
        if ( s.startsWith(vurl) ) {
            QString target = s.replace(vurl, as);
            liste.replace(i, target);
        }
    }
}

/**
  * @brief recentService::onRenamed
  * @param vurl
  * @param name
  */
void recentService::onRenamed(QString vurl, QString name) {
    QString p = VFI.getParentVUrl(vurl);
    QString as  = p + (VFI.isRoot(p) ? "" : "/") + name;
    onMoved(vurl,as);
}

/**
 * @brief recentService::onDateChanged
 * @param vurl
 * @param timestamp
 */
void recentService::onDateChanged(QString vurl, quint64 timestamp) {
    if(raf == true) {
        return;
    }

    for(int i=0; i<cache.length(); i++) {
        if(cache[i].vurl.compare(vurl) ==0) {
            cache[i].timestamp = timestamp;
            constuireListePublique();
            emit listeChanged();
            return;
        }
    }
}

