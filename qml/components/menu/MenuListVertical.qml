import QtQuick 2.2
import Sailfish.Silica 1.0
import "../"

ColumnView {
    id:mainColumn
    width: parent.width
    itemHeight: Theme.iconSizeExtraLarge
    anchors.centerIn: parent

    delegate: ListItem {
        width: parent.width
        contentHeight:Math.max(img.height, desc.height)

        Image {
            id: img
            anchors {
                left: parent.left
                leftMargin: Theme.horizontalPageMargin
                verticalCenter: parent.verticalCenter
            }
            width: Theme.iconSizeExtraLarge
            height: width
            source: icone + "?" + Theme.primaryColor
        }

        LabelPlus {
            id:desc
            anchors {
                left: img.right
                leftMargin: Theme.horizontalPageMargin
                right: parent.right
                rightMargin: Theme.horizontalPageMargin
                verticalCenter: parent.verticalCenter
            }
            description.wrapMode:Text.WordWrap
            highlighted: false
            texte.font.pixelSize:Theme.fontSizeLarge
            text: vi18n.get(t)
            //value: vi18n.get(d)
            Component.onCompleted: {
                desc.value =  vi18n.get(d); // trick pour lancer l'animation
            }
        }

        onClicked:  pageStack.push(Qt.resolvedUrl("../../pages/" + c));
    }
}
