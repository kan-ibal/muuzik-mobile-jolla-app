import QtQuick 2.2
import Sailfish.Silica 1.0
import "../components"
import "../js/app.js" as App

Page {
    id:page
    property bool active: status===PageStatus.Active
    property bool loaded: false

    //----------------------------------
    function ouvrir(uuid) {
        var pls = PlaylistService.getPls(uuid);
        lecteurService.remplacer(pls.liste);
        pageStack.push(Qt.resolvedUrl("ListeLecture.qml"));
    }

    //----------------------------------
    function ajouter(uuid) {
        var pls = PlaylistService.getPls(uuid);
        lecteurService.ajouter(pls.liste);
    }

    //----------------------------------
    function recharger() {
        var list = PlaylistService.liste;
        var l = [];
        for(var i in list) {
            l.push({
                       uuid: list[i].uuid,
                       nom: list[i].nom,
                       chemin: list[i].path,
                       duree: list[i].duree,
                       taille: list[i].taille
                   });
        }
        listModel.clear();
        listModel.append(l);
    }

    //-----------------------------------------
    function renommer(u,n) {
        for(var i = 0 ; i < listModel.count ; i++ ) {
            if(listModel.get(i).uuid===u) {
                listModel.setProperty(i,"nom",n);
                return;
            }
        }
    }

    Component {
        id: entree

        ListItem {
            id: fileItem
            menu: contextMenu
            width: ListView.view.width
            contentHeight: Math.max(img.height,description.height) +
                           (  ConfigService.marge ? Theme.paddingSmall : 0)

            Image {
                id: img
                anchors {
                    left: parent.left
                    leftMargin: Theme.horizontalPageMargin
                    verticalCenter: parent.verticalCenter
                }

                width: Theme.iconSizeLarge
                height: Theme.iconSizeLarge

                source: "image://muuzik/img/playlist?"+Theme.primaryColor
                sourceSize.width: Theme.iconSizeLarge
                sourceSize.height: Theme.iconSizeLarge

                asynchronous: true
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
                value: ""

                elide: Text.ElideRight
                highlighted: false;
            }

            Component.onCompleted: {
                description.value = vi18n.get("playlists.description").arg( App.getDuree(duree)).arg(taille);
            }

            onClicked: {
                ouvrir(uuid);
            }


            Component {
                id: contextMenu
                ContextMenu {
                    MenuItem {
                        text:  vi18n.get("playlists.ajouter")
                        onClicked: ajouter(uuid)
                    }
                    MenuItem {
                        text:  vi18n.get("playlists.editer")
                        onClicked: editer(uuid, nom)
                    }
                    MenuItem {
                        text:  vi18n.get("playlists.renommer")
                        onClicked: renommer(uuid)
                    }
                    MenuItem {
                        text:  vi18n.get("playlists.supprimer")
                        onClicked: supprimer(uuid)
                    }
                }
            }

            RemorseItem { id: remorse }

            //-----------------------------------------
            function supprimer(uuid) {
                remorse.execute(fileItem, "Deleting", function() {
                    PlaylistService.supprimer(uuid);
                    ListView.view.model.remove(index);
                } );
            }

            //-----------------------------------------
            function editer(uuid,nom) {
                pageStack.push(Qt.resolvedUrl("EditerPlaylist.qml"),{"uuid": uuid, "nom": nom});
            }

            //-----------------------------------------
            function renommer(uuid) {
                var dialog = pageStack.push(Qt.resolvedUrl("../dlg/NomDialog.qml"),
                                            {
                                                "valeur": nom,
                                                "dif": nom,
                                                "titre": vi18n.get("playlists.renommer.titre"),
                                                "label": vi18n.get("playlists.renommer.label")
                                            });

                //  dialog.canAccept=(dialog.valeur.length>0) && (dialog.valeur !== label);
                dialog.accepted.connect(function() {
                    PlaylistService.renommer(uuid,dialog.valeur);
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
                onClicked: {
                    pageStack.push(Qt.resolvedUrl("ListeLecture.qml"));
                }
            }
        }

        header: PageHeader {
            title: vi18n.get("playlists.titre")
        }

        delegate:entree

        ViewPlaceholder {
            enabled: loaded && listModel.count===0
            text: vi18n.get("playlists.vide")
            hintText: vi18n.get("playlists.vide.sum")
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
        target: PlaylistService

        onNomChanged : {
            recharger();
//            renommer(uuid, nouveauNom);
        }

        onTracksChanged: {
            recharger();
//            for(var i = 0 ; i < listModel.count ; i++ ) {
//                if(listModel.get(i).uuid===uuid) {
//                    var pls = PlaylistService.getPls(uuid);

//                    console.log("trackchanged " + pls.taille + " " + pls.duree);

//                    listModel.setProperty(i,"taille",pls.taille);
//                    listModel.setProperty(i,"duree",pls.duree);
//                    return;
//                }
//            }
        }
    }
}
