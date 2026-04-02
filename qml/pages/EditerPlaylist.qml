import QtQuick 2.2
import Sailfish.Silica 1.0
import "../components"
import "../js/app.js" as App
import org.vostok.metadata 1.0

Dialog {
    id:page

    property bool active: status===PageStatus.Active
    property string uuid
    property string nom
    property bool loaded: false
    property bool modif: false

    canAccept: modif !== false

    onAccepted: {
        var donnee=[];
        for(var i = 0 ; i < listModel.count ; i++ ) {
            donnee.push(listModel.get(i).chemin);
        }

        PlaylistService.ecrireAsync(uuid,donnee);
    }

    ListModel {
        id:listModel

        //-----------------------------------------
        function remplir(list) {
            var l = [];
            for(var i in list) {
                l.push({
                           uuid: list[i].uuid,
                           titre: list[i].titre,
                           chemin: list[i].chemin,
                           duree: list[i].duree,
                           album: list[i].album,
                           artiste: list[i].artiste
                       });
            }
            listModel.clear();
            listModel.append(l);
        }

        //-----------------------------------------
        function monter(i) {
            modif=true;
            move(i,i-1,1);
        }

        //-----------------------------------------
        function descendre(i) {
            modif=true;
            move(i,i+1,1);
        }

        //-----------------------------------------
        function supprimer(i) {
            modif=true;
            remove(i);
            //     donnee.splice(i,1);
        }

    }

    Component {
        id: entree

        ListItem {
            id: fileItem
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

                source: "image://muuzik/img/son?"+Theme.primaryColor
            }

            LabelPlus {
                id:description
                anchors {
                    left: img.right
                    right: sup.left
                    leftMargin: Theme.horizontalPageMargin
                    rightMargin: Theme.horizontalPageMargin
                    verticalCenter: parent.verticalCenter
                }
                text:titre
                value: "klkllklkjlkjlk"

                elide: Text.ElideRight
                highlighted: false;
            }

            IconButton {
                id:sup
                icon.source:   "image://theme/icon-m-delete?" + Theme.primaryColor
                width: Theme.iconSizeSmallPlus
                height: Theme.iconSizeSmallPlus
                anchors {
                    verticalCenter: parent.verticalCenter
                    right: monter.left
                    rightMargin: Theme.paddingLarge
                }

                onClicked: showRemorseItem();

            }

            IconButton {
                id:monter
                icon.source:   "image://theme/icon-m-up?"+ Theme.primaryColor
                width: Theme.iconSizeSmallPlus
                height: Theme.iconSizeSmallPlus
                anchors {
                    verticalCenter: parent.verticalCenter
                    right: descendre.left
                    rightMargin: Theme.paddingLarge
                }
                enabled: index !==0
                onClicked: listModel.monter(index)
            }

            IconButton {
                id:descendre
                icon.source:   "image://theme/icon-m-down?"+ Theme.primaryColor
                width: Theme.iconSizeSmallPlus
                height: Theme.iconSizeSmallPlus
                anchors {
                    verticalCenter: parent.verticalCenter
                    right: parent.right
                    rightMargin: Theme.paddingLarge
                }
                enabled: index !== listModel.count - 1
                onClicked: listModel.descendre(index)
            }

            RemorseItem { id: remorse }

            ListView.onRemove: animateRemoval(fileItem)

            //-----------------------------------------
            function showRemorseItem() {
                remorse.execute(fileItem, "Deleting", function() {
                    listModel.supprimer(index);
                } );
            }

            Component.onCompleted: {
                MetaDataService.getMetaDataASynch(chemin);
                ARTService.getArtAsync(chemin);
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

                    var t= "";
                    if(meta.duree>0) {
                        t = App.getDuree(meta.duree) + " - ";
                    }

                    if(meta.artiste.length===0 && meta.album.length===0) {
                        description.value= vi18n.get("navigateur.audio.vide").arg(t);
                        return;
                    }

                    if(meta.album.length===0) {
                        description.value= vi18n.get("navigateur.audio.description.simple").arg(t).arg(meta.artiste);
                        return;
                    }

                    if(meta.artiste.length===0) {
                        description.value= vi18n.get("navigateur.audio.description.simple").arg(t).arg(meta.album);
                        return;
                    }

                    description.value= vi18n.get("navigateur.audio.description").arg(t).arg(meta.artiste).arg(meta.album);
                }
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

        model: listModel

        VerticalScrollDecorator { }

        addDisplaced: Transition {
            NumberAnimation { properties: "x,y"; duration: 1000 }
        }

        header: DialogHeader {
            title: nom
            acceptText: vi18n.get("playlists.editer.sauver")
        }

        delegate: entree

        ViewPlaceholder {
            enabled:  listModel.count === 0 && loaded
            text:  vi18n.get("enlecture.vide")
            hintText:  vi18n.get("enlecture.vide.sum")
        }
    }

    Attente {
        enabled: loaded === false || status===PageStatus.Activating
    }

    onStatusChanged: {
        // wait for view to load
        if (status===PageStatus.Active && loaded === false ) {
            PlaylistService.ouvrirAsync(uuid);
        }
    }

    Connections {
        target:   PlaylistService
        onPlaylistOuverte: {
            if(uuid != page.uuid) {
                return;
            }
            loaded = true;
            listModel.remplir(liste);
        }
    }
}
