import QtQuick 2.2
import Sailfish.Silica 1.0
import "../components"
import "../hints"
import "../js/app.js" as App

import org.vostok.virtualfiles 1.0
import org.vostok.metadata 1.0

Page {
    id: page
    width: root.width

    property bool active: status===PageStatus.Active
    property string uri: ""       // base uri for the "view", defautl to "root"

    property bool _vertical: page.orientation === Orientation.Portrait ||
                             page.orientation === Orientation.PortraitInverted

    SilicaFlickable {
        contentWidth: parent.width
        contentHeight: nom.height +
                       (_vertical ?
                            containt.height + imgContainer.height + Theme.paddingLarge + Theme.horizontalPageMargin :
                            Math.max(containt.height+ Theme.paddingLarge + Theme.horizontalPageMargin, imgContainer.height))
        anchors.fill: parent

        VerticalScrollDecorator { flickable: parent }

        PageHeader {
            title: ""
            id:nom
        }

        Item {
            id:imgContainer
            width: _vertical ? parent.width : parent.width / 2
            height: _vertical ? parent.width / 1.5 : Screen.width - nom.height

            anchors {
                top:nom.bottom
                right: parent.right
            }

            Image {
                id:img
                source: "image://muuzik/img/son?" + Theme.primaryColor

                width: parent.width / 1.5
                height: width

                sourceSize.width: width
                sourceSize.height: height
                anchors.centerIn: parent
            }
        }

        Column {
            id:containt
            width: _vertical ? parent.width : parent.width / 2
            anchors {
                left: parent.left
                right: _vertical ? parent.right : imgContainer.left
                top: _vertical ? imgContainer.bottom : nom.bottom
                margins:Theme.horizontalPageMargin
            }

            SectionHeader {
                text:  vi18n.get("info.media")
                visible: artiste.visible || album.visible || annee.visible || duree.visible
            }

            LabelPlus {
                id:artiste
                icone:"image://theme/icon-m-people?"+ Theme.highlightColor
                width: parent.width
                visible: text.length>0
            }

            LabelPlus {
                id:album
                icone:"image://muuzik/img/album?"+ Theme.highlightColor
                width: parent.width
                visible: text.length>0
            }

            LabelPlus {
                id:annee
                icone:"image://theme/icon-s-date?"+ Theme.highlightColor
                width: parent.width
                visible: text !== 0
            }

            LabelPlus {
                id:duree
                icone:"image://theme/icon-s-time?"+ Theme.highlightColor
                width: parent.width
                visible: text.length>0
            }

            SectionHeader { text:  vi18n.get("info.information") }

            LabelPlus {
                id:taille
                icone:"image://theme/icon-s-device-download?"+ Theme.highlightColor
                width: parent.width
            }

            LabelPlus {
                id:emplacement
                icone:"image://theme/icon-m-folder?"+ Theme.highlightColor
                width: parent.width
            }

            LabelPlus {
                id:date
                icone:"image://theme/icon-s-date?"+ Theme.highlightColor
                width: parent.width
            }
        }
    }

    Component.onCompleted: {
        ARTService.getArtAsync(uri);
        MetaDataService.getMetaDataASynch(uri);

        var info = VFService.getInfo(uri);
        taille.text = App.getTaille(info.size);
        date.text = Qt.formatDateTime(new Date(info.timestamp), "yyyy-MM-dd hh:mm");
        emplacement.text = vi18n.get(
                    SdcardLocator.isOnSdcard(info.source) ? "info.emplacement.sdcard" : "info.emplacement.interne" );
    }

    Connections {
        target: ARTService
        onArtFound : {
            if( vurl !== uri) {
                return;
            }

            img.source = artPath;
        }
    }

    Connections {
        target: MetaDataService

        onMetaDataAvailable: {
            if( uri !== vurl) {
                return;
            }

            nom.title = meta.titre;
            artiste.text = meta.artiste;
            album.text = meta.album;
            annee.text = meta.annee;
            duree.text = App.getDuree(meta.duree);
        }
    }
}
