import QtQuick 2.2
import Sailfish.Silica 1.0
import "../components"
import "../js/app.js" as App

Dialog {
    id:page
    property bool active: status===PageStatus.Active
    property string chemin : ""
    property string artiste : ""
    property string album : ""
    property string info: ""
    property var cache : [];

    property bool _vertical: page.orientation === Orientation.Portrait ||
                             page.orientation === Orientation.PortraitInverted

    acceptDestinationAction :PageStackAction.Replace
    acceptDestination: Qt.resolvedUrl("ListeLecture.qml")

    canAccept: recentService.enCours === false && chemin.length >0

    onAccepted: {
        console.log("charge : " + chemin);
        lecteurService.remplacer([chemin]);
    }

    //---------------------------------------
    function rechercher() {

        // on garde un cache local pour eviter de retirer plusieurs fois le même album
        if(cache.length ===0 ) {
            // permet de faire le rechargement de recent si pas deja fait
            var list = recentService.liste;
            if(recentService.enCours || list.length === 0 ) {
                return
            }

            // on cree le cache
            for(var i in list) {
                cache.push(list[i]);
            }
        }

  //      var i = App.getRandomInt(0, cache.length);
        var i = recentService.random(cache.length);

        chemin = cache[i];
        cache.splice(i, 1);

        var tmp = ARTService.getArt(chemin);
        img.source = img.overlay.length>0 && tmp ? img.overlay : "image://muuzik/img/album?"+Theme.primaryColor;
        img.overlay = tmp ? tmp : "image://muuzik/img/album?"+Theme.primaryColor;

        var meta = MetaDataService.getStatFromAlbumAsQVariant(chemin);
        artiste = meta.artiste;
        album = meta.album;
        info = "";
        if(meta.duree>0) {
            info = vi18n.get("aleatoire.info").arg(App.getDuree(meta.duree)).arg(meta.pistes);
        }
    }

    Item {
        visible: recentService.enCours === false

        width: parent.width
        anchors {
            fill: parent
            margins: Theme.paddingLarge
        }

        PullDownMenu {

            MenuItem {
                text:  vi18n.get("menu.config")
                onClicked: {pageStack.push(Qt.resolvedUrl("Configuration.qml"));}
            }

            MenuItem {
                text:  vi18n.get("menu.enlecture")
                onClicked: {
                    pageStack.push(Qt.resolvedUrl("ListeLecture.qml"));
                }
            }

            MenuItem {
                text:  vi18n.get("menu.recharger")
                onClicked: {
                    actuel="";
                    recentService.recharger();
                }
            }
        }

        DialogHeader {
            id:titre
        }

        Miniature {
            id: img
            taille: _vertical ? parent.width * 0.8 : (parent.height -titre.height) * 0.8
            anchors {
                top: titre.bottom
                topMargin: Theme.paddingLarge
                horizontalCenter: _vertical ? parent.horizontalCenter : undefined
            }

            source: "image://muuzik/img/album?"+Theme.primaryColor
        }

        Item {
            anchors {
                top: _vertical ? img.bottom : titre.bottom
                topMargin: Theme.paddingLarge
                left: _vertical ? parent.left : img.right
                leftMargin: _vertical ? undefined : Theme.paddingLarge
                right: parent.right
                //                horizontalCenter: _vertical ? parent.horizontalCenter : undefined
                bottom: parent.bottom
                bottomMargin: Theme.paddingLarge
            }

            Text {
                id:txt_artiste
                width: parent.width
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.Wrap
                anchors {
                    //                    top: img.bottom
                    //                    topMargin: Theme.paddingLarge
                    horizontalCenter: parent.horizontalCenter
                }
                color: Theme.highlightColor
                font.pixelSize: Theme.fontSizeLarge

                text:  artiste.length>0 ? artiste : App.getNomSimple(chemin)
            }

            Text {
                id:txt_album
                visible: album.length>0
                width: parent.width
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.Wrap
                anchors {
                    top: txt_artiste.bottom
                    topMargin: Theme.paddingLarge
                    horizontalCenter: parent.horizontalCenter
                }
                color: Theme.highlightColor
                font.pixelSize: Theme.fontSizeLarge

                text: album
            }

            Text {
                id:txt_info
                visible: info.length>0
                width: parent.width
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.Wrap
                anchors {
                    top: txt_album.bottom
                    topMargin: Theme.paddingLarge *1.5
                    horizontalCenter: parent.horizontalCenter
                }
                color: Theme.secondaryHighlightColor
                font.pixelSize: Theme.fontSizeLarge

                text: info
            }

            Button {
                id:bouton
                text:  vi18n.get("aleatoire.essai")
                anchors {
                    horizontalCenter: parent.horizontalCenter
                    bottom: parent.bottom
                    bottomMargin: Theme.paddingLarge
                }
                onClicked: rechercher()
            }
        }


        Connections {
            target: ARTService
            onArtFound : {
                if(chemin !== vurl) {
                    return;
                }
                img.overlay = artPath;
            }
        }
    }

    //    ViewPlaceholder {
    //        id:vperr
    //        enabled: recentService.enCours === false && recentService.liste.length === 0
    //        text:  vi18n.get("aleatoire.vide")
    //        hintText:  vi18n.get("aleatoire.vide.sum")
    //    }

    Attente {
        enabled: recentService.enCours || status===PageStatus.Activating
    }

    Component.onCompleted: {
        // charge un album (si db initialisé)
        rechercher();
    }

    Connections {
        target: recentService
        onListeChanged : {
            rechercher();
        }
    }
}

