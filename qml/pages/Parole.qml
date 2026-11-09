import QtQuick 2.2
import Sailfish.Silica 1.0
import Sailfish.WebView 1.0
import Sailfish.WebEngine 1.0
import Sailfish.WebView.Popups 1.0
import "../components"

Page {
    id:page
    property bool active: status===PageStatus.Active
    property bool areseau: networkwatcher.wifi || (ConfigService.wifi===false && networkwatcher.wcdma)
    property bool done:false;
    property string artiste: "";
    property string titre: "";
    property string chemin: "";

    function recherche() {
        if(artiste.length===0||titre.length===0) {
            done=true;
            return;
        }

        vph.enabled=false;

        var tmp= ParoleService.chercher(chemin,artiste,titre);
        if(tmp.length!==0) {
            done=true;

            // verif si c'est une url
            if(tmp.indexOf("http")===0) {
                console.log("ok url : " +tmp);

                if(ConfigService.paroleexterne) {
                    texte.text=vi18n.get("parole.externe");
                    Qt.openUrlExternally(tmp );
                    return;
                }

                main.visible=false;
                webLoader.targetUrl = tmp;
                webLoader.active = true;
                webLoader.visible = true;
                if (webLoader.item) {
                    webLoader.item.url = tmp;
                }
                return;
            }

            texte.text=tmp;
            return;
        }

        if(areseau===false) {
            // va pas plus loin, pas de reseau
            vre.enabled=true;
            done=true;
            return;
        }
    }

    SilicaFlickable {
        id:main
        visible: true
        anchors.fill: parent
        clip: true;
        contentHeight: parole.height
        contentWidth: parent.width

        PullDownMenu {

            MenuItem {
                text:  vi18n.get("menu.config")
                onClicked: {pageStack.push(Qt.resolvedUrl("Configuration.qml"));}
            }

            MenuItem {
                text:  vi18n.get("parole.effacer")
                enabled: texte.text.length>0
                onClicked: {
                    ParoleService.effacer(chemin);
                    pageStack.navigateBack();
                }
            }
        }

        VerticalScrollDecorator {flickable:parent}

        // parole
        Column {
            id:parole

            anchors {
                left: parent.left;
                right: parent.right;
            }

            PageHeader {
                title: titre
            }

            Text {
                id: texte
                text: ""
                width: parent.width-(Theme.horizontalPageMargin *2)
                x:Theme.horizontalPageMargin
                color: Theme.highlightColor
                wrapMode: Text.WordWrap
                elide: Text.ElideRight
                font.pixelSize: Theme.fontSizeSmall
                textFormat: Text.PlainText
            }


        }

        // info reseau
        ViewPlaceholder {
            id:vre
            enabled: false
            text:  vi18n.get("info.reseau.titre")
            hintText:  vi18n.get("info.reseau.desc")
        }

        // pas de contenu
        ViewPlaceholder {
            id:vph
            enabled: false
            text:  vi18n.get("parole.aucune.titre")
            hintText:  vi18n.get("parole.aucune.sum")
        }
    }

    Loader {
        id: webLoader
        anchors.fill: parent
        visible: false
        active: false
        property string targetUrl: ""
        onLoaded: {
            if (item && targetUrl.length > 0) {
                item.url = targetUrl;
            }
        }
        sourceComponent: Component {
            WebView {
                anchors.fill: parent
            }
        }
    }

    Component.onDestruction: {
        if (webLoader.item) {
            webLoader.item.url = "about:blank";
        }
        webLoader.active = false;
    }

    Attente {
        enabled: (ParoleService.enCours || status===PageStatus.Activating) && (done === false)
    }

    onStatusChanged: {
        if (status===PageStatus.Active && done ===false) {
            recherche();
        }
    }

    Connections {
        target:ParoleService
        onTrouve:{
            if (status!==PageStatus.Active || done === true) {
                return;
            }

            done=true;
            if( donnee !== undefined && donnee.length >0 ) {
                texte.text=donnee;
                return;
            }

            // pas de texte
            vph.enabled=true;
        }
    }

    Connections {
        target:networkwatcher
        onConnectedChanged : {
            if(areseau===false || done===true) {
                return;
            }

            vre.enabled=false;
            recherche();
        }
    }
}
