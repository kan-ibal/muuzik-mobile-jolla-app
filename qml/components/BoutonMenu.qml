import QtQuick 2.2
import Sailfish.Silica 1.0

BackgroundItem {
    property color couleur : Theme.primaryColor
    property alias rayon : rec.radius

    property string icone: ""
    property real tailleIcone : Theme.iconSizeMedium

    property alias label :desc.text
    property bool vertical : false


    Rectangle {
        id: rec
        radius:Theme.paddingMedium
        anchors.fill: parent
        anchors.margins: Theme.horizontalPageMargin / 2
        border.color: couleur
        border.width: 1
        color: "transparent"

        Image {
            id: img
            source: icone + "?" +  couleur
            anchors {
                top:parent.top
                leftMargin: vertical ? 0 : Theme.horizontalPageMargin
                horizontalCenter: vertical || label.length == 0 ? parent.horizontalCenter : undefined
                verticalCenter: vertical && label.length > 0 ? undefined : parent.verticalCenter
            }
            width: tailleIcone
            height: tailleIcone
        }

        Label {
            id:desc
            font.pixelSize:Theme.fontSizeLarge
            visible: text.length >0
            anchors {
                top: vertical ? img.bottom : undefined
                left: vertical ? undefined : img.right
                leftMargin: vertical ? 0 : Theme.horizontalPageMargin
                horizontalCenter: vertical ? parent.horizontalCenter : undefined
                verticalCenter: vertical ? undefined : parent.verticalCenter
            }
        }
    }
}
