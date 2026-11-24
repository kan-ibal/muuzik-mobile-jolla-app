import QtQuick 2.5
import Sailfish.Silica 1.0
import "../../js/app.js" as App

Item {
    id: echelle

    property int tempsEcoule: playerService.getPosition()
    property int tempsTotal: playerService.title ? playerService.title.duree : 0

    width: parent.width
    height: Math.max(ecoule.height,duree.height)

    Label {
        id: ecoule
        text: App.getDuree(tempsEcoule)
        color: Theme.primaryColor
        font.pixelSize: Theme.fontSizeSmall
        anchors {
            left: parent.left
            leftMargin: Theme.paddingLarge
            verticalCenter: parent.verticalCenter
        }
    }

    Slider {
        id:duree
        anchors.horizontalCenter: parent.horizontalCenter
        width: parent.width - avenir.width - ecoule.width
        minimumValue: 0
        maximumValue: Math.max(1, tempsTotal)
        enabled: tempsTotal > 0
        value: tempsEcoule
        onReleased: {playerService.avancer(duree.value);}
    }

    Label {
        id: avenir
        text: App.getDuree(tempsTotal)
        color: Theme.primaryColor
        font.pixelSize: Theme.fontSizeSmall
        anchors {
            right: parent.right
            rightMargin: Theme.paddingLarge
            verticalCenter: parent.verticalCenter
        }
    }

    Connections {
        target:playerService

        onDureeChanged:{
           if(enabled === false || visible === false || duree.pressed ) {
                return;
            }

            tempsEcoule = value;
            duree.value=value;
        }

        onTitleChanged: {
            tempsEcoule = playerService.getPosition();
            duree.value = tempsEcoule;
            tempsTotal = t ? t.duree : 0;
        }
    }

    Component.onCompleted: {
        tempsEcoule = playerService.getPosition();
        duree.value = tempsEcoule;
    }

    onVisibleChanged: {
        if (visible) {
            tempsEcoule = playerService.getPosition();
            duree.value = tempsEcoule;
        }
    }

}
