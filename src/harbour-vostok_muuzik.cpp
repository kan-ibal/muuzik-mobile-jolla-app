/*
  Copyright (C) 2016 Vostoksystem
  Contact: jerome levreau <levreau.jerome@vostoksystem.eu>
  All rights reserved.

  You may use this file under the terms of BSD license as follows:

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the Jolla Ltd nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE FOR
  ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
  ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

//#ifdef QT_QML_DEBUG
#include <QtQuick>
//#endif

#include <QCoreApplication>
#include <QMetaType>
//#include <QtWebView>
#include <sailfishapp.h>

#include "vi18n/vi18n.h"
#include "sdcardlocator/sdcardlocator.h"
#include "iconprovider/iconprovider.h"
#include "virtualfiles/VFservice.h"
#include "qnetworkwatcher/qnetworkwatcher.h"
#include "qdownloader/qdownloaderservice.h"

#include "service/config.h"
//#include "service/navigateur.h"
#include "service/lecteur.h"
#include "service/playlist.h"
#include "service/favoris.h"
#include "service/infos.h"
#include "service/parole.h"
#include "service/lastfm.h"
#include "service/artwork.h"
#include "service/indexeur.h"
#include "service/recherche.h"
#include "service/historique.h"
#include "service/modenuit.h"
#include "service/pulse/volumeProxy.h"
#include "service/recent.h"
#include "service/metadataservice.h"

#include "bean/listeitem.h"
#include "bean/resultitem.h"

#include "util/rowhistorique.h"
#include "util/rowindexeur.h"

#include <QRegExp>
#include <qdebug.h>

#include "app.h"
//#include "opusfile/opusfile.h"

#include "util/db.h"

int main(int argc, char *argv[])
{
    qRegisterMetaType<QStringPtrListe>( "QStringPtrListe" );
    qRegisterMetaType<QStringListe>( "QStringListe" );
    qRegisterMetaTypeStreamOperators<QList<QString> >("QList<QString>");

    qmlRegisterType<ListeItem>("UuidListeItem", 1, 0, "UuidListeItem");
    qmlRegisterType<ResultItem>( "ResultItem", 1, 0, "ResultItem" );
    qRegisterMetaType<ResultItemListe>( "ResultItemListe" );

    QScopedPointer<QGuiApplication> app(SailfishApp::application(argc, argv));
    //QtWebView::initialize();
    app->setOrganizationName(QStringLiteral("com.vostoksystem"));
    app->setApplicationName(QStringLiteral("Muuzik!"));
    app->setApplicationVersion(QString(APP_VERSION));

    QScopedPointer<QQuickView> view(SailfishApp::createView());

    // ini tialise la base du système virtuel + config pour accès sdcard
    VFService::init(view->rootContext());
    SdcardLocator::init(view->rootContext());
    vI18n::init(view->rootContext());
    IconProvider::init(view.data(),"muuzik");
    qnetworkwatcher::init(view->rootContext());
    QDownloaderService::init(view->rootContext());
    VolumeProxy::init(view->rootContext());

    // on charge la config
    configService::init(view->rootContext());

    // set the sources path for vfi
    VFI.setSources(configService::getInstance().getSource());

    // NOTA : il faut mettre à jour les source VFI avant de lancer
    // les autre service car certain chargent depende de VFI au demarrage
    configService::init(view->rootContext());
    ArtWorkService::init(view->rootContext());
    MetaDataService::init(view->rootContext());
    favorisService::init(view->rootContext());
    HistoriqueService::init(view->rootContext());
    IndexeurService::init(view->rootContext());
    rechercheService::init(view->rootContext());
    recentService::init(view->rootContext());
    paroleService::init(view->rootContext());
    playlistService::init(view->rootContext());
    lecteurService::init(view->rootContext());
    modeNuitService::init(view->rootContext());
    lastfmService::init(view->rootContext());

    //VI18N.setLanguage("de");

//    VFStat *stat = VFI.getStatSynch("/Rock", VFService::FILTERAUDIO);
//    qDebug() << "STAT : " << stat->getFileCount() << " " << stat->getFileSize();


    // set main pages and start
    view->setSource(SailfishApp::pathTo("qml/harbour-vostok_muuzik.qml"));
    //view->show();
    view->showFullScreen();

    return app->exec();
}
