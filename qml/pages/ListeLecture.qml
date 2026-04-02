import QtQuick 2.2
import Sailfish.Silica 1.0
import "../js/app.js" as App
import "../components"
import "../hints"
import org.vostok.virtualfiles 1.0
import org.vostok.metadata 1.0

Page {
    id:page

    property bool active: status===PageStatus.Active
    property var morceau: lecteurService.titre
    property int wvuemettre: 0
    property bool ainfo:false

    //-----------------------------------------
    function ajouter(v) {
        pageStack.push(Qt.resolvedUrl("AjouterA.qml"), {url:v,aulecteur:false});
    }

    //-----------------------------------------
    function sauver(v) {
        pageStack.push(Qt.resolvedUrl("Sauvegarder.qml"), {url:v});
    }

    //-----------------------------------------
    function retirer(u) {
        lecteurService.retirerUuid(u);
    }

    Component{
        id: fileItem

        ListItem {
            menu: contextMenu
            width: ListView.view.width
            contentHeight: listtitre.height + listinfo.height+ Theme.horizontalPageMargin

            property bool _estactif : lecteurService.index === index
            highlighted: false

            Rectangle {
                id:fonditem
                anchors.fill: parent;
                color: "transparent"
                opacity: 0

                Rectangle {
                    anchors.fill: parent;
                    color: Theme.highlightBackgroundColor
                    opacity: Theme.highlightBackgroundOpacity

                    Rectangle {
                        width: parent.width
                        height: Theme.paddingSmall
                        color: Theme.secondaryHighlightColor
                        anchors {
                            bottom: parent.bottom
                            left: parent.left
                        }
                        visible: _estactif

                        Rectangle {
                            width:wvuemettre
                            height: Theme.paddingSmall
                            color: Theme.highlightColor
                            anchors {
                                left: parent.left
                            }
                            visible: _estactif
                        }
                    }
                }

                states: [
                    State { when: _estactif;
                        PropertyChanges {   target: fonditem; opacity: 1.0 }
                    },
                    State { when: _estactif === false;
                        PropertyChanges {   target: fonditem; opacity: 0.0    }
                    }
                ]
                transitions: Transition {
                    NumberAnimation { property: "opacity"; duration: 1000}
                }
            }

            Item {
                id: listnb

                width: Math.max(bpcontrole.width,nombre.width)
                anchors {
                    left: parent.left
                    leftMargin: Theme.horizontalPageMargin
                    verticalCenter: parent.verticalCenter
                }

                Image {
                    id:bpcontrole
                    visible:  _estactif
                    source:  (lecteurService.etat ? "image://theme/icon-m-pause?" :"image://theme/icon-m-play?") + Theme.primaryColor
                    width: Theme.iconSizeSmallPlus
                    height: Theme.iconSizeSmallPlus
                    cache: false
                    anchors {
                        centerIn: parent
                    }
                }

                Label {
                    id:nombre
                    anchors {
                        centerIn: parent
                    }
                    font.bold: true
                    text: index +1
                    visible:  _estactif === false
                }
            }

            Label {
                id: listtitre
                anchors {
                    left: listnb.right
                    right: listdurree.left
                    leftMargin: Theme.horizontalPageMargin
                    rightMargin: Theme.horizontalPageMargin
                    bottomMargin: Theme.paddingTiny
                }
                text: titre
                elide: Text.ElideRight
            }

            Label {
                id: listinfo
                anchors {
                    left: listnb.right
                    right: listdurree.left
                    top:listtitre.bottom
                    leftMargin: Theme.horizontalPageMargin
                    rightMargin: Theme.horizontalPageMargin
                }
                font.pixelSize: Theme.fontSizeTiny
                text: artiste + " - " + album
                elide: Text.ElideRight
            }

            Label {
                id: listdurree
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                anchors.rightMargin: Theme.horizontalPageMargin
                font.bold: true
                text: App.getDuree(duree)
            }

            onClicked: {
                if(lecteurService.index === index) {
                    lecteurService.lire();
                    return;
                }
                lecteurService.lire(index);
            }

            Component {
                id: contextMenu
                ContextMenu {
                    MenuItem {
                        text: vi18n.get("lecteur.ajouter")
                        onClicked: ajouter(chemin)
                    }

                    MenuItem {
                        text: vi18n.get("lecteur.retirer")
                        onClicked: retirer(uuid)
                    }
                }
            }
        }
    }

    SilicaFlickable {
        anchors.fill: parent
        contentWidth: parent.width

        Image {
            id:fond
            visible: morceau && ConfigService.voirPochette && (controle.panneauOuvert === false)
            source: "image://muuzik/img/son?" + Theme.primaryColor  // chargé dynamiquement
            asynchronous: true
            width: parent.width
            height: parent.width
            sourceSize.width: parent.width
            sourceSize.height: parent.width
            anchors {
                horizontalCenter: parent.horizontalCenter
                top: parent.top
            }

            Connections {
                target: ARTService
                onArtFound : {
                    try {
                        if( vurl !== morceau.chemin ) {
                            return;
                        }
                        fond.source = artPath;
                    }catch(e) {}
                }
            }
        }

        OpacityRampEffect {
            opacity: ConfigService.opacitePochette/100.0
            sourceItem: fond
            direction: OpacityRamp.TopToBottom
            offset:0.5
        }

        SilicaListView {
            id: fileList
            anchors{
                top:parent.top
                left: parent.left
            }
            height: parent.height - controle.height
            width: parent.width
            clip: true
            visible: controle.panneauOuvert === false

            model: lecteurService.liste;

            delegate: fileItem

            header: PageHeader {
                title: morceau && morceau.album.length>0 ? morceau.album : vi18n.get("enlecture.titre")
            }

            VerticalScrollDecorator { flickable: parent }

            PullDownMenu {
                id:menu
                property string  minstr: vi18n.get("menu.nuit.off").arg(0)
                onActiveChanged: {
                    minstr=vi18n.get("menu.nuit.off").arg(App.getDuree(modeNuitService.tempRestant()));
                }

                MenuItem {
                    text:  vi18n.get("menu.config")
                    onClicked: {pageStack.push(Qt.resolvedUrl("Configuration.qml"));}
                }
                MenuItem {
                    visible: modeNuitService.actif===false && lecteurService.liste.length>0
                    text:  vi18n.get("menu.nuit.on")
                    onClicked: {
                        pageStack.push(Qt.resolvedUrl("Minuteur.qml"));
                    }
                }
                MenuItem {
                    visible: modeNuitService.actif===true
                    //                    text:  vi18n.get("menu.nuit.off")
                    text: menu.minstr
                    onClicked: {
                        modeNuitService.arreter();
                    }

                }
                MenuItem {
                    text:  vi18n.get("menu.menu")
                    onClicked: {
                        pageStack.clear();
                        pageStack.replace(Qt.resolvedUrl("Menu.qml"));
                    }
                }
                MenuItem {
                    text:  vi18n.get("menu.enregistrer")
                    enabled:  lecteurService.taille !== 0
                    onClicked: {

                        var u = [];
                        var l = lecteurService.liste;
                        for(var i in l) {
                            u.push(l[i].chemin);
                        }

                        sauver(u);
                    }
                }

                MenuItem {
                    text:  vi18n.get("menu.effacer")
                    enabled:  lecteurService.taille !== 0
                    onClicked: {lecteurService.effacer();}
                }

                MenuItem {
                    text:  vi18n.get("menu.editer")
                    enabled:  lecteurService.taille !== 0
                    onClicked: {pageStack.push(Qt.resolvedUrl("EditerListeLecture.qml"));}
                }
            }
        }

        ControlePlayer {
            id:controle
            panneauOuvert: lecteurService.showControl

            anchors.bottom: parent.bottom
        }

        ViewPlaceholder {
            enabled:  lecteurService.enCours === false && lecteurService.taille === 0 && status === PageStatus.Active
            text:  vi18n.get("enlecture.vide")
            hintText:  vi18n.get("enlecture.vide.sum")
        }
    }

    HintPress {
        id:hin1
        clee : "/vostok/muuzik/hint/controle"
        texte: vi18n.get("hint.controle")
        startx: Screen.width*0.5
        starty: Screen.height - controle.height
        onFin:hin2.lancer()
    }

    Hint {
        id:hin2
        clee : "/vostok/muuzik/hint/listelecture"
        texte: vi18n.get("hint.playlist")
        direction:TouchInteraction.Down
        interactionMode:TouchInteraction.Pull
        onFin:hin3.lancer()
    }

    Hint {
        id:hin3
        clee : "/vostok/muuzik/hint/bio"
        texte: vi18n.get("hint.bio")
        direction:TouchInteraction.Left
        interactionMode:TouchInteraction.Swipe
        onFin:hin4.lancer()
    }

    HintPress {
        id:hin4
        clee : "/vostok/muuzik/hint/context"
        texte: vi18n.get("hint.context")
    }

    Component.onCompleted: {
        hin1.lancer();
    }

    Attente {
        enabled: status===PageStatus.Activating || lecteurService.enCours
    }

    onStatusChanged:  {
        if(status === PageStatus.Active && ainfo == false) {
            ainfo=true;
            pageStack.pushAttached(Qt.resolvedUrl("Info.qml"));

            if(morceau) {
                ARTService.getArtAsync(morceau.chemin);
            }
        }
    }

    Connections {
        target:lecteurService

        onDureeChanged:{
            wvuemettre=Math.ceil( (page.width * value) / (lecteurService.dureeTitre+1) );
        }

        onIndexChanged: {
            wvuemettre=0;
        }

        onTitreChanged: {
            if(ConfigService.voirPochette === false ) {
                return;
            }

            fond.source = "image://muuzik/img/son?" + Theme.primaryColor
            ARTService.getArtAsync(t.chemin);
        }
    }
}
