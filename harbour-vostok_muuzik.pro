# NOTICE:
#
# Application name defined in TARGET has a corresponding QML filename.
# If name defined in TARGET is changed, the following needs to be done
# to match new name:
#   - corresponding QML filename must be changed
#   - desktop icon filename must be changed
#   - desktop filename must be changed
#   - icon definition filename in desktop file must be changed
#   - translation filenames have to be changed

# The name of your application
TARGET = harbour-vostok_muuzik

CONFIG += sailfishapp taglib
PKGCONFIG += nemonotifications-qt5 taglib ogg qt5embedwidget
QT += multimedia dbus sql
# webview
#LIBS += -ltag -lnemonotifications-qt5 -lopus -lFLAC++ -logg
LIBS += -ltag -lnemonotifications-qt5 -logg

QMAKE_CFLAGS = -Wno-unused-parameter
QMAKE_CXXFLAGS_WARN_ON += -Wno-unused-parameter

DEFINES += APP_VERSION=\"\\\"$${VERSION}\\\"\"
DEFINES += APP_COPYRIGHT=\"\\\"$${COPYRIGHT}\\\"\"
DEFINES += APP_NAME=\"\\\"$${TARGET}\\\"\"

include(src/vi18n/vi18n.pri)
include(src/sdcardlocator/sdcardlocator.pri)
include(src/iconprovider/iconprovider.pri)
include(src/virtualfiles/virtualfiles.pri)
include(src/qdownloader/qdownloader.pri)
include(src/qnetworkwatcher/qnetworkwatcher.pri)
#include(src/opusfile/opusfile.pri)
#include(src/json/qt-json.pri)

SOURCES += \
    src/harbour-vostok_muuzik.cpp \
    src/app.cpp \
    src/service/config.cpp \
    src/service/playlist.cpp \
    src/bean/pls.cpp \
    src/service/lecteur.cpp \
    src/service/favoris.cpp \
    src/service/recherche.cpp \
    src/service/infos.cpp \
    src/service/parole.cpp \
    src/service/lastfm.cpp \
    src/bean/resultitem.cpp \
    src/service/mpris/mpris.cpp \
    src/service/mpris/mprislecteur.cpp \
    src/service/modenuit.cpp \
    src/service/historique.cpp \
    src/util/db.cpp \
    src/util/rowhistorique.cpp \
    src/util/daohistorique.cpp \
    src/service/pulse/volumeProxy.cpp \
    src/service/recent.cpp \
    src/service/indexeur.cpp \
    src/util/daoindexeur.cpp \
    src/util/rowindexeur.cpp \
    src/bean/uuiditem.cpp \
    src/bean/listeitem.cpp \
    src/service/metadataservice.cpp \
    src/bean/MetaData.cpp \
    src/service/artwork.cpp \
    src/service/manager/plsmanager.cpp \
    src/testjson.cpp \
    src/util/daofavoris.cpp \
    src/util/rowfavoris.cpp \
    src/bean/albumstat.cpp \
    src/bean/track.cpp


HEADERS += \
    src/app.h \
    src/service/config.h \
    src/service/playlist.h \
    src/bean/pls.h \
    src/service/lecteur.h \
    src/service/favoris.h \
    src/service/recherche.h \
    src/service/infos.h \
    src/service/parole.h \
    src/service/lastfm.h \
    src/bean/resultitem.h \
    src/service/mpris/mpris.h \
    src/service/mpris/mprislecteur.h \
    src/service/modenuit.h \
    src/service/historique.h \
    src/util/db.h \
    src/util/rowhistorique.h \
    src/util/daohistorique.h \
    src/service/pulse/volumeProxy.h \
    src/service/recent.h \
    src/service/indexeur.h \
    src/util/daoindexeur.h \
    src/util/rowindexeur.h \
    src/bean/uuiditem.h \
    src/bean/listeitem.h \
    src/service/metadataservice.h \
    src/bean/MetaData.h \
    src/service/artwork.h \
    src/service/manager/plsmanager.h \
    src/testjson.h \
    src/util/daofavoris.h \
    src/util/rowfavoris.h \
    src/bean/albumstat.h \
    src/bean/track.h

OTHER_FILES += qml/harbour-vostok_muuzik.qml \
    qml/cover/*.qml \
    qml/pages/*.qml \
    qml/components/*.qml \
    qml/hints/*.qml \
    qml/js/*.js \
    rpm/harbour-vostok_muuzik.changes \
    rpm/harbour-vostok_muuzik.spec \
    rpm/harbour-vostok_muuzik.yaml \
    harbour-vostok_muuzik.desktop

RESOURCES += harbour-vostok_muuzik.qrc

SAILFISHAPP_ICONS = 86x86 108x108 128x128 256x256

tag.files = lib
tag.path = /usr/share/$${TARGET}
INSTALLS += tag

DISTFILES += \
    harbour-vostok_muuzik.qrc \
    qml/components/* \
    qml/components/menu/* \
    qml/pages/* \
    qml/dlg/* \
    qml/cover/* \
    qml/hints/* \
    qml/components/lecteur/* \
    qml/js/affichage.js \
    qml/pages/InfoMedia.qml \
    qml/pages/InfoRep.qml
