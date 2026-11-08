#include "VFservice.h"
#include <QtQml>
#include <QMetaType>
#include <QQmlEngine>
#include <QFileInfo>
#include <QDir>
#include <QRegularExpression>
#include <QRegExp>
#include <QDateTime>
#include <QFuture>
#include <QtConcurrent/QtConcurrentRun>
#include <stdio.h>
#include <utime.h>

static QRegularExpression REG_parent( "(.*)/[^/]+$");
static QRegularExpression REG_hasparent( "/[^/]+");


static QRegularExpression REG_name("([^/]+)$");
static QRegularExpression REG_sname( "([^/]*)\\.[^\\.]+$");




static quint64 batch_id = 0;

const QString VFService::FILTERNONE = QString("");
const QString VFService::FILTERAUDIO = QString("\\.(mp3|ogg|flac|m4a|m4b|wav|aif|aiff|opus|alac)$");
const QString VFService::FILTERVIDEO = QString("\\.(mkv[avi|mp4|m4v|mpg|mov|3gp)$");
const QString VFService::FILTERIMAGE = QString("\\.(jpg|jpeg|png|gif)$");
const QString VFService::FILTERTEXT = QString("\\.(txt|text)$");
const QString VFService::FILTERDOCUMENT = QString("\\.(txt|text|doc|docx|xls|pps|ppsx|odt|ods|odp|pdf)$");
const QString VFService::FILTERPDF = QString("\\.(pdf)$");


quint64 getBatchId() {
    return ++batch_id;
}

/**
 * @brief VFService::VFService
 * @param parent
 */
VFService::VFService(QObject *parent) : QObject(parent), hidden(false){
    srcs.append(QString(QDir::homePath()));

    setHidden(false);
}

/**
 * @brief instance (singleton) for the service
 * @return
 */
VFService& VFService::getInstance() {
    static VFService instance;
    return instance;
}

/**
 * @brief register the service into the context so it can be used in qml
 * @param context
 * @param name : register service on this name, default to VFSERVICE_QML_NAME
 */
void VFService::init(QQmlContext *context, QString name) {
    context->setContextProperty(name,&VFService::getInstance());
    qmlRegisterType<VFInfo>("org.vostok.virtualfiles", 1, 0, "VFInfo");
    qRegisterMetaType<QList<VFInfo*> >( "QList<VFInfo*>" );
    qRegisterMetaType<QList<VFInfo*> *>( "QList<VFInfo*>*" );

    qmlRegisterType<VFStat>("org.vostok.virtualfiles", 1, 0, "VFStat");

    qmlRegisterUncreatableType<VFService>("org.vostok.virtualfiles", 1, 0, "VFEnum","");
}

/*************************************************************/

/**
 * @brief add a new source location (absolute path, directory)
 * @param src : path to the directory
 * @return true if successfully added (exit)
 */
bool VFService::addSource(const QString &path) {
    qDebug()  << "VFI, adding source : " << path;

    QFileInfo info(path);
    if(info.exists() == false || info.isDir() == false) {
        qDebug() << "VFI, path \"" << path << "\" is invalid";
        return false;
    }

    // prevent malformed path
    QString str = info.absoluteFilePath();

    // already exist, still return true because path parts of the list
    if(srcs.contains(str)) {
        return true;
    }

    // check if "path" is already defined by a higher path
    foreach (QString src, srcs) {
        if(str.startsWith(src)) {
            // already got something, still return true because it will be a match
            qDebug() << "VFI, path \"" << path << "\" is a sub directory, skip";
            return true;
        }
    }

    // remove all sub directory of "path" (because it takes now priority)
    QList<QString> tmp = QList<QString>(srcs);
    foreach (QString src, tmp) {
        if(src.startsWith(str)) {
            srcs.removeAll(src);
        }
    }

    srcs.append(str);

    qDebug() << "VFI, path \"" << path << "\" added";

    return true;
}

/**
 * @brief remove a source from the system
 * @param path to the directory (absolute path)
 * @return
 */
