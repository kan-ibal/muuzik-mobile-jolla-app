import QtQuick 2.2
import Sailfish.Silica 1.0
import "../components"
import "../js/app.js" as App

Page {
    id:page
    property bool active: status===PageStatus.Active
    property bool loaded: false

    //-----------------------------------------
    function openUri(v) {
        pageStack.push(Qt.resolvedUrl("Navigateur.qml"), {uri:v})
    }

    //----------------------------------
    function ouvrir(url) {
        lecteurService.remplacer([url]);
        pageStack.push(Qt.resolvedUrl("ListeLecture.qml"));
    }

    //----------------------------------
    function ajouter(url) {
        lecteurService.ajouter([url]);
    }

    //----------------------------------
    function recharger() {
        var list = FavorisService.liste;
        var l = [];
        for(var i in list) {
            l.push({
                       uuid: list[i].uuid,
                       nom: list[i].nom,
                       chemin: list[i].chemin,
                       duree: list[i].duree,
                       pistes: list[i].pistes
                   });
        }
        listModel.clear();
        listModel.append(l);
    }

    Component {
        id: entree

        ListItem {
            id: fileItem
            menu: contextMenu
            width: ListView.view.width
            contentHeight: Math.max(img.height,description.height) +
                           (  ConfigService.marge ? Theme.paddingSmall : 0)

            Miniature {
                id: img
                anchors {
                    left: parent.left
                    leftMargin: Theme.horizontalPageMargin
                    verticalCenter: parent.verticalCenter
                }

                source: "image://muuzik/img/album?"+Theme.primaryColor
            }

            LabelPlus {
                id:description
                anchors {
                    left: img.right
                    right: parent.right
                    leftMargin: Theme.horizontalPageMargin
                    rightMargin: Theme.horizontalPageMargin
                    verticalCenter: parent.verticalCenter
                }
                text:nom
                value: duree > 0 ? vi18n.get("favoris.description").arg(App.getDuree(duree)).arg(pistes) :
                                   vi18n.get("favoris.description.simple").arg(pistes)

                elide: Text.ElideRight
                highlighted: false;
            }


            Connections {
                target: ARTService
                onArtFound : {
                    if( vurl !== chemin) {
                        return;
                    }

                    img.overlay = artPath;
                }
            }

            onClicked: {
                ouvrir(chemin);
            }

            Component.onCompleted: {
                ARTService.getArtAsync(chemin)
            }

            Component {
                id: contextMenu
                ContextMenu {
                    MenuItem {
                        text:  vi18n.get("navigateur.rep.voir")
                        onClicked: openUri(chemin)
                    }
                    MenuItem {
                        text:  vi18n.get("navigateur.rep.ecouter")
                        onClicked: ouvrir(chemin)
                    }
                    MenuItem {
                        text:  vi18n.get("favoris.ajouter")
                        onClicked: ajouter(chemin)
                    }
                    MenuItem {
                        text:  vi18n.get("favoris.supprimer")
                        onClicked: showRemorseItem(uuid)
                    }
                }
            }

            RemorseItem { id: remorse }

            ListView.onRemove: animateRemoval(fileItem)

            function showRemorseItem() {
                var idx = uuid;
                remorse.execute(fileItem, "Deleting", function() {
                    FavorisService.supprimer(idx);
                } );
            }
        }
    }

    SilicaListView {
        id: fileList
        anchors{
            top:parent.top
            left: parent.left
        }

        height: parent.height
        width: parent.width
        clip: true
        visible: loaded

        model: ListModel {
            id:listModel
        }

        VerticalScrollDecorator { flickable: fileList }

        PullDownMenu {
            MenuItem {
                text:  vi18n.get("menu.config")
                onClicked: {pageStack.push(Qt.resolvedUrl("Configuration.qml"));}
            }

            MenuItem {
                visible:   ConfigService.ordreFavoris!==0
                text:  vi18n.get("favoris.date")
                onClicked: {  ConfigService.ordreFavoris=0;}
            }
            MenuItem {
                visible:   ConfigService.ordreFavoris!==1
                text:  vi18n.get("favoris.inv")
                onClicked: {  ConfigService.ordreFavoris=1;}
            }
            MenuItem {
                visible:   ConfigService.ordreFavoris!==2
                text:  vi18n.get("favoris.nom")
                onClicked: {  ConfigService.ordreFavoris=2;}
            }

            MenuItem {
                text:  vi18n.get("menu.enlecture")
                visible: page.orientation === Orientation.Portrait || page.orientation === Orientation.PortraitInverted
                onClicked: {
                    pageStack.push(Qt.resolvedUrl("ListeLecture.qml"));
                }
            }
        }

        header: PageHeader {
            title: vi18n.get("favoris.titre")
        }

        delegate:entree

        ViewPlaceholder {
            enabled: loaded && listModel.count===0
            text: vi18n.get("favoris.vide")
            hintText: vi18n.get("favoris.vide.sum")
        }
    }

    Attente {

        enabled: loaded === false || status===PageStatus.Activating
    }

    onStatusChanged: {
        // wait for view to load
        if (status===PageStatus.Active && loaded === false ) {
            recharger();
            loaded = true;
        }
    }

    Connections {
        target: FavorisService
        onListeChanged: recharger()
    }
}
