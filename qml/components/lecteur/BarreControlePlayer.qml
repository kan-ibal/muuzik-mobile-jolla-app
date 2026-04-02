import QtQuick 2.5
import Sailfish.Silica 1.0
import "../../js/app.js" as App

Item {
    anchors {
        verticalCenter: parent.verticalCenter
        leftMargin: Theme.horizontalPageMargin
        rightMargin: Theme.horizontalPageMargin
    }

    width:  parent.width
    height: parent.height

    property real tailleBp : Theme.iconSizeMedium
    property string couleur: Theme.primaryColor

    IconButton {
        id: bp_prec
        anchors {
            left: parent.left
            leftMargin: Theme.horizontalPageMargin
            verticalCenter: parent.verticalCenter
        }
        width: parent.tailleBp
        height: width
        icon {
            source: "image://theme/icon-m-previous?"+couleur
            width: parent.tailleBp
            height: width
        }

        onClicked: lecteurService.precedent()
        enabled: lecteurService.index>0
    }

    IconButton {
        id:bp_play
        anchors {
            horizontalCenter: parent.horizontalCenter
            verticalCenter: parent.verticalCenter
        }
        width: parent.tailleBp
        height: width
        icon {
            source: (lecteurService.etat ? "image://theme/icon-m-pause?" : "image://theme/icon-m-play?") + couleur
            width: parent.tailleBp
            height: width
        }

        onClicked: lecteurService.lire()
    }

    IconButton {
        id:bp_suivant
        anchors {
            rightMargin: Theme.horizontalPageMargin
            right: parent.right
            verticalCenter: parent.verticalCenter
        }
        width: parent.tailleBp
        height: width
        icon {
            source: "image://theme/icon-m-next?"+couleur
            width: parent.tailleBp
            height: width
        }

        onClicked: lecteurService.suivant()
        enabled: lecteurService.index<lecteurService.taille
    }
}
