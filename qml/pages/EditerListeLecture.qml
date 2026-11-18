import QtQuick 2.2
import Sailfish.Silica 1.0
import "../js/app.js" as App

Page {
    id:page

    property bool active: status===PageStatus.Active


    SilicaListView {
        id: fileList
        anchors{
            top:parent.top
            left: parent.left
        }
        height: parent.height
        width: parent.width
        clip: true
        enabled:  playerService.size !== 0
        model: playerService.list;

        VerticalScrollDecorator { flickable: fileList }

        addDisplaced: Transition {
                NumberAnimation { properties: "x,y"; duration: 1000 }
            }


        header: PageHeader {
            title:   vi18n.get("enlecture.edition.titre")
        }

        delegate: ListItem {
            id: fileItem
            width: ListView.view.width
            contentHeight: Math.max( listtitre.height + listinfo.height, supprimer.height) + Theme.paddingLarge

            IconButton {
                id:supprimer
                icon.source:   "image://theme/icon-m-delete?"+ Theme.primaryColor
                width: Theme.iconSizeSmallPlus
                height: Theme.iconSizeSmallPlus
                anchors {
                    verticalCenter: parent.verticalCenter
                    left: parent.left
                    leftMargin: Theme.paddingLarge
                }

                onClicked: showRemorseItem();
            }

            Label {
                id: listtitre
                anchors {
                    left: supprimer.right
                    right: monter.left
                    leftMargin: Theme.paddingLarge
                    rightMargin: Theme.paddingLarge
                    bottomMargin: Theme.paddingTiny
                }
                text: titre
                elide: Text.ElideRight
            }

            Label {
                id: listinfo
                anchors {
                    left: supprimer.right
                    right: monter.left
                    top:listtitre.bottom
                    leftMargin: Theme.paddingLarge
                    rightMargin: Theme.paddingLarge
                }
                font.pixelSize: Theme.fontSizeTiny
                text: artiste + " - " + album
                elide: Text.ElideRight
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
                onClicked: playerService.monterUuid(uuid)
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
                enabled: index !== playerService.size -1
                onClicked: playerService.descendreUuid(uuid)
            }

            RemorseItem { id: remorse }

            ListView.onRemove: animateRemoval(fileItem)

            function showRemorseItem() {
                var idx = uuid;
                remorse.execute(fileItem, "Deleting", function() {
                    playerService.retirerUuid(idx);
                } );
            }

        }

        ViewPlaceholder {
            enabled:  playerService.size === 0
            text:  vi18n.get("enlecture.vide")
            hintText:  vi18n.get("enlecture.vide.sum")
        }
    }

}