void VFService::removeSource(const QString &path) {
    QFileInfo info(path);
    QString str = info.absoluteFilePath();

    srcs.removeAll(str);
}

/**
 * @brief replace all sources with a new list
 * @param paths
 * @return true if all source set, else false
 */
bool VFService::setSources(const QList<QString> &paths) {

    bool res = true;

    srcs.clear();

    foreach (QString str,paths) {
        res &= addSource(str);
    }

    // fail safe, at least one valid source
    if(srcs.length()==0) {
        srcs.append(QString(QDir::homePath()));
        return false;
    }

    return res;
}

/**
 * @brief return true if path parts of the source
 * @param path
 * @return
 */
bool VFService::SourceExist(const QString &path) {
    QFileInfo info(path);
    QString str = info.absoluteFilePath();

    return srcs.contains(str);
}

/*************************************************************
 * control
*************************************************************/

/**
 * @brief define if hidden files and dir (start with a dot ) must by listed, default to false
 * @param show : true, include in "get" else skipped
 */
void VFService::setHidden(bool val) {
    hidden = val;

    _filter = QDir::Dirs|QDir::Files|QDir::Readable|QDir::NoDotAndDotDot;
    _filter_d = QDir::Dirs|QDir::Readable|QDir::NoDotAndDotDot;
    _filter_f = QDir::Files|QDir::Readable|QDir::NoDotAndDotDot;

    if(hidden) {
        _filter |= QDir::Hidden;
        _filter_d |= QDir::Hidden;
        _filter_f |= QDir::Hidden;
    }
}

/*************************************************************/

/**
 * @brief check if resource exist AND is readable
 * @param uri
 * @return true if exist
 */
bool VFService::exist(const QString &uri) {
    if(uri.isEmpty()) {
        return false;
    }
    foreach (QString str, srcs) {
        QFileInfo info(str + uri);

        if( info.exists() && info.isReadable()) {
            return true;
        }
    }

    return false;
}

/**
 * @brief check if can write into directory or override / create file
 * @param uri
 * @return
 */
bool VFService::isWritable(const QString &uri) {

    foreach (QString str, srcs) {
        QFileInfo info(str + uri);

        if(info.exists()) {
            return info.isWritable();
        }
    }

    // future resource, check for parent
    QString parent = getParentVUrl(uri);
    foreach (QString str, srcs) {
        QFileInfo info(str + parent);

        if(info.exists()) {
            return info.isWritable();
        }
    }

    return false;
}

/**
 * @brief check if uri is a valid directory
 * @param uri
 * @return
 */
bool VFService::isDirectory(const QString &uri) {

    foreach (QString str, srcs) {
        QFileInfo info(str + uri);

        if(info.exists()) {
            return info.isDir();
        }
    }

    return false;
}

/**
 * @brief check if uri is a valid file
 * @param uri
 * @return
 */
bool VFService::isFile(const QString &uri) {

    foreach (QString str, srcs) {
        QFileInfo info(str + uri);

        if(info.exists()) {
            return info.isFile();
        }
    }

    return false;
}

/**
 * @brief check if uri at root of vfs
 * @param uri
 * @return true if root, esle false
 */
bool VFService::isRoot(const QString &uri) {
    return uri.compare("/") == 0;
}

/**
 * @brief check if "parent" is a parent directory of "uri" (direct or indirect)
 * @param parent
 * @param uri
 * @return
 */
bool VFService::isParent(const QString &parent, const QString &uri) {

    QVector<QStringRef> p = parent.splitRef('/',QString::SkipEmptyParts);
    QVector<QStringRef> u = uri.splitRef('/',QString::SkipEmptyParts);

    // for uri to be child of parent it has to have at least 1 more block
    if(p.length()>=u.length()) {
        // because parent can't be wider than its child
        return false;
    }

    // now compare each bock until end of 'parent' of got some difference
    for(quint16 i = 0 ; i < p.length() ; i++ ) {
        if( p.at(i).compare(u.at(i))!=0) {
            return false;
        }
    }

    // all previous block are the same and there at least one more, uri is a child of parent
    return true;
}

