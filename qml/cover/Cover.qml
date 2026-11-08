import QtQuick 2.0
import Sailfish.Silica 1.0

import "../js/app.js" as App

CoverBackground {
    property bool active: status === Cover.Active
    property var morceau: lecteurService.titre

    function infoLecture(value) {
        ecoule.text = App.getDuree(value);
    }

    function infoBoutonLecture(s) {
        icoplay.iconSource = (s === true ? "image://theme/icon-cover-pause" : "image://theme/icon-cover-play");
    }

    CoverPlaceholder {
        visible: morceau === null
        icon.source: "qrc:///img/logo"
        text: vi18n.get("app.titre")
    }

    Item {
        visible: morceau !==null
        anchors.fill: parent

        Image {
            id:fond
            source:  "image://muuzik/img/son?"+ Theme.primaryColor // chargé dynamiquement
            asynchronous: true
            anchors.fill: parent
            cache: false
            fillMode: Image.PreserveAspectCrop

            //                ok ?
            //              (source.indexOf("image") === 0 ? Image.PreserveAspectFit :  Image.PreserveAspectCrop)
            //                          : Image.PreserveAspectFit
            sourceSize.height: parent.height

            Connections {
                target: ARTService
                onArtFound : {
                    try {
                        if( vurl !== morceau.chemin) {
                            return;
                        }
                        fond.source = artPath;
                    }catch(e){}
                }
            }
        }

        OpacityRampEffect {
            sourceItem: fond
            direction: OpacityRamp.TopToBottom
        }

        Label {
            id:ecoule
            font.pixelSize: Theme.fontSizeHuge
            font.bold: true
            text: ""
            color: Theme.highlightColor
            anchors.centerIn: parent
        }

        Label {
            text: morceau !== null ? morceau.titre : ""
            color: Theme.highlightColor
            font.pixelSize: Theme.fontSizeExtraSmall
            anchors {
                horizontalCenter: parent.horizontalCenter
                top: ecoule.bottom
                topMargin: Theme.paddingMedium
            }
        }
    }

    CoverActionList {
        CoverAction {
            id : icoplay
            iconSource: "image://theme/icon-cover-pause?"+ Theme.primaryColor
            onTriggered: lecteurService.lire();
        }

        CoverAction {
            iconSource: "image://theme/icon-cover-next-song?"+ Theme.primaryColor
            onTriggered: lecteurService.suivant();
        }
    }

    onActiveChanged: {
        if(active) {
            infoLecture(lecteurService.getPosition());
            infoBoutonLecture(lecteurService.getEtat());
            if(morceau) {
                ARTService.getArtAsync(morceau.chemin);
            }
        }
    }

    Connections {
        target:lecteurService
        onEtatChanged:{
            infoBoutonLecture(s);
        }

        onDureeChanged:{
            infoLecture(value);
        }

        onTitreChanged: {
            fond.source = "image://muuzik/img/son?"+ Theme.primaryColor
            if (t) {
                ARTService.getArtAsync(t.chemin);
            }
        }
    }
}
