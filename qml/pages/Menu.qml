import QtQuick 2.2
import Sailfish.Silica 1.0
import "../components"
import "../hints"

Page {
    id:page
    x:0
    y:0
    width:  Screen.width
    height: Screen.height
    property bool active: status===PageStatus.Active

    property Item contenu;

    function afficherMenu() {

        if(page.contenu) {
            page.contenu.destroy();
        }

        var ctag = Qt.createComponent(
                    page.orientation === Orientation.Portrait ||
                    page.orientation === Orientation.PortraitInverted ?
                        "../components/menu/MenuListeVertical.qml" :
                        "../components/menu/MenuGrilleHorizontal.qml");
        page.contenu  = ctag.createObject(conteneur, {model:listeModel});

    }

    ListModel {
        id: listeModel

        ListElement {
            icone:"image://theme/icon-m-folder"
            t:"topmenu.navigateur"
            d:"topmenu.navigateur.desc"
            c:"Navigateur.qml"
        }
        ListElement {
            icone:"image://theme/icon-m-menu"
            t:"topmenu.playlist"
            d:"topmenu.playlist.desc"
            c:"Playlists.qml"
        }

        ListElement {
            icone:"image://theme/icon-m-favorite"
            t:"topmenu.favoris"
            d:"topmenu.favoris.desc"
            c:"Favoris.qml"
        }

        ListElement {
            icone:"image://muuzik/img/historique"
            t:"topmenu.historique"
            d:"topmenu.historique.desc"
            c:"Historique.qml"
        }

        ListElement {
            icone:"image://muuzik/img/album"
            t:"topmenu.recent"
            d:"topmenu.recent.desc"
            c:"Recent.qml"
        }

        ListElement {
            icone:"image://theme/icon-m-search"
            t:"topmenu.recherche"
            d:"topmenu.recherche.desc"
            c:"Rechercher.qml"
        }

        ListElement {
            icone:"image://theme/icon-m-question"
            t:"topmenu.aleatoire"
            d:"topmenu.aleatoire.desc"
            c:"Aleatoire.qml"
        }
        ListElement {
            icone:"image://muuzik/img/son"
            t:"topmenu.enlecture"
            d:"topmenu.enlecture.desc"
            c:"ListeLecture.qml"
        }
    }

    SilicaFlickable {
        id: flickable
        width: parent.width
        height: parent.height
        anchors.fill: parent
        //       contentHeight: mainColumn.height
        contentHeight: parent.height
        contentWidth: parent.width

        VerticalScrollDecorator {  }

        Item {
            id: conteneur
            anchors.fill: parent
        }

        PullDownMenu {
            MenuItem {
                text:  vi18n.get("menu.apropos")
                onClicked: pageStack.push(Qt.resolvedUrl("Apropos.qml"))
            }
            MenuItem {
                text:  vi18n.get("menu.config")
                onClicked: {pageStack.push(Qt.resolvedUrl("Configuration.qml"));}
            }
            MenuItem {
                visible: modeNuitService.actif===true
                text:  vi18n.get("menu.nuit.off")
                onClicked: {
                    modeNuitService.arreter();
                }
            }
        }
    }

    onStatusChanged: {
        if(ConfigService.getNiveauConfigOk()===false && status===PageStatus.Active) {
            var dlg = pageStack.push(Qt.resolvedUrl("../components/DialogRedirectConfig.qml") );
        }
    }

    Component.onCompleted: {
        afficherMenu();
    }

    onOrientationChanged: {
        afficherMenu();
    }
}
