import QtQuick 2.2
import Sailfish.Silica 1.0
import "../components"
import "../hints"
import "../js/app.js" as App
import "../js/affichage.js" as Affichage

import org.vostok.virtualfiles 1.0
import org.vostok.metadata 1.0

Page {
    id: page
    width: root.width

    property bool active: status===PageStatus.Active
    property string uri: ""       // base uri for the "view", defautl to "root"
    property string filter: "\\.(mp3|ogg|flac|m4a|m4b|wav|aif|aiff|opus|alac)$"
    property int batch_id: -1
    property bool artset: false
    property date currentDate: new Date()

    property bool _vertical: page.orientation === Orientation.Portrait ||
                             page.orientation === Orientation.PortraitInverted

    //--------------------------------------
    //@param val : Date
    function formateDate(val) {
        return Qt.formatDateTime(val, "yyyy-MM-dd hh:mm");
    }

    //--------------------------------------
    SilicaFlickable {
        id:container
        contentWidth: parent.width
        contentHeight: nom.height +
                       (_vertical ?
                            containt.height + imgContainer.height + Theme.paddingLarge + Theme.horizontalPageMargin :
                            Math.max(containt.height+ Theme.paddingLarge + Theme.horizontalPageMargin, imgContainer.height))

        anchors.bottomMargin: barredate.open ? barredate.height : 0
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
                source: "image://theme/icon-m-folder?" + Theme.primaryColor

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
                visible: artiste.visible || album.visible || annee.visible || duree.visible || piste.visible
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
                visible: text.length>0
            }

            LabelPlus {
                id:duree
                icone:"image://theme/icon-s-time?"+ Theme.highlightColor
                width: parent.width
                visible: text.length>0
            }

            LabelPlus {
                id:piste
                icone:"image://muuzik/img/son?"+ Theme.highlightColor
                width: parent.width
                visible: text.length>0
            }

            SectionHeader { text:  vi18n.get("info.information") }

            LabelPlus {
                id:localisation
                icone:"image://theme/icon-m-folder?"+ Theme.highlightColor
                width: parent.width
            }

            LabelPlus {
                id:taille
                icone:"image://theme/icon-s-device-download?"+ Theme.highlightColor
                width: parent.width
            }

            LabelPlus {
                id:nbDir
                icone:"image://theme/icon-m-file-folder?"+ Theme.highlightColor
                width: parent.width
                visible: text.length>0
            }

            LabelPlus {
                id:nbFile
                icone:"image://muuzik/img/son?"+ Theme.highlightColor
                width: parent.width
                visible: text.length>0
            }

            Item {
                width: parent.width
                height: dateDir.height

                LabelPlus {
                    id:dateDir
                    icone:"image://theme/icon-s-date?"+ Theme.highlightColor
                    width: parent.width
                }

                IconButton {
                    icon.source: "image://theme/icon-s-edit?" + Theme.primaryColor
                    width: Theme.iconSizeSmall
                    height: Theme.iconSizeSmall

                    anchors {
                        verticalCenter: parent.verticalCenter
                        right: parent.right
                        rightMargin: Theme.horizontalPageMargin
                    }

                    onClicked: {
                        barredate.open = barredate.open ? false : true;
                        container.scrollToBottom();
                    }
                }
            }
        }
    }

    DockedPanel {
        id: barredate
        width: parent.width
        height: Theme.iconSizeExtraLarge * 1.5
        dock:  Dock.Bottom
        parent: page.parent
        open:false

        Item {
            height: parent.height
            width: parent.width / 2
            anchors {
                top: parent.top
                left: parent.left
            }

            IconButton  {
                icon.source: "image://theme/icon-l-date"
                width: Theme.iconSizeLarge
                height: width

                anchors.centerIn: parent
                onClicked:  {
                    barredate.open = false;
                    var dialog = pageStack.push("Sailfish.Silica.DatePickerDialog", {
                                                    date: currentDate
                                                });
                    dialog.accepted.connect(function() {
                        VFService.date(uri, dialog.date.getTime());
                        currentDate = dialog.date;
                        dateDir.text = formateDate(dialog.date);
                    });
                }
            }
        }

        Item {
            height: parent.height
            width: parent.width / 2
            anchors {
                top: parent.top
                right:parent.right
            }

            IconButton  {
                icon.source: "image://theme/icon-m-time-date"
                width: Theme.iconSizeLarge
                height: width

                anchors.centerIn: parent
                onClicked:  {
                    barredate.open = false;
                    var dialog = pageStack.push("Sailfish.Silica.TimePickerDialog", {
                                                    hour: currentDate.getHours(),
                                                    minute: currentDate.getMinutes(),
                                                    hourMode: DateTime.TwentyFourHours
                                                });
                    dialog.accepted.connect(function() {
                        VFService.date(uri, dialog.time.getTime());
                        dateDir.text = formateDate(dialog.time);
                    });
                }
            }
        }
    }

    Component.onCompleted: {
        ARTService.getArtAsync(uri);
        MetaDataService.getStatFromAlbumASynch(uri);
        batch_id = VFService.getStat(uri,filter);

        var info = VFService.getInfo(uri);
        currentDate = new Date(info.timestamp);
        dateDir.text = formateDate(new Date(info.timestamp));
        nom.title = info.name;
        localisation.text = App.getRepParent(uri);

        var cd = VFService.countDirectories(uri);
        var cf = VFService.countFiles(uri,filter);

        if(artset) {
            return;
        }

        img.source = (cf === 0 || cd > 0 ? "image://theme/icon-m-folder?" : "image://muuzik/img/album?") + Theme.primaryColor;
    }

    Connections {
        target: ARTService
        onArtFound: {
            if( vurl !== uri) {
                return;
            }

            img.source = artPath;
            artset = true;
        }
    }

    Connections {
        target: MetaDataService
        onStatAvailable: {
            if( uri !== vurl) {
                return;
            }

            artiste.text = stat.artiste;
            album.text = stat.album;
            annee.text = stat.annee >0 ? stat.annee : "";
            duree.text = stat.duree >0 ? App.getDuree(stat.duree) : "";
            piste.text = stat.pistes >0 ? stat.pistes : "";
        }
    }

    Connections {
        target: VFService
        onStatAvailable: {

            if(batchId !== batch_id) {
                return;
            }

            taille.text = stat.fileSize > 0 ? App.getTaille(stat.fileSize) : "";
            nbDir.text = stat.dirCount > 0 ? vi18n.get("info.dir").arg(stat.dirCount) : "";
            nbFile.text = stat.fileCount > 0 ? vi18n.get("info.file").arg(stat.fileCount) : "";
        }
    }
}

