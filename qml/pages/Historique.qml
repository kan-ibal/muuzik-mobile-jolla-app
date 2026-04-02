import QtQuick 2.2
import Sailfish.Silica 1.0
import "../components"
import "../js/app.js" as App
import "../js/affichage.js" as Affichage

Page {
    id:page
    property bool active: status===PageStatus.Active
    property bool loaded: false

    //-----------------------------------------
    function openUri(v) {
        pageStack.push(Qt.resolvedUrl("Navigateur.qml"), {uri:v})
    }

    //-----------------------------------------
    function ouvrir(v) {
        lecteurService.remplacer([v]);
        pageStack.push(Qt.resolvedUrl("ListeLecture.qml"));
    }

    //-----------------------------------------
    function ajouter(v) {
        lecteurService.ajouter([v]);
    }

    //-----------------------------------------
    function recharger() {
        var list = historiqueService.liste;
        var l = [];
        for(var i in list) {
            l.push({
                       uuid: list[i].uuid,
                       nom: list[i].nom,
                       chemin: list[i].chemin,
                       nombre: list[i].nombre,
                       ouverture: list[i].ouverture,
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
            contentHeight: Math.max(img.height,titre.height) +
                           (ConfigService.marge ? Theme.paddingSmall : 0)

            Miniature {
                id: img
                anchors {
                    left: parent.left
                    leftMargin: Theme.horizontalPageMargin
                    verticalCenter: parent.verticalCenter
                }

                source: "image://muuzik/img/album?"+Theme.primaryColor
            }

            LabelNavigation {
                id:titre
                anchors {
                    left: img.right
                    leftMargin: Theme.horizontalPageMargin
                    right: parent.right
                    rightMargin: Theme.horizontalPageMargin
                    verticalCenter: parent.verticalCenter
                }
                titre:nom
                description: duree > 0 ?
                                 vi18n.get("historique.description").arg(
                                     App.getDuree(duree)).arg(pistes).arg(
                                     historiqueService.formatTime(ouverture)).arg(nombre) :
                                 vi18n.get("historique.description.simple").arg(pistes).arg(
                                     historiqueService.formatTime(ouverture)).arg(nombre)

                elide: Text.ElideRight
                highlighted: false;
            }

            onClicked: {
                ouvrir(chemin)
            }

            Component.onCompleted: {
                ARTService.getArtAsync(chemin)

                // est on recup info "album"
                if(ConfigService.dirTag) {
                    MetaDataService.getMetaDataASynch(chemin,true);
                }

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

            Connections {
                target: MetaDataService

                onMetaDataAvailable: {
                    if( chemin !== vurl) {
                        return;
                    }

                    // et le titre
                    if( meta.album.length == 0) {
                        return;
                    }

                    titre.titre = (meta.annee >0 ? meta.annee + " - " : "") + meta.album + ( meta.artiste.length >0 ? ", " + meta.artiste : "")
                }
            }

            RemorseItem { id: remorse }

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
                        text:  vi18n.get("navigateur.rep.ajouter")
                        onClicked: ajouter(chemin)
                    }
                    MenuItem {
                        text:  vi18n.get("navigateur.action.supprimer")
                        onClicked: supprimer(uuid)
                    }
                }
            }

            //-----------------------------------------
            function supprimer(uuid) {
                remorse.execute(fileItem, "Deleting", function() {
                    historiqueService.supprimer(uuid);
                    for(var i=0 ; i<listModel.count;i++) {
                        if(listModel.get(i).uuid === uuid) {
                            listModel.remove(i);
                            return;
                        }
                    }
                });
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
        visible: loaded && listModel.count>0

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
                text:  vi18n.get("menu.enlecture")
                visible: page.orientation === Orientation.Portrait || page.orientation === Orientation.PortraitInverted
                onClicked: {
                    pageStack.push(Qt.resolvedUrl("ListeLecture.qml"));
                }
            }
            MenuItem {
                text:  vi18n.get("historique.vider")
                onClicked: {
                    historiqueService.vider();
                }
            }
        }

        header: PageHeader {
            title: vi18n.get("historique.titre")
        }

        delegate:entree

        ViewPlaceholder {
            enabled: loaded && listModel.count===0
            text:  vi18n.get("historique.vide")
            hintText:  vi18n.get("historique.vide.sum")
        }
    }

    Attente {
        enabled:  status===PageStatus.Activating
    }

    onStatusChanged: {
        // wait for view to load
        if (status===PageStatus.Active && loaded === false ) {
            recharger();
            loaded = true;
        }
    }

    Connections {
        target: historiqueService
        onListeChanged: recharger()
    }
}
