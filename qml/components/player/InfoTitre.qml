import QtQuick 2.5
import Sailfish.Silica 1.0
import "../../js/app.js" as App

Item {
    id:info

    property var morceau: lecteurService.titre

    anchors {
        horizontalCenter: parent.horizontalCenter
    }
    width: parent.width

    // titre du morceau en cours
    Label {
        id:titre
        anchors {
            horizontalCenter: parent.horizontalCenter
        }
        font.pixelSize: Theme.fontSizeExtraLarge
        font.bold: true
        elide: Text.ElideRight
        color: Theme.highlightColor
        text: morceau ? morceau.titre : "---"
    }

    // artiste du morceau en cours
    Label {
        id:artiste
        anchors {
            horizontalCenter: parent.horizontalCenter
            top:titre.bottom
        }
        font.pixelSize: Theme.fontSizeMedium
        elide: Text.ElideRight
        color: Theme.highlightColor
        text: morceau && (morceau.artiste.length>0) ? morceau.artiste : "---"
    }

    // album du morceau en cours
    Label {
        anchors {
            horizontalCenter: parent.horizontalCenter
            top:artiste.bottom
        }
        font.pixelSize: Theme.fontSizeMedium
        elide: Text.ElideRight
        color: Theme.highlightColor
        text: morceau && (morceau.album.length>0) ? morceau.album : "---"
    }
}
