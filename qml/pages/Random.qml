import QtQuick 2.2
import Sailfish.Silica 1.0
import "../components"
import "../js/app.js" as App

Dialog {
    id: page
    property bool active: status === PageStatus.Active
    property string albumPath: ""
    property string artist: ""
    property string album: ""
    property string info: ""
    property var cache: []

    property bool _vertical: page.orientation === Orientation.Portrait ||
                             page.orientation === Orientation.PortraitInverted

    acceptDestinationAction: PageStackAction.Replace
    acceptDestination: Qt.resolvedUrl("Playlist.qml")

    canAccept: recentService.enCours === false && albumPath.length > 0

    onAccepted: {
        console.log("load: " + albumPath)
        playerService.replace([albumPath])
    }

    //---------------------------------------
    function pickRandom() {
        // Keep a local cache to avoid picking the same album multiple times
        if (cache.length === 0) {
            // Trigger/check recent albums list if not already loaded
            var list = recentService.liste
            if (recentService.enCours || list.length === 0) {
                return
            }

            // Build the local cache
            for (var i = 0; i < list.length; i++) {
                cache.push(list[i])
            }
        }

        var index = recentService.random(cache.length)

        albumPath = cache[index]
        cache.splice(index, 1)

        var art = ARTService.getArt(albumPath)
        coverImage.source = coverImage.overlay.length > 0 && art ? coverImage.overlay : "image://muuzik/img/album?" + Theme.primaryColor
        coverImage.overlay = art ? art : "image://muuzik/img/album?" + Theme.primaryColor

        var meta = MetaDataService.getStatFromAlbumAsQVariant(albumPath)
        artist = meta.artiste
        album = meta.album
        info = ""
        if (meta.duree > 0) {
            info = vi18n.get("aleatoire.info").arg(App.getDuree(meta.duree)).arg(meta.pistes)
        }
    }

    SilicaFlickable {
        id: flickable
        anchors.fill: parent
        contentWidth: width
        contentHeight: height

        PullDownMenu {
            MenuItem {
                text: vi18n.get("menu.config")
                onClicked: { pageStack.push(Qt.resolvedUrl("Configuration.qml")) }
            }

            MenuItem {
                text: vi18n.get("menu.enlecture")
                onClicked: {
                    pageStack.push(Qt.resolvedUrl("Playlist.qml"))
                }
            }

            MenuItem {
                text: vi18n.get("menu.recharger")
                onClicked: {
                    cache = []
                    recentService.recharger()
                }
            }
        }

        DialogHeader {
            id: header
            dialog: page
            title: vi18n.get("aleatoire.titre")
        }

        Item {
            visible: recentService.enCours === false
            anchors {
                top: header.bottom
                left: parent.left
                right: parent.right
                margins: Theme.paddingLarge
            }
            height: flickable.height - header.height - Theme.paddingLarge * 2

            Miniature {
                id: coverImage
                taille: _vertical ? Math.min(parent.width * 0.8, parent.height * 0.5) : parent.height * 0.8
                anchors {
                    top: parent.top
                    topMargin: Theme.paddingLarge
                    horizontalCenter: _vertical ? parent.horizontalCenter : undefined
                }

                source: "image://muuzik/img/album?" + Theme.primaryColor
            }

            Item {
                anchors {
                    top: _vertical ? coverImage.bottom : parent.top
                    topMargin: Theme.paddingLarge
                    left: _vertical ? parent.left : coverImage.right
                    leftMargin: _vertical ? 0 : Theme.paddingLarge
                    right: parent.right
                    bottom: parent.bottom
                    bottomMargin: Theme.paddingLarge
                }

                Text {
                    id: artistText
                    width: parent.width
                    horizontalAlignment: Text.AlignHCenter
                    wrapMode: Text.Wrap
                    anchors.horizontalCenter: parent.horizontalCenter
                    color: Theme.highlightColor
                    font.pixelSize: Theme.fontSizeLarge

                    text: artist.length > 0 ? artist : App.getNomSimple(albumPath)
                }

                Text {
                    id: albumText
                    visible: album.length > 0
                    width: parent.width
                    horizontalAlignment: Text.AlignHCenter
                    wrapMode: Text.Wrap
                    anchors {
                        top: artistText.bottom
                        topMargin: Theme.paddingLarge
                        horizontalCenter: parent.horizontalCenter
                    }
                    color: Theme.highlightColor
                    font.pixelSize: Theme.fontSizeLarge

                    text: album
                }

                Text {
                    id: infoText
                    visible: info.length > 0
                    width: parent.width
                    horizontalAlignment: Text.AlignHCenter
                    wrapMode: Text.Wrap
                    anchors {
                        top: albumText.bottom
                        topMargin: Theme.paddingLarge * 1.5
                        horizontalCenter: parent.horizontalCenter
                    }
                    color: Theme.secondaryHighlightColor
                    font.pixelSize: Theme.fontSizeLarge

                    text: info
                }

                Button {
                    id: retryButton
                    text: vi18n.get("aleatoire.essai")
                    anchors {
                        horizontalCenter: parent.horizontalCenter
                        bottom: parent.bottom
                        bottomMargin: Theme.paddingLarge
                    }
                    onClicked: pickRandom()
                }
            }

            Connections {
                target: ARTService
                onArtFound: {
                    if (albumPath !== vurl) {
                        return
                    }
                    coverImage.overlay = artPath
                }
            }
        }
    }

    Attente {
        enabled: recentService.enCours || status === PageStatus.Activating
    }

    Component.onCompleted: {
        // Load an album (if database is initialized)
        pickRandom()
    }

    Connections {
        target: recentService
        onListeChanged: {
            pickRandom()
        }
    }
}
