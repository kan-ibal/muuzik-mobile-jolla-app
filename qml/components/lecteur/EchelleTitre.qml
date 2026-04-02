import QtQuick 2.5
import Sailfish.Silica 1.0
import "../../js/app.js" as App

Item {
    id: echelle

    property int tempsEcoule:0
    property int tempsTotal: lecteurService.titre ? lecteurService.titre.duree : 0

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
        maximumValue: tempsTotal
        value: 0
        onReleased: {lecteurService.avancer(duree.value);}
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
        target:lecteurService

        onDureeChanged:{
           if(enabled === false || visible === false || duree.pressed ) {
                return;
            }

            tempsEcoule = value;
            duree.value=value;
        }

        onTitreChanged: {
            tempsEcoule = 0;
            tempsTotal = t.duree;
        }
    }

}
