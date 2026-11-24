import QtQuick 2.5
import Sailfish.Silica 1.0
import "../../js/app.js" as App

Item {
    id: infoTitreRoot

    property var morceau: playerService.title

    width: parent.width
    height: infoColumn.height

    Column {
        id: infoColumn
        width: parent.width - Theme.horizontalPageMargin * 2
        anchors.horizontalCenter: parent.horizontalCenter
        spacing: Theme.paddingSmall / 2

        // titre du morceau en cours
        Label {
            id: titre
            width: parent.width
            horizontalAlignment: Text.AlignHCenter
            font.pixelSize: Theme.fontSizeExtraLarge
            font.bold: true
            elide: Text.ElideRight
            color: Theme.highlightColor
            text: morceau && (morceau.titre.length > 0) ? morceau.titre : "---"
        }

        // artiste du morceau en cours
        Label {
            id: artiste
            width: parent.width
            horizontalAlignment: Text.AlignHCenter
            font.pixelSize: Theme.fontSizeMedium
            elide: Text.ElideRight
            color: Theme.highlightColor
            text: morceau && (morceau.artiste.length > 0) ? morceau.artiste : "---"
        }

        // album du morceau en cours
        Label {
            id: album
            width: parent.width
            horizontalAlignment: Text.AlignHCenter
            font.pixelSize: Theme.fontSizeMedium
            elide: Text.ElideRight
            color: Theme.highlightColor
            text: morceau && (morceau.album.length > 0) ? morceau.album : "---"
        }
    }
}
