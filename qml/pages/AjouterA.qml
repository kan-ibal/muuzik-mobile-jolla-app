import QtQuick 2.2
import Sailfish.Silica 1.0
import "../js/app.js" as App
import "../components"

Page {
    id:page
    property bool active: status===PageStatus.Active
    property var url
    property string filter: "\\.(mp3|ogg|flac|m4a|m4b|wav|aif|aiff|opus|alac)$"

    property real batch_id : 0      // batch_id for data on asynch call
    property bool aulecteur:true
    property bool loaded: false

    property var listUrl : []       // on peu passer un rep ou une media seul
    // la liste est rempli sur onComplete

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
                color: type === 0 ? Theme.secondaryColor : "transparent"
                anchors.fill: parent
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
                    font.bold: type === 0 ? true : false
                    font.pixelSize: type === 0 ? Theme.fontSizeLarge : Theme.fontSizeMedium
                    //elide: Text.ElideRight
                    wrapMode: Text.WordWrap
                }
            }

            onClicked: {
                switch(type) {
                case 0 :
                    lecteurService.ajouter(listUrl);
                    pageStack.navigateBack();
                    break;

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

                        PlaylistService.ajouterAsync(uuid,listUrl);

                        pageStack.completeAnimation();
                        pageStack.navigateBack(PageStackAction.Immediate);
                        pageStack.navigateBack();
                    });
                    break;

                case 2 :
                    PlaylistService.ajouter(uuid,listUrl);
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
            if(aulecteur) {
                append({
                           icone:"image://muuzik/img/son?"+ Theme.primaryColor,
                           label:vi18n.get("ajouter.lecteur"),
                           type:0
                       });
            }
            // pour ajouter a une nouvelle liste
            append({
                       icone:"image://muuzik/img/nouvellepl?"+ Theme.primaryColor,
                       label:vi18n.get("ajouter.nouvelle"),
                       type:1
                   });

            // pour ajouter aux listes existantes
            for(var i in l) {
                append({
                           icone:"image://muuzik/img/ajouterpl?"+ Theme.primaryColor,
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
        visible: loaded

        model:  listModel

        VerticalScrollDecorator { flickable: fileList }

        PullDownMenu {
            MenuItem {
                text:  vi18n.get("menu.config")
                onClicked: {pageStack.push(Qt.resolvedUrl("Configuration.qml"));}
            }
        }

        header: PageHeader {
            title: vi18n.get("ajouter.titre")
        }

        delegate: entree
    }

    Attente {
        enabled: loaded === false || status===PageStatus.Activating
    }

    Component.onCompleted: {
        // on recharge la liste des playlists
        recharger();

        // et on prepare la liste de media
        if(VFService.isFile(url)) {
            listUrl.push(url);
            loaded=true;
            return;
        }

        // c'est un rep, recherche les media
        batch_id = VFService.get(url, filter, VFService.FILE);
    }

    Connections {
        target: VFService

        // listen for change on the list
        onListAvailable:{
            if( batchId != page.batch_id) {
                return;
            }

            for(var i in list) {
                listUrl.push(list[i].vur);
            }

            loaded=true;
        }
    }
}
