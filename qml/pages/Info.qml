import QtQuick 2.2
import Sailfish.Silica 1.0
import "../composents"

Page {
    id:page
    property bool active: status===PageStatus.Active
    property string chemin:""
    property alias artiste : tartiste.text
    property alias album : talbum.text
    property alias titre : ttitre.text
    property bool direct:false


    function setTitre(t) {
        if(t=== undefined) {
            return;
        }

        artiste=t.artiste;
        album=t.album;
        titre=t.titre;
        chemin=t.chemin;
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

                ButtonLayout {
//                SectionLink {
                  Button {
//                    name: vi18n.get("info.voir.bio")
                    text: vi18n.get("info.voir.bio")
                    enabled: artiste.length >0
                    onClicked: pageStack.push(Qt.resolvedUrl("Biographie.qml"),
                                              {artiste:artiste})
                }
//                SectionLink {
                  Button {
//                    name: vi18n.get("info.voir.paroles")
                    text: vi18n.get("info.voir.paroles")
                    enabled: titre.length >0
                    onClicked: pageStack.push(Qt.resolvedUrl("Parole.qml"),
                                              {chemin:chemin,artiste:artiste,titre:titre})
                }
            }
            }
        }
    }

    onStatusChanged: {

        if (status===PageStatus.Active&&direct===false) {
            setTitre(lecteurService.getTitre());
        }
    }

    Connections {
        target:lecteurService

        onTitreChanged: {
            if (status!==PageStatus.Active || direct===true) {
                return;
            }

            setTitre(t);
        }
    }
}
