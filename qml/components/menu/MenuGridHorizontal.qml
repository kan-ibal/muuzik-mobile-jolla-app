import QtQuick 2.2
import Sailfish.Silica 1.0
import "../"

GridView {
    id:grille

    anchors.centerIn: parent
    width: parent.width * 0.85
    height: parent.height * 0.85

    cellHeight : height/2
    cellWidth : width/4

    delegate: ListItem {
        anchors.margins: Theme.horizontalPageMargin

        width: grille.cellWidth
        height: grille.cellHeight

        Image {
            id: img
            source: icone + "?" +  Theme.primaryColor
            anchors {
                top:parent.top
                horizontalCenter: parent.horizontalCenter
            }
            height: Theme.iconSizeLarge
            width: height
        }

        Label {
            id:desc
            font.pixelSize:Theme.fontSizeLarge
            text: vi18n.get(t)
            anchors {
                top: img.bottom
                horizontalCenter: parent.horizontalCenter
            }
        }

        onClicked: pageStack.push(Qt.resolvedUrl("../../pages/" + c));
    }
}
