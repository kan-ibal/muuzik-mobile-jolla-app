import QtQuick 2.2
import Sailfish.Silica 1.0
import"../components"

Page {
    id:page
    width: root.width

    SilicaFlickable {
        width: parent.width
        anchors.fill: parent
        contentHeight: content.height
        contentWidth: parent.width

        VerticalScrollDecorator {flickable:parent}

        Column {
            id: content
            width: parent.width
            spacing: Theme.paddingMedium

            anchors {
                left: parent.left
                right: parent.right
                margins:Theme.horizontalPageMargin
            }

            PageHeader {
                title: vi18n.get("menu.apropos")
            }

            Row {
                spacing: Theme.paddingMedium
                height: Theme.itemSizeExtraLarge
                anchors.margins: Theme.paddingMedium
                Image {
                    width: Theme.iconSizeLauncher
                    height:Theme.iconSizeLauncher
                    anchors.verticalCenter: parent.verticalCenter
                    source: "qrc:///img/logo"
                }

                Column {
                    anchors.verticalCenter: parent.verticalCenter

                    Label {
                        text:vi18n.get("apropos.titre")
                        font.pixelSize: Theme.fontSizeHuge
                        font.bold: true
                    }

                    Label {
                        text:vi18n.get("apropos.version").arg(Qt.application.version)
                        font.pixelSize: Theme.fontSizeSmall
                    }
                }
            }

            SectionHeader {
                text: vi18n.get("apropos.licence")
              //  position: true
            }

            Label {
                width:parent.width
                wrapMode: Text.WordWrap
                text: vi18n.get("apropos.licence.texte")
                color: Theme.highlightColor
                textFormat: Text.StyledText
            }

            SectionHeader {
                text: vi18n.get("apropos.copy")
                //position: true
            }
            Label {
                width:parent.width
                wrapMode: Text.WordWrap
                text: vi18n.get("apropos.copy.texte").arg("2017-2022")
                color: Theme.highlightColor
                textFormat: Text.StyledText
            }
            Label {
                width:parent.width
                wrapMode: Text.WordWrap
                text: vi18n.get("apropos.copy.text2").arg("2026")
                color: Theme.highlightColor
                textFormat: Text.StyledText
            }

            SectionHeader {
                text: vi18n.get("apropos.remerciement")
            }
            Label {
                width:parent.width
                wrapMode: Text.WordWrap
                text: "Carmen, Christoph, Khan"
                color: Theme.highlightColor
                textFormat: Text.StyledText
            }

            Item {
                width: parent.width
                height: Theme.itemSizeExtraLarge
            }

            BackgroundItem {
                width: parent.width
                height: Theme.itemSizeExtraLarge

                onClicked: Qt.openUrlExternally("https://openrepos.net/content/kan/Muuzik!")

                Label {
                    width:parent.width
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    text: "Muuzik!@openrepos"
                    font.underline: true
                    font.bold: true
                }
            }
        }
    }
}
