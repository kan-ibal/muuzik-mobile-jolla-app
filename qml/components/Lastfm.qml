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
        title: vi18n.get("lastfm.titre")
        y: Theme.paddingLarge
        width: parent.width
    }

    Text {
        width: parent.width
        anchors {
            top: titre.bottom
            topMargin: Theme.paddingLarge
        }

        text:  vi18n.get("lastfm.message")
        wrapMode: Text.WordWrap
        color: Theme.highlightColor
      //  font.pointSize: Theme.fontSizeLarge
    }

    Button {
        id:lastfmgrant
        text:  vi18n.get("lastfm.bp")
        anchors {
            horizontalCenter: parent.horizontalCenter
            bottom: parent.bottom
            bottomMargin: Theme.paddingLarge
        }
        onClicked: parent.buttonClicked()
    }
}