/**
 * @brief return the parent for uri (until root e.i. "/")
 * @param uri
 * @return
 */
QString VFService::getParentVUrl(const QString &uri) {
    QRegularExpressionMatch m = REG_parent.match(uri);
    if(m.hasMatch()==false) {
        return "/";
    }

    return m.captured(1).length() == 0 ? "/" : m.captured(1);
}

/**
  * @brief check if vurl is a direct child of root
  * @param vurl
  * @return true if more parent or false if already child of root
  * @since 1.4
  */
bool VFService::hasParent(const QString &vurl) {

    QRegularExpressionMatchIterator m = REG_hasparent.globalMatch(vurl);

    quint16 nb = 0;
    while (m.hasNext()) {
        nb++;
        m.next();
    }

    return nb>1;
}

/**
 * @brief create an uri from an ABSOLUTE file path
 * @param path
 * @return
 */
QString VFService::getVUrl(const QString &path) {

    foreach (QString str, srcs) {

        if(path.startsWith(str)) {
            QString chemin(path);

            chemin.replace(str,"");
            if(chemin.length()==0) {
                return "/";
            }

            return chemin;
        }
    }

    return QString::null;
}

/**
 * @brief return corresponding source i.e. the mount point into the vfs for this uri
 * into the system file system
 * @param uri
 * @return  the source path or QString::null
 */
QString VFService::getSourceFromVUrl(const QString &uri) {
    foreach (QString str, srcs) {
        QFileInfo info(str + uri);

        if(info.exists() && info.isReadable()) {
            return str;
        }

    }
    return QString::null;
}

/**
 * @brief return the absolute path to the uri
 * @param uri must exist and be readable
 * @return either the absolute path or empty string
 */
QString VFService::getPathFromVUrl(const QString &uri) {

    foreach (QString str, srcs) {
        QFileInfo info(str + uri);

        if(info.exists()&& info.isReadable()) {
            return info.absoluteFilePath();
        }

    }

    return QString::null;
}

/**
 * @brief instead of returning the first valid path from vurl, return an
	 * array of all existing (usefull for rep)
 * @param uri must exist and be readable
 * @return either the absolute path or empty string
 */
QStringList VFService::getPathSFromVUrl(const QString &uri) {

	QStringList res;
	
    foreach (QString str, srcs) {
        QFileInfo info(str + uri);

        if(info.exists()&& info.isReadable()) {
			res.append( info.absoluteFilePath() );
        }
    }

    return res;
}

/**
 * @brief return corresponding source i.e. the mount point into the vfs for this absolute
 * file path
 * @param path : an absolute path (NOT uri)
 * @return the source path or QString::null
 */
QString VFService::getSourceFromPath(const QString &path) {
    foreach (QString str, srcs) {
        if(path.startsWith(str)) {
            return str;
        }
    }
    return QString::null;
}

/**
 * @brief return the name of a file/rep (including extention) from an uri
 * @param uri
 * @return
 */
QString VFService::getFileName(const QString &uri) {
    QRegularExpressionMatch m = REG_name.match(uri);

    if(m.hasMatch()) {
        return m.captured(1);
    }

    return QString("");
}

/**
 * @brief this method strip path and file extension from an uri or path
 * @param uri
 * @return
 */
QString VFService::getName(const QString &uri) {
    QRegularExpressionMatch m = REG_sname.match(uri);
    if(m.hasMatch()==false) {
        return getFileName(uri);
    }

    QString tmp = QString(m.captured(1));
    if(tmp.length()==0) {
        // special case when file is hidden and has no extension
        return getFileName(uri);
    }

    return tmp;
}

/**
  * @brief convenience method to build a proper vurl
  * @param parent vurl
  * @param child
  * @return well formated vurl
  */
