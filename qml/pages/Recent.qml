import QtQuick 2.2
import Sailfish.Silica 1.0
import "../components"
import "../hints"
import "../js/app.js" as App
import "../js/affichage.js" as Affichage

Page {
    id:page
    property bool active: status===PageStatus.Active

    //-----------------------------------------
    function openUri(v) {
        pageStack.push(Qt.resolvedUrl("Navigateur.qml"), {uri:v});
    }

    //-----------------------------------------
    function ouvrir(url) {
        lecteurService.remplacer([url]);
        pageStack.push(Qt.resolvedUrl("ListeLecture.qml"));
    }

    //-----------------------------------------
    function ajouter(url) {
        lecteurService.ajouter([url]);
    }

    //-----------------------------------------
    function info(url) {
        pageStack.push(Qt.resolvedUrl("InfoRep.qml"), {uri:url});

    }

    Component {
        id: entree

        ListItem {
            id: fileItem
            menu: contextMenu
            width: ListView.view.width
            contentHeight: Math.max(img.height,titre.height) +
                           (  ConfigService.marge ? Theme.paddingSmall : 0)

            property bool charge: false
            property string album: ""
            property string groupe: ""
            property int annee: 0
            property int duree: 0
            property int pistes: 0

            Miniature {
                id: img
                anchors {
                    left: parent.left
                    leftMargin: Theme.horizontalPageMargin
                    verticalCenter: parent.verticalCenter
                }

                source: "image://muuzik/img/album?" + Theme.primaryColor
            }

            LabelNavigation {
                id:titre
                anchors {
                    left: img.right
                    right: parent.right
                    leftMargin: Theme.horizontalPageMargin
                    rightMargin: Theme.horizontalPageMargin
                    verticalCenter: parent.verticalCenter
                }
                titre:App.getNomSimple(chemin)
                description: ""

                elide: Text.ElideRight
                highlighted: false;
            }

            onClicked: {
                ouvrir(chemin);
            }

            Component.onCompleted: {
                if(charge===true) {
                    return;
                }

                charge=true;
                ARTService.getArtAsync(chemin)

                // est on recup info "album"
                if(ConfigService.dirTag) {
                    MetaDataService.getStatFromAlbumASynch(chemin)
                } else {
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

                // @since 2.4 stats inclus aussi les info meta, on ne fait plus qu'un, où l'autre
                onMetaDataAvailable: {
                    if( chemin !== vurl) {
                        return;
                    }

                    titre.description = Affichage.getDescriptionPourAlbum(meta.artiste,0,0,0);

                    // et le titre
                    if( meta.album.length !== 0) {
                        titre.titre = (meta.annee >0 ? meta.annee + " - " : "") + meta.album
                    }

                }

                onStatAvailable : {
                    if( vurl !== chemin) {
                        return;
                    }

                    titre.description = Affichage.getDescriptionPourAlbum(stat.artiste,stat.duree,0,stat.pistes);

                    // et le titre
                    if( stat.album.length !== 0) {
                        titre.titre = (stat.annee >0 ? stat.annee + " - " : "") + stat.album
                    }
                }
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
                        text:  vi18n.get("navigateur.rep.ajouter")
                        onClicked: ajouter(chemin)
                    }

                    MenuItem {
                        text:  vi18n.get("navigateur.rep.information")
                        onClicked: info(chemin)
                    }

                }
            }
        }
    }

    ListModel {
        id:listModel

        function recharger() {
            var list = recentService.liste;
            var max = ConfigService.nbRecent;
            var nb = 0;

            var l = [];
            for(var i in list) {
                if(nb++ >= max) {
                    break;
                }

                l.push({ chemin: list[i] });
            }

            clear();
            append(l);
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
        visible: recentService.enCours === false && listModel.count>0

        model: listModel

        VerticalScrollDecorator { flickable: fileList }

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
                    recentService.recharger();
                }
            }
        }

        header: PageHeader {
            title: vi18n.get("recent.titre")
        }

        delegate:entree

        ViewPlaceholder {
            enabled:recentService.enCours === false && listModel.count===0
            text: vi18n.get("navigateur.vide")
            hintText: vi18n.get("navigateur.vide.sum ")
        }
    }

    Attente {
        enabled: status===PageStatus.Activating || recentService.enCours
    }

    //    onStatusChanged: {
    //        // wait for view to load
    //        if (status===PageStatus.Active && recentService.liste.length === 0 ) {
    //            recentService.recharger();
    //        }
    //    }

    Hint {
        id:hin1
        clee : "/vostok/muuzik/hint/recent"
        texte: vi18n.get("hint.recent")
        direction: TouchInteraction.Down
        interactionMode :TouchInteraction.Pull
    }

    Component.onCompleted: {
        hin1.lancer();
        listModel.recharger();
    }

    Connections {
        target: recentService
        onListeChanged : {
            listModel.recharger();
        }
    }
}


