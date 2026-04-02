import QtQuick 2.2
import Sailfish.Silica 1.0

BackgroundItem {
    property color couleur : Theme.primaryColor
    property color fond : "transparent"
    property alias rayon : rec.radius
    property real espace : Theme.paddingMedium
    property real taille : Theme.iconSizeMedium
    property string icone: "image://theme/icon-l-play"

    id:bp

    width: rec.width
    height: rec.height

    Rectangle {
        id: rec
        radius:Theme.paddingSmall
        height: img.height + (2 * espace)
        width: img.width + (2 * espace)
        color: fond
        Image {
            id: img
            source: icone + "?" +  couleur
            width: taille
            height: taille
            anchors.verticalCenter: parent.verticalCenter
            anchors.horizontalCenter: parent.horizontalCenter
        }
    }

}