QString VFService::mkVUrl(const QString &parent, const QString &child) {
    static QRegularExpression sec("/+");

    //@TODO : calculer les paths relatifs
    QString tmp(parent + "/" + child);
    return tmp.replace(sec, "/");
}

/*************************************************************/

/**
 * @brief return information for this uri (path into the vfs)
 * @param path
 * @return
 */
VFInfo *VFService::getInfo(const QString &uri) {

    foreach (QString str, srcs) {
        QFileInfo info(str + uri);

        VFInfo *vf = getInfo_imp(info);
        if( vf == NULL) {
            continue;
        }

        vf->vurl = uri;
        vf->source = str;
        return vf;
    }

    return NULL;
}

/**
 * @brief return inforamtion / convert but from a qfileinfo
 * @param f
 * @return
 */
VFInfo *VFService::getInfo(QFileInfo &f) {
    VFInfo *vf = getInfo_imp(f);
    if(vf == NULL) {
        return NULL;
    }

    vf->vurl = getVUrl(f.absoluteFilePath());
    vf->source = getSourceFromPath(f.absoluteFilePath());

    return vf;
}

/**
 * @brief internal use ; VFInfo is incomplete
 * @param f
 * @param uri
 * @return
 */
VFInfo *VFService::getInfo_imp(QFileInfo &f) {
    if( f.exists() == false || f.isReadable() == false) {
        return NULL;
    }

    VFInfo *info = new VFInfo();
    if(f.isDir()) {
        info->type = VFInfo::DIRECTORY;
    } else {
        info->type = VFInfo::FILE;
    }

    info->name = getName(f.absoluteFilePath());
    info->filename = f.fileName();

    info->grouppid = f.groupId();
    info->userid  = f.ownerId();
    info->timestamp = f.lastModified().toMSecsSinceEpoch();
    info->size = f.size();

    return info;
}

/*************************************************************/

/**
 * @brief start scanning for files/directories from this uri
 * This is a asynchronious operation, list will be available from signal listAvailable
 *
 * filter is a regexp apply to files name ONLY, never to directory (purpose is to be a file type extension) even if type = VFType::DIR
 * filter is allways case insensitive
 * nota : non readable file and directory got skipped
 *
 * @param uri : source for file
 * @param filter : optionnal regular expression for file name (extension) ; see FILTERAUDIO for example
 * @param type : what to look for
 * @param order : ordering to apply on the list (directory will still be listed first alpha)
 * @return a batch id to identify this scan on the signal callback
 */
quint64 VFService::get(const QString &vurl, const QString &filter, VFType type, VFOrder order) {
    quint64 id = getBatchId();
    QFuture<void> f = QtConcurrent::run(doGet, id, vurl, filter, type, order );
    return id;
}

/**
 * @brief qtconcurrent worker
 * @param uri
 * @param filter
 * @param type
 * @param order
 */
void VFService::doGet(quint64 id, const QString &vurl, const QString &filter,VFType type, VFOrder order) {
    QList<VFInfo *> list = VFI.getSynch(vurl, filter, type, order );

    QVariantList v;
    foreach (VFInfo *t, list) {
        v.append(QVariant::fromValue(t));
    }

    emit VFI.listAvailable(id, vurl, v);
}

/**
 * @brief this is a synchronous - blocking function - of "get"
 * @param uri
 * @param filter
 * @param type
 * @return
 */
