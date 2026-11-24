import QtQuick 2.2
import Sailfish.Silica 1.0

BackgroundItem {
    property color couleur: enabled ? ( pressed ? Theme.highlightColor : Theme.primaryColor ) : Theme.secondaryColor

    property color bordure : fond
    property color fond : "transparent"


    property alias rayon : rec.radius

    property string icone: "image://theme/icon-l-play"
    property alias label : titre.text
    property alias decription : desc.text

    property real espace : Theme.paddingMedium
    property real taille : Theme.iconSizeMedium
    property real fonte : Theme.fontSizeMedium

    id:bp

    Rectangle {
        id: rec
        radius:Theme.paddingSmall
        anchors.fill: parent
        border.color: bordure
        border.width: 1
        color: "transparent"

        Image {
            id: img
            source: icone + "?" +  couleur
            width: taille
            height: taille
            anchors {
                bottom: parent.verticalCenter
                horizontalCenter: parent.horizontalCenter
            }
        }

        Label {
            id:titre
            width: parent.width
            font.pixelSize: fonte
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
            anchors {
                top:img.bottom
                topMargin: espace
            }
        }

        Label {
            id:desc
            font.pixelSize: Theme.fontSizeExtraSmall
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
            anchors {
                top:titre.bottom
                topMargin: espace
            }

        }
    }

}
