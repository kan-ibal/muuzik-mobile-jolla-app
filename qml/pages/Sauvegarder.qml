import QtQuick 2.2
import Sailfish.Silica 1.0
import "../js/app.js" as App
import "../components"

Page {
    id:page
    property bool active: status===PageStatus.Active
    property var url
    property bool loaded: false

    //-----------------------------------------
    function recharger() {
        listModel.remplir(PlaylistService.liste);
    }

    //-----------------------------------------
    function retour() {
        pageStack.completeAnimation();
        pageStack.navigateBack();
    }

    Component {
        id: entree

        ListItem {
            contentHeight: Theme.itemSizeLarge

            Rectangle {
                anchors.fill: parent
                color:  "transparent"
                Image {
                    id:img
                    source: icone
                    height: Theme.iconSizeLarge
                    width: height
                    anchors{
                        leftMargin: Theme.horizontalPageMargin
                        verticalCenter: parent.verticalCenter
                    }
                }

                Label {
                    anchors{
                        left: img.right
                        right: parent.right
                        leftMargin: Theme.paddingLarge
                        rightMargin: Theme.horizontalPageMargin
                        verticalCenter: parent.verticalCenter
                    }
                    text: label
                    font.pixelSize: Theme.fontSizeMedium
                    //elide: Text.ElideRight
                    wrapMode: Text.WordWrap
                }
            }

            onClicked: {
                switch(type) {

                    // on cree une nouvelle playlist
                case 1 :
                    var dialog = pageStack.push(Qt.resolvedUrl("../dlg/NomDialog.qml"),
                                                {
                                                    "valeur": "",
                                                    "titre":  vi18n.get("playlists.sauve.titre"),
                                                    "label": vi18n.get("playlists.sauve.label")
                                                });
                    dialog.accepted.connect(function() {
                        var uuid = PlaylistService.creer(dialog.valeur);
                        if( uuid === -1 ) {
                            return;
                        }

                        PlaylistService.ecrireAsync(uuid,url);

                        pageStack.completeAnimation();
                        pageStack.navigateBack(PageStackAction.Immediate);
                        pageStack.navigateBack();
                    });
                    break;

                case 2 :
                    PlaylistService.ecrireAsync(uuid,url);
                    pageStack.navigateBack();
                    break;
                }
            }
        }

    }

    ListModel {
        id:listModel

        //-----------------------------------------
        function remplir(l) {
            clear();

            // pour ajouter a une nouvelle liste
            append({
                       icone:"image://muuzik/img/nouvellepl?" + Theme.primaryColor,
                       label:vi18n.get("ajouter.nouvelle"),
                       type:1
                   });

            // pour ajouter aux listes existantes
            for(var i in l) {
                append({
                           icone:"image://muuzik/img/ajouterpl?" + Theme.primaryColor,
                           label:l[i].nom,
                           uuid:l[i].uuid,
                           type:2
                       });
            }
        }
    }

    SilicaListView {
        id: fileList
        anchors{
            top:parent.top
            left: parent.left
        }

        height: parent.height
        width: parent.width
        clip: true
        visible: loaded && listModel.count>0

        model:  listModel

        VerticalScrollDecorator { flickable: fileList }

        PullDownMenu {
            MenuItem {
                text:  vi18n.get("menu.config")
                onClicked: {pageStack.push(Qt.resolvedUrl("Configuration.qml"));}
            }
        }

        header: PageHeader {
            title: vi18n.get("lecteur.sauver")
        }

        delegate: entree

        ViewPlaceholder {
            enabled: loaded && listModel.count===0
            text: vi18n.get("playlists.vide")
            hintText: vi18n.get("playlists.vide.sum")
        }
    }

    onStatusChanged: {
        // wait for view to load
        if (status===PageStatus.Active && loaded === false ) {
            recharger();
            loaded = true;
        }
    }

    Attente {
        enabled: loaded === false || status===PageStatus.Activating
    }

    Component.onCompleted: {
        // on recharge la liste des playlists
        recharger();

    }


}