QList<VFInfo *> VFService::getSynch(const QString &vurl, const QString &filter, VFType type, VFOrder order) {

    QRegularExpression *exp = NULL;
    if(filter.length()>0) {
        exp = new QRegularExpression(filter, QRegularExpression::CaseInsensitiveOption);
        if(exp->isValid()==false) {
            delete(exp);
            exp = NULL;
        }
    }

    // recup les elements
    QList<VFInfo *> tmp;
    foreach (QString str, srcs) {
        tmp.append(doGetScan(str, vurl, exp, type));
    }

    QList<VFInfo *> list;
    QList<QString> hash;

    // on filtre les doublons
    foreach (VFInfo *item, tmp) {
        if(hash.contains(item->getVUrl())) {
            delete item;
            continue;
        }
        list.append(item);
        hash.append(item->getVUrl());
    }

    if(exp != NULL) {
        delete(exp);
    }

    switch (order) {
    case VFOrder::ALPHA:
        qSort(list.begin(), list.end(), VFInfo::orderByName);
        break;
    case VFOrder::DATE:
        qSort(list.begin(), list.end(), VFInfo::orderByDate);
        break;

    case VFOrder::SIZE:
        qSort(list.begin(), list.end(), VFInfo::orderBySize);
        break;

    case VFOrder::URI:
        qSort(list.begin(), list.end(), VFInfo::orderByUri);
        break;

    default:
        break;
    }

    return list;
}

/**
 * @brief VFService::doScan
 * @param base
 * @param uri
 * @param exp
 * @param type
 * @return
 */
QList<VFInfo *> VFService::doGetScan(QString &base, QString vurl, QRegularExpression *exp, VFType type ) {

    QList<VFInfo *> list;

    // this hack prevent double // in uri
    QString separator = isRoot(vurl) ? "": "/";

    QDir dir(base + vurl);
    dir.setFilter(_filter);
    QFileInfoList li = dir.entryInfoList();

    foreach (QFileInfo f, li ) {
        // if a dir and recursive, go to lower directory first
        if( f.isDir() && type == VFType::RECURSIVE ) {
            list << doGetScan(base, vurl + separator + f.fileName(), exp,type);
            continue;
        }

        // only want files
        if(type == VFType::FILE && f.isDir()) {
            continue;
        }

        // only want directory
        if(type == VFType::DIR && f.isFile()) {
            continue;
        }

        // check if should do a filenma match
        if(exp != NULL && f.isFile() && exp->match(f.fileName()).hasMatch() == false) {
            continue;
        }

        // either filename match or a dir on a not recursive mode, let's add
        VFInfo *vf = getInfo_imp(f);
        if(vf != NULL) {
            vf->vurl = vurl + separator + f.fileName();
            vf->source = base;
            list.append(vf);
        }
    }

    return list;
}

/*************************************************************
 * stat
*************************************************************/

/**
 * @brief return the number of directory (one level down) from this uri (to a directory !)
 * only count readable entries
 * @param uri
 * @return
 */
quint16 VFService::countDirectories(const QString &uri, const QString &filter) {
    qint64 nb=0;
    QRegularExpression exp = QRegularExpression(filter, QRegularExpression::CaseInsensitiveOption);

    foreach (QString str, srcs) {
        QDir dir(str + uri);
        if(dir.isReadable() == false ) {
            continue;
        }

        dir.setFilter(_filter_d);
        QStringList l = dir.entryList();
        if(filter.length() >0 && exp.isValid()) {
            foreach (QString f, l) {
                if(exp.match(f).hasMatch()) {
                    nb++;
                }
            }
        } else {
            nb += l.length();
        }
    }

    return nb;
}

/**
 * @brief return the number files into this directory
 * only count readable entries
 * @param uri
 * @param filter : optional filter
 * @return
 */
quint16 VFService::countFiles(const QString &uri, const QString &filter) {
    qint64 nb=0;
    QRegularExpression exp = QRegularExpression(filter, QRegularExpression::CaseInsensitiveOption);

    foreach (QString str, srcs) {
        QDir dir(str + uri);
        if(dir.isReadable() == false ) {
            continue;
        }

        dir.setFilter(_filter_f);
        QStringList l = dir.entryList();
        if(filter.length() >0 && exp.isValid()) {
            foreach (QString f, l) {
                if(exp.match(f).hasMatch()) {
                    nb++;
                }
            }
        } else {
            nb += l.length();
        }
    }

    return nb;
}

/**
 * @brief check if a vurl match  a filter
 * @param vurl
 * @param filter; ex FILTERAUDIO, etc...
 * @return true if match, else false
 */
