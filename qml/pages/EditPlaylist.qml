import QtQuick 2.2
import Sailfish.Silica 1.0
import "../components"
import "../js/app.js" as App
import pl.kanibal.metadata 1.0

Dialog {
    id: page

    property bool active: status === PageStatus.Active
    property string uuid: ""
    property string nom: ""
    property bool loaded: false
    property bool modified: false

    readonly property bool isSavedPlaylist: uuid.length > 0
    readonly property bool isReady: isSavedPlaylist ? loaded : true

    canAccept: isSavedPlaylist && modified !== false

    onAccepted: {
        if (!isSavedPlaylist) {
            return;
        }
        var tracks = [];
        for (var i = 0; i < listModel.count; i++) {
            tracks.push(listModel.get(i).chemin);
        }

        PlaylistService.writeAsync(uuid, tracks);
    }

    ListModel {
        id: listModel

        function populate(list) {
            var items = [];
            for (var i in list) {
                items.push({
                               uuid: list[i].uuid,
                               titre: list[i].titre,
                               chemin: list[i].chemin,
                               duree: list[i].duree,
                               album: list[i].album,
                               artiste: list[i].artiste
                           });
            }
            listModel.clear();
            listModel.append(items);
        }

        function moveUp(i) {
            modified = true;
            move(i, i - 1, 1);
        }

        function moveDown(i) {
            modified = true;
            move(i, i + 1, 1);
        }

        function removeAt(i) {
            modified = true;
            remove(i);
        }
    }

    Component {
        id: savedHeader

        DialogHeader {
            title: nom
            acceptText: vi18n.get("playlists.editer.sauver")
        }
    }

    Component {
        id: queueHeader

        PageHeader {
            title: vi18n.get("enlecture.edition.titre")
        }
    }

    Component {
        id: savedTrackDelegate

        ListItem {
            id: trackItem
            width: ListView.view.width
            contentHeight: Math.max(thumb.height, description.height) +
                           (ConfigService.marge ? Theme.paddingSmall : 0)

            Miniature {
                id: thumb
                anchors {
                    left: parent.left
                    leftMargin: Theme.horizontalPageMargin
                    verticalCenter: parent.verticalCenter
                }

                source: "image://muuzik/img/son?" + Theme.primaryColor
            }

            LabelPlus {
                id: description
                anchors {
                    left: thumb.right
                    right: deleteButton.left
                    leftMargin: Theme.horizontalPageMargin
                    rightMargin: Theme.horizontalPageMargin
                    verticalCenter: parent.verticalCenter
                }
                text: titre
                value: ""

                elide: Text.ElideRight
                highlighted: false
            }

            IconButton {
                id: deleteButton
                icon.source: "image://theme/icon-m-delete?" + Theme.primaryColor
                width: Theme.iconSizeSmallPlus
                height: Theme.iconSizeSmallPlus
                anchors {
                    verticalCenter: parent.verticalCenter
                    right: upButton.left
                    rightMargin: Theme.paddingLarge
                }

                onClicked: showRemorseItem()
            }

            IconButton {
                id: upButton
                icon.source: "image://theme/icon-m-up?" + Theme.primaryColor
                width: Theme.iconSizeSmallPlus
                height: Theme.iconSizeSmallPlus
                anchors {
                    verticalCenter: parent.verticalCenter
                    right: downButton.left
                    rightMargin: Theme.paddingLarge
                }
                enabled: index !== 0
                onClicked: listModel.moveUp(index)
            }

            IconButton {
                id: downButton
                icon.source: "image://theme/icon-m-down?" + Theme.primaryColor
                width: Theme.iconSizeSmallPlus
                height: Theme.iconSizeSmallPlus
                anchors {
                    verticalCenter: parent.verticalCenter
                    right: parent.right
                    rightMargin: Theme.paddingLarge
                }
                enabled: index !== listModel.count - 1
                onClicked: listModel.moveDown(index)
            }

            RemorseItem { id: remorse }

            ListView.onRemove: animateRemoval(trackItem)

            function showRemorseItem() {
                remorse.execute(trackItem, "Deleting", function() {
                    listModel.removeAt(index);
                });
            }

            Component.onCompleted: {
                MetaDataService.getMetaDataASynch(chemin);
                ARTService.getArtAsync(chemin);
            }

            Connections {
                target: ARTService
                onArtFound: {
                    if (vurl !== chemin) {
                        return;
                    }

                    thumb.overlay = artPath;
                }
            }

            Connections {
                target: MetaDataService

                onMetaDataAvailable: {
                    if (chemin !== vurl) {
                        return;
                    }

                    var durationPrefix = "";
                    if (meta.duree > 0) {
                        durationPrefix = App.getDuree(meta.duree) + " - ";
                    }

                    if (meta.artiste.length === 0 && meta.album.length === 0) {
                        description.value = vi18n.get("navigateur.audio.vide").arg(durationPrefix);
                        return;
                    }

                    if (meta.album.length === 0) {
                        description.value = vi18n.get("navigateur.audio.description.simple").arg(durationPrefix).arg(meta.artiste);
                        return;
                    }

                    if (meta.artiste.length === 0) {
                        description.value = vi18n.get("navigateur.audio.description.simple").arg(durationPrefix).arg(meta.album);
                        return;
                    }

                    description.value = vi18n.get("navigateur.audio.description").arg(durationPrefix).arg(meta.artiste).arg(meta.album);
                }
            }
        }
    }

    Component {
        id: queueTrackDelegate

        ListItem {
            id: trackItem
            width: ListView.view.width
            contentHeight: Math.max(trackTitle.height + trackInfo.height, deleteButton.height) + Theme.paddingLarge

            IconButton {
                id: deleteButton
                icon.source: "image://theme/icon-m-delete?" + Theme.primaryColor
                width: Theme.iconSizeSmallPlus
                height: Theme.iconSizeSmallPlus
                anchors {
                    verticalCenter: parent.verticalCenter
                    left: parent.left
                    leftMargin: Theme.paddingLarge
                }

                onClicked: showRemorseItem()
            }

            Label {
                id: trackTitle
                anchors {
                    left: deleteButton.right
                    right: upButton.left
                    leftMargin: Theme.paddingLarge
                    rightMargin: Theme.paddingLarge
                    bottomMargin: Theme.paddingTiny
                }
                text: titre
                elide: Text.ElideRight
            }

            Label {
                id: trackInfo
                anchors {
                    left: deleteButton.right
                    right: upButton.left
                    top: trackTitle.bottom
                    leftMargin: Theme.paddingLarge
                    rightMargin: Theme.paddingLarge
                }
                font.pixelSize: Theme.fontSizeTiny
                text: (artiste.length > 0 && album.length > 0) ? (artiste + " - " + album) : (artiste.length > 0 ? artiste : album)
                elide: Text.ElideRight
            }

            IconButton {
                id: upButton
                icon.source: "image://theme/icon-m-up?" + Theme.primaryColor
                width: Theme.iconSizeSmallPlus
                height: Theme.iconSizeSmallPlus
                anchors {
                    verticalCenter: parent.verticalCenter
                    right: downButton.left
                    rightMargin: Theme.paddingLarge
                }
                enabled: index !== 0
                onClicked: playerService.monterUuid(uuid)
            }

            IconButton {
                id: downButton
                icon.source: "image://theme/icon-m-down?" + Theme.primaryColor
                width: Theme.iconSizeSmallPlus
                height: Theme.iconSizeSmallPlus
                anchors {
                    verticalCenter: parent.verticalCenter
                    right: parent.right
                    rightMargin: Theme.paddingLarge
                }
                enabled: index !== playerService.size - 1
                onClicked: playerService.descendreUuid(uuid)
            }

            RemorseItem { id: remorse }

            ListView.onRemove: animateRemoval(trackItem)

            function showRemorseItem() {
                var trackUuid = uuid;
                remorse.execute(trackItem, "Deleting", function() {
                    playerService.retirerUuid(trackUuid);
                });
            }
        }
    }

    SilicaListView {
        id: trackList
        anchors {
            top: parent.top
            left: parent.left
        }
        height: parent.height
        width: parent.width
        clip: true
        visible: isReady

        model: isSavedPlaylist ? listModel : playerService.list

        VerticalScrollDecorator { flickable: trackList }

        addDisplaced: Transition {
            NumberAnimation { properties: "x,y"; duration: 1000 }
        }

        header: isSavedPlaylist ? savedHeader : queueHeader

        delegate: isSavedPlaylist ? savedTrackDelegate : queueTrackDelegate

        ViewPlaceholder {
            enabled: isReady && trackList.count === 0
            text: vi18n.get("enlecture.vide")
            hintText: vi18n.get("enlecture.vide.sum")
        }
    }

    Wait {
        enabled: !isReady || status === PageStatus.Activating
    }

    onStatusChanged: {
        if (isSavedPlaylist && status === PageStatus.Active && !loaded) {
            PlaylistService.openAsync(uuid);
        }
    }

    Connections {
        target: PlaylistService
        onPlaylistOuverte: {
            if (!isSavedPlaylist || uuid != page.uuid) {
                return;
            }
            loaded = true;
            listModel.populate(liste);
        }
    }
}
