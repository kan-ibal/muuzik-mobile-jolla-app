import QtQuick 2.2
import Sailfish.Silica 1.0
import "../components"

Page {
    id:page
    property bool active: status===PageStatus.Active
    property string chemin:""
    property alias artiste : tartiste.text
    property alias album : talbum.text
    property alias titre : ttitre.text
    property bool direct:false


    function setTitre(t) {
        if (!t) {
            artiste = "";
            album = "";
            titre = "";
            chemin = "";
            return;
        }

        var streamArt = playerService.streamArtist;
        var streamTit = playerService.streamTitle;
        if (playerService.isStream && (streamArt.length > 0 || streamTit.length > 0)) {
            artiste = streamArt;
            titre = streamTit.length > 0 ? streamTit : t.titre;
        } else {
            artiste = t.artiste;
            titre = t.titre;
        }
        album = t.album;
        chemin = t.chemin;
    }

    SilicaFlickable {
        anchors.fill: parent

        // form
        Item {
            anchors.fill: parent
            Column {
                width: parent.width

                PageHeader{
                    title: vi18n.get("info.form.header")
                }

                TextField {
                    id:tartiste
                    label: vi18n.get("info.form.artiste")
                    placeholderText: vi18n.get("info.form.vide")
                    width: parent.width
                }
                TextField {
                    visible: false
                    id:talbum
                    label: vi18n.get("info.form.album")
                    placeholderText: vi18n.get("info.form.vide")
                    width: parent.width
                }
                TextField {
                    id:ttitre
                    label: vi18n.get("info.form.titre")
                    placeholderText: vi18n.get("info.form.vide")
                    width: parent.width
                }
            }
            Column {
                anchors.bottom: parent.bottom
                width: parent.width

                SectionLink {
                    name: vi18n.get("info.voir.bio")
                    enabled: artiste.length >0
                    onClicked: pageStack.push(Qt.resolvedUrl("Biographie.qml"),
                                              {artiste:artiste})
                }
                SectionLink {
                    name: vi18n.get("info.voir.paroles")
                    enabled: titre.length >0
                    onClicked: pageStack.push(Qt.resolvedUrl("Parole.qml"),
                                              {chemin:chemin,artiste:artiste,titre:titre})
                }
            }
        }
    }

    onStatusChanged: {
        if ((status === PageStatus.Activating || status === PageStatus.Active) && direct === false) {
            setTitre(playerService.title);
        }
    }

    Connections {
        target:playerService

        onTitleChanged: {
            if (status!==PageStatus.Active || direct===true) {
                return;
            }

            setTitre(t);
        }

        onStreamMetaDataChanged: {
            if (status !== PageStatus.Active || direct === true) {
                return;
            }

            setTitre(playerService.title);
        }
    }
}
