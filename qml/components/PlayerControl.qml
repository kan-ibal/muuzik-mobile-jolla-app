import QtQuick 2.5
import Sailfish.Silica 1.0
import "player"
import "../js/app.js" as App

Rectangle {
    id: controle

    property bool _vertical: page.orientation === Orientation.Portrait ||
                             page.orientation === Orientation.PortraitInverted
    property bool panneauOuvert : false

    anchors {
        horizontalCenter: parent.horizontalCenter
    }

    width: parent.width
    height: panneauOuvert ? parent.height :  principale.height
    color: panneauOuvert ? "transparent" : "black"

    // le panneau complet
    Item {
        id: complete

        visible: panneauOuvert
        anchors {
            top:parent.top
            topMargin: Theme.paddingLarge
        }
        width: parent.width
        height: _vertical ? parent.height - principale.height : parent.height

        // le bp de fermeture
        IconButton {
            id:fermeture
            anchors {
                top: parent.top
                horizontalCenter: parent.horizontalCenter
                bottomMargin: Theme.horizontalPageMargin
            }

            icon {
                source: "image://theme/icon-m-down?" + Theme.primaryColor
                width: Theme.itemSizeExtraSmall
                height: Theme.itemSizeExtraSmall
            }
            onClicked: {

                playerService.showControl=false;

            //    panneauOuvert=false;
            }
        }

        // block image et jauge
        Item {
            id:jauge

            anchors {
                top: fermeture.bottom
                left: parent.left
            }
            width: _vertical ? parent.width : (parent.width / 2) - Theme.paddingMedium
            height: jaugeC.height

            TtileGauge {
                id: jaugeC

                anchors {
                    horizontalCenter: parent.horizontalCenter
                }

                //  ls bp de lecture aleatoire
                IconButton {
                    anchors {
                        left:parent.left
                        bottom: parent.bottom
                        margins: Theme.horizontalPageMargin
                    }

                    icon {
                        source: "image://theme/icon-m-shuffle?" + Theme.primaryColor
                        width: Theme.iconSizeSmallPlus
                        height: Theme.iconSizeSmallPlus
                    }
                    highlighted: playerService.random
                    onClicked:  {
                        playerService.switchRandom();}
                }

                // le bp de repetition
                IconButton {
                    anchors {
                        right: parent.right
                        bottom: parent.bottom
                        margins: Theme.horizontalPageMargin
                    }

                    icon {
                        source: "image://theme/icon-m-repeat?" + Theme.primaryColor
                        width: Theme.iconSizeSmallPlus
                        height: Theme.iconSizeSmallPlus
                    }
                    highlighted:playerService.loop
                    onClicked: {playerService.switchLoop();}
                }
            }
        }

        // info titre + stream / encoder metadata
        Item {
            id:info

            anchors {
                top: _vertical ? jauge.bottom : fermeture.bottom
                topMargin: Theme.paddingSmall
                bottom: echelle.top
                bottomMargin: Theme.paddingSmall
                right: parent.right
            }
            width: _vertical ? parent.width : parent.width / 2

            Column {
                id: infoContentColumn
                width: parent.width
                y: Math.max(0, (parent.height - height) / 2)
                spacing: Theme.paddingSmall

                TitleInfo {
                }

                Column {
                    id: streamEncoderInfo
                    width: parent.width - Theme.horizontalPageMargin * 2
                    anchors.horizontalCenter: parent.horizontalCenter
                    spacing: Theme.paddingSmall / 2

                    readonly property string encoderText: {
                        var parts = [];
                        if (playerService.audioCodec.length > 0) {
                            parts.push(playerService.audioCodec);
                        }
                        if (playerService.audioBitrate > 0) {
                            parts.push(playerService.audioBitrate + " kbps");
                        }
                        return parts.join(" \u2022 ");
                    }

                    visible: playerService.streamArtist.length > 0 ||
                             playerService.streamTitle.length > 0 ||
                             encoderText.length > 0

                    Label {
                        id: streamArtistLabel
                        width: parent.width
                        horizontalAlignment: Text.AlignHCenter
                        font.pixelSize: Theme.fontSizeSmall
                        elide: Text.ElideRight
                        color: Theme.primaryColor
                        text: playerService.streamArtist
                        visible: playerService.streamArtist.length > 0
                    }

                    Label {
                        id: streamTitleLabel
                        width: parent.width
                        horizontalAlignment: Text.AlignHCenter
                        font.pixelSize: Theme.fontSizeSmall
                        font.bold: true
                        elide: Text.ElideRight
                        color: Theme.primaryColor
                        text: playerService.streamTitle
                        visible: playerService.streamTitle.length > 0
                    }

                    Label {
                        id: encoderInfoLabel
                        width: parent.width
                        horizontalAlignment: Text.AlignHCenter
                        font.pixelSize: Theme.fontSizeExtraSmall
                        elide: Text.ElideRight
                        color: Theme.secondaryColor
                        text: streamEncoderInfo.encoderText
                        visible: streamEncoderInfo.encoderText.length > 0
                    }
                }
            }
        }

        // echelle avancement dans le titre
        TitleScale {
            id: echelle
            anchors {
                bottom: parent.bottom
                bottomMargin: Theme.paddingLarge + (_vertical ? 0 : principale.height)
                right: parent.right
            }
            width:  _vertical ? parent.width : parent.width / 2
        }
    }

    //-------------------
    //la barre principale
    BackgroundItem {
        id: principale

        anchors {
            bottom: parent.bottom
            right: parent.right
        }
        height: boutonOuvrir.height + barreBoutons.height
        width: _vertical || (panneauOuvert === false) ? parent.width : parent.width /2

        property real ratio: panneauOuvert ? 1.0 : 0.75

        Image {
            id:boutonOuvrir
            source:"image://theme/icon-m-up?#ffffff"
            visible: panneauOuvert === false
            width: Theme.iconSizeSmall
            height: width
            anchors {
                horizontalCenter: parent.horizontalCenter
                top:parent.top
            }
        }

        Item {
            id:barreBoutons

            anchors.bottom: parent.bottom
            height: Theme.iconSizeLarge * principale.ratio
            width: parent.width

            PlayerControlBar {
                anchors {
                    horizontalCenter: panneauOuvert ? parent.horizontalCenter : undefined
                    left: panneauOuvert ? undefined : parent.left
                }
                width: panneauOuvert ? parent.width : Theme.iconSizeMedium * 4

                tailleBp : Theme.iconSizeMedium * principale.ratio
                couleur: panneauOuvert ? Theme.primaryColor : "white"
            }

            // le compteur de titre
            TitleCounter {
                color: "white"
                visible: panneauOuvert === false
                anchors {
                    right:  parent.right
                    rightMargin: Theme.horizontalPageMargin
                    verticalCenter: parent.verticalCenter
                }
            }
        }

        onClicked: {
            if(panneauOuvert) {
                return;
            }

            playerService.showControl=true;

           // panneauOuvert = !panneauOuvert;
        }
    }
}