bool VFService::VurlMatch(const QString &vurl,const QString &filter) {
    QRegularExpression exp = QRegularExpression(filter, QRegularExpression::CaseInsensitiveOption);

    return exp.match(vurl).hasMatch();
}

/**
 * @brief return statistic from a directory, recursively. If vurl refer to a file
 * just convert a vfinfo into vfstat
 * @param vurl : base url
 * @param filter : filter to apply on file, never on directory
 * @return a batch_id, data will be available on signal statAvailable
 */
quint64 VFService::getStat(const QString &vurl, const QString &filter) {

    quint64 id = getBatchId();
    QFuture<void> f = QtConcurrent::run(doStat, id, vurl, filter );
    return id;
}

/**
 * @brief return statistic from a directory, recursively. If vurl refer to a file
 * just convert a vfinfo into vfstat
 * @param vurl : base url
 * @param filter : filter to apply on file, never on directory
 * @return stat for vurl; if vurl not found, return an empty class
 */
VFStat *VFService::getStatSynch(const QString &vurl, const QString &filter) {

    QRegularExpression *exp = NULL;
    if(filter.length()>0) {
        exp = new QRegularExpression(filter, QRegularExpression::CaseInsensitiveOption);
        if(exp->isValid()==false) {
            delete(exp);
            exp = NULL;
        }
    }

    VFStat *stat = new VFStat();
    stat->vurl = vurl;
    stat->filter = filter;

    foreach (QString str, srcs) {
        doStatScan(stat, str,vurl,exp);
        //     stat.dirCount  += tmp.dirCount;
        //    stat.fileSize += tmp.fileSize;
        //    stat.fileCount += tmp.fileCount;
    }

    if(exp != NULL) {
        delete exp;
        exp = NULL;
    }

    return stat;
}

/**
 * @brief qtconcurrent worker
 * @param vurl
 * @param filter
 */
void VFService::doStat(quint64 id, const QString &vurl, const QString &filter) {

    VFStat *stat = VFI.getStatSynch(vurl, filter);
    emit VFI.statAvailable(id, QVariant::fromValue(stat));
}

/**
 * @brief doStatScan
 * @param base
 * @param vurl
 * @param exp
 * @return
 */
void VFService::doStatScan(VFStat *stat, QString &base, QString vurl, QRegularExpression *exp) {

    // @NOTA : don't set vurl or filter, not mean to be used stand alone
    // this is doe by the caller function
    //VFStat * stat = new VFStat();

    // this hack prevent double // in uri
    QString separator = isRoot(vurl) ? "": "/";

    QDir dir(base + vurl);
    dir.setFilter(_filter);
    QFileInfoList li = dir.entryInfoList();

    foreach (QFileInfo f, li ) {
        // if a dir and recursive, go to lower directory first
        if( f.isDir() ) {
            stat->dirCount++;
            doStatScan(stat, base, vurl + separator + f.fileName(), exp);
            continue;
        }

        // check if should do a filenma match
        if(exp != NULL && exp->match(f.fileName()).hasMatch() == false) {
            continue;
        }

        if( f.isDir() ) {
            stat->dirCount++;
        } else {
            stat->fileCount++;
            stat->fileSize += f.size();
        }
    }
}

/*************************************************************
 * action
*************************************************************/

/**
 * @brief remove files from this uri  ; delete directory RECURSIVELY
 * this method also delete hidden files ans directories
 * NOTA : even if return false, some files may have been deleted
 * @param uri
 * @return true if file(s) removed, else false (likely r/w denied )
 */
bool VFService::remove(const QString &vurl) {
    bool ok = false;

    foreach (QString str, srcs) {
        QFileInfo info(str + vurl);

        if(info.isDir()) {
            QDir d(info.absoluteFilePath());
            ok = ok || d.removeRecursively();
            continue;
        }

        QFile f(info.absoluteFilePath());
        ok = ok || f.remove();
    }

    if(ok) {
        emit removed(vurl);
    }
    return ok;
}

