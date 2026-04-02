import QtQuick 2.2
import Sailfish.Silica 1.0
import "../components"
import "../hints"
import "../js/app.js" as App
import org.vostok.virtualfiles 1.0
import org.vostok.metadata 1.0

Page {
    id:page
    property bool active: status===PageStatus.Active
    property string actuel: ""
    property bool peutvide: false

    //-----------------------------------------
    function openUri(v) {
        console.log("opening --> " + v)
        pageStack.push(Qt.resolvedUrl("Navigateur.qml"), {uri:v})
    }

    //-----------------------------------------
    function ecouter(v,r) {
        r = typeof r !== 'undefined' ? r : true;
        lecteurService.remplacer([v],r);
        pageStack.push(Qt.resolvedUrl("ListeLecture.qml"));
    }

    //-----------------------------------------
    function ajouter(v) {
        pageStack.push(Qt.resolvedUrl("AjouterA.qml"), {url:v})
    }

    //-----------------------------------------
    function ajouterdirect(v) {
        lecteurService.ajouter([v],false);
        pageStack.push(Qt.resolvedUrl("ListeLecture.qml"));
    }

    Component {
        id: itemHolder

        ListItem {
            menu: contextMenu
            width: ListView.view.width
            contentHeight: Math.max(img.height,description.height) +
                           (ConfigService.marge ? Theme.paddingSmall : 0)

            Miniature {
                id: img
                anchors {
                    left: parent.left
                    leftMargin: Theme.horizontalPageMargin
                    verticalCenter: parent.verticalCenter
                }
            }

            LabelPlus {
                id:description
                anchors {
                    left: img.right
                    leftMargin: Theme.horizontalPageMargin
                    right: parent.right
                    rightMargin: Theme.horizontalPageMargin
                    verticalCenter: parent.verticalCenter
                }
                text:name
                value: ""
                highlighted: false;
            }

            Component.onCompleted: {
                switch(type) {
                    //---------------
                case VFInfo.DIRECTORY :
                    img.source = "image://theme/icon-m-folder?" + Theme.primaryColor;
                    ARTService.getArtAsync(vurl);

                    MetaDataService.getStatFromAlbumASynch(vurl);
                    break;

                    //---------------
                case VFInfo.FILE :
                    img.source = "image://muuzik/img/son?"+Theme.primaryColor;

                    MetaDataService.getMetaDataASynch(vurl);
                    break;
                }
            }

            Connections {
                target: ARTService
                onArtFound : {
                    if( model.vurl !== vurl) {
                        return;
                    }

                    img.overlay = artPath;
                }
            }

            Connections {
                target: MetaDataService

                onMetaDataAvailable: {
                    if(model.vurl !== vurl) {
                        return;
                    }

                    var t= "";
                    if(meta.duree>0) {
                        t = App.getDuree(meta.duree) + " - ";
                    }

                    if(meta.artiste.length===0 && meta.album.length===0) {
                        description.value= vi18n.get("navigateur.audio.vide").arg(t);
                        return;
                    }

                    if(meta.album.length===0) {
                        description.value= vi18n.get("navigateur.audio.description.simple").arg(t).arg(meta.artiste);
                        return;
                    }

                    if(meta.artiste.length===0) {
                        description.value= vi18n.get("navigateur.audio.description.simple").arg(t).arg(meta.album);
                        return;
                    }

                    description.value= vi18n.get("navigateur.audio.description").arg(t).arg(meta.artiste).arg(meta.album);
                }

                onStatAvailable: {
                    if(model.vurl !== vurl) {
                        return;
                    }

                    description.value = time > 0 ?
                                vi18n.get("favoris.description").arg(App.getDuree(stat.duree)).arg(stat.pistes) :
                                vi18n.get("favoris.description.simple").arg(stat.pistes)
                }
            }

            onClicked: {
                console.log("va " + vurl)

                switch(type) {
                case VFInfo.DIRECTORY :
                    ecouter(vurl);
                    break;
                case  VFInfo.FILE :
                    ajouterdirect(vurl);
                    break;
                }

            }

            Component {
                id: contextMenu
                ContextMenu {
                    MenuItem {
                        text:  vi18n.get("navigateur.rep.voir")
                        onClicked: openUri( type === VFInfo.DIRECTORY ? vurl : App.getRepParent(vurl) )
                    }
                    MenuItem {
                        text:  vi18n.get("navigateur.rep.ajouter")
                        onClicked: ajouter(vurl)
                    }
                    MenuItem {
                        text:  vi18n.get("navigateur.rep.ecouter")
                        onClicked:  ecouter(vurl)
                    }
                }
            }
        }
    }

    SilicaListView {
        id:li
        anchors.fill: parent
        clip: true
        contentWidth: parent.width
        focus: visible
        visible: IndexeurService.enCours===false && IndexeurService.dbok

        model: RechercherService.liste

        VerticalScrollDecorator { flickable: parent }

        PullDownMenu {
            MenuItem {
                text:  vi18n.get("menu.config")
                onClicked: {pageStack.push(Qt.resolvedUrl("Configuration.qml"));}
            }
            MenuItem {
                text:  vi18n.get("recherche.miseajour")
                onClicked: {
                    actuel="";
                    IndexeurService.recharger();
                }
            }
            MenuItem {
                text:  vi18n.get("menu.enlecture")
                onClicked: {
                    pageStack.push(Qt.resolvedUrl("ListeLecture.qml"));
                }
            }
        }

        header: SearchField {
            id:sch
            width: parent.width
            placeholderText: vi18n.get("recherche.titre")
            label: vi18n.get("recherche.titre")
            focus:true

            inputMethodHints: Qt.ImhNoAutoUppercase

            EnterKey.onClicked:fn_rech()
            EnterKey.enabled: text.length>=3
            EnterKey.iconSource: "image://theme/icon-m-enter-accept?" + Theme.primaryColor

            onTextChanged: {
                peutvide=false;
                if(text.length==0) {
                    RechercherService.vider();
                    peutvide=false;
                    autorech.stop();
                    return;
                }
                autorech.restart();
            }

            Timer {
                id: autorech
                running: false
                interval: 1500
                onTriggered: fn_rech()
            }

            function fn_rech() {
                autorech.stop();
                if(IndexeurService.enCours===true) {
                    console.log('en cours');
                    return;
                }

                if(text === actuel) {
                    return;
                }
                actuel = text;

                peutvide=false;
                RechercherService.rechercher(text);
            }

            Label {
                text:vi18n.get("recherche.vide")
                visible: RechercherService.liste.length===0 && peutvide
                color: Theme.highlightColor
                anchors {
                    horizontalCenter: parent.horizontalCenter
                    top:parent.bottom
                    topMargin: Theme.paddingLarge
                }
            }
        }

        currentIndex: -1

        delegate: itemHolder
    }

    Hint {
        id:hin1
        clee : "/vostok/muuzik/hint/recherche"
        texte: vi18n.get("hint.recherche")
        direction: TouchInteraction.Down
        interactionMode :TouchInteraction.Pull
        debug: false
    }

    Component.onCompleted: {
        hin1.lancer();
    }

    // info init db
    Dbinit {
        visible: IndexeurService.enCours===false && IndexeurService.dbok===false
        onButtonClicked: IndexeurService.recharger()
    }

    Attente {
        enabled: IndexeurService.enCours || status===PageStatus.Activating
    }

    Connections {
        target: RechercherService
        onListeChanged : {
            peutvide=true;
        }
    }
}
