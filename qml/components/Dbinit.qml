import QtQuick 2.2
import Sailfish.Silica 1.0

Item {
    width: parent.width
    anchors {
        fill: parent
        margins: Theme.paddingLarge
    }

    signal buttonClicked

    PageHeader {
        id:titre
        title: vi18n.get("recherche.db.titre")
        y: Theme.paddingLarge
        width: parent.width
    }

    Text {
        width: parent.width
        anchors {
            top: titre.bottom
            topMargin: Theme.paddingLarge
        }
        text:  vi18n.get("recherche.db.info")
        wrapMode: Text.WordWrap
        color: Theme.highlightColor
    }

    Button {
        id:bouton
        text:  vi18n.get("recherche.db.bouton")
        anchors {
            horizontalCenter: parent.horizontalCenter
            bottom: parent.bottom
            bottomMargin: Theme.paddingLarge
        }
        onClicked: parent.buttonClicked()
    }
}