/**
 * @brief move uri (files or directory) into a new location
 * each files / directory are moved into the same "source",
 * no files will be moved between different physical location (e.i. files
 * on a sdcard stay in the sdcard)
 * destination directories will be automatically created
 * @param uri : source
 * @param into : the new location, i.e the target PARENT directory
 * @return true if file/directory moved, else false (r/w issues, target exist)
 */
bool VFService::move(const QString &uri, const QString &into) {

    if((exist(uri)==false) || isRoot(uri)) {
        return false;
    }

    // prevent moving directory into child !
    if(VFI.isParent(uri, into)) {
        return false;
    }

    // uri exist, not root, not parent of child

    bool ok = true;

    foreach (QString str, srcs) {
        QString source(str + uri);

        // uri has to exist into this mount, else skip
        QFileInfo info(source);
        if(info.exists() == false) {
            continue;
        }

        // ok, source exist
        // build target directories
        QDir target(str + into);
        if( target.mkpath(".") == false ) {
            // can't creatre target
            return false;
        }

        QString cmd("mv -u -t " + VFService::sanitize(target.absolutePath()) + " " + VFService::sanitize(source));
        int term_status = system(cmd.toStdString().c_str());
        if( WIFEXITED( term_status ) ) {
            qDebug() << "ok dep "  << WIFEXITED( term_status );

            ok = ok && (WEXITSTATUS (term_status) != -1 ? true : false);
        } else {
            ok = false;
        }
    }

    if(ok) {
        QString as = into + "/" + getFileName(uri);
        emit moved(uri,as);
    }
    return ok;
}

/**
 * @brief rename file at "uri" as "name"
 * NOTA : if return false, no file got renamed on any locaiton
 * @param uri
 * @param name
 * @return true if successfully renamed else false (r/w error, name already exist..)
 */
bool VFService::rename(const QString &uri, const QString &name) {

    if((exist(uri)==false) || isRoot(uri) || name.isEmpty() || name.contains('/')) {
        return false;
    }

    QString p(getParentVUrl(uri));
    QString target(p + (isRoot(p) ? "" : "/") + name);

    foreach (QString str, srcs) {
        QFileInfo t(str+target);
        if(t.exists()) {
            return false;
        }
    }

    // target doesn't exist, we can now rename safely
    QString old = getFileName(uri);
    bool res = false;
    foreach (QString str, srcs) {
        QDir d(str + p);
        res = res || d.rename(old,name);
    }

    emit renamed(uri,name);
    return res;
}

/**
 * @brief change date for file or directory at vurl
 * NOTA : date will be changed on any source and action is not recursive
 * @param vurl : source file
 * @param timestamp : new date on the form of a timestamp (mill sec since 1970)
 * @return : true if successfull + emit dateChanged
 */
bool VFService::date(const QString &vurl,const quint64 timestamp) {

    if( exist(vurl) == false) {
        return false;
    }

    bool ok = true;

    foreach (QString str, srcs) {
        QFileInfo info(str+ vurl);
        if(info.exists() == false) {
            continue;
        }

        QString target(str+vurl);
        utimbuf t {
            timestamp / 1000, timestamp / 1000
        };
        int res = utime(target.toLocal8Bit(), &t);

        if(res == -1 ) {
            ok = false;
        }
    }

    if(ok) {
        emit dateChanged(vurl,timestamp);
    }

    return ok;
}

/**
 * @brief escape "'()space
 * @param val
 * @return
 */
QString VFService::sanitize(QString val) {

    //    static QRegularExpression REG_escape( "([\"\'\(\) ])");
    //qDebug() << tmp.replace(REG_escape, "\$1");

    val.replace('\'', "\\'");
    val.replace('"', "\\\"");
    val.replace('(', "\\(");
    val.replace(')', "\\)");
    val.replace(' ', "\\ ");

    return val;
}
