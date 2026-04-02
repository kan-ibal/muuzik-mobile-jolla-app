import QtQuick 2.2
import Sailfish.Silica 1.0
import"../components"

Page {
    id:page
    width: root.width

    function rechargerDefault() {
        ConfigService.parDefaut();
    }

    SilicaFlickable {
        contentWidth: parent.width
        contentHeight: c.height
        anchors.fill: parent

        PullDownMenu {
            MenuItem {
                text:  vi18n.get("menu.apropos")
                onClicked: pageStack.push(Qt.resolvedUrl("Apropos.qml"))
            }
            MenuItem {
                text:  vi18n.get("options.default")
                onClicked: rechargerDefault()
            }
        }

        VerticalScrollDecorator { flickable: parent }

        Column {
            width: parent.width
            anchors {
                left: parent.left
                right: parent.right
                margins:Theme.horizontalPageMargin
            }

            spacing: Theme.paddingSmall
            id:c

            PageHeader {
                title: vi18n.get("options.titre")
                id:ph
            }

            SectionHeader { text:  vi18n.get("options.sdcard") }

            TextField {
                //    focus: true
                enabled: SdcardLocator.present
                text: ConfigService.nomsdcard
                placeholderText: vi18n.get("options.sdcard.nom")
                label: vi18n.get("options.sdcard.nom.desc")
                width: parent.width

                inputMethodHints: Qt.ImhNoAutoUppercase | Qt.ImhNoPredictiveText
                EnterKey.onClicked: {
                    focus = false;
                }
                onFocusChanged: {
                    if(focus ===false) {
                        ConfigService.setNomsdcard(text);
                    }
                }
            }

            SectionHeader { text: vi18n.get("options.navigation") }
            TextSwitch {
                text:  vi18n.get("options.navigation.vide")
                description: vi18n.get("options.navigation.vide.info")
                Component.onCompleted: checked = ConfigService.showEmptyDir
                onCheckedChanged:ConfigService.showEmptyDir=checked
            }

            TextSwitch {
                text:  vi18n.get("options.navigation.marge")
                description: vi18n.get("options.navigation.marge.info")
                Component.onCompleted: checked = ConfigService.marge
                onCheckedChanged:ConfigService.marge=checked
            }

            TextSwitch {
                text:  vi18n.get("options.navigation.metadata")
                description: vi18n.get("options.navigation.metadata.info")
                Component.onCompleted: checked = ConfigService.dirTag
                onCheckedChanged:ConfigService.dirTag=checked
            }


            ComboBox {
                id:diralbum
                width: parent.width
                label:  vi18n.get("options.navigation.album")
                currentIndex :ConfigService.dirAlbum
                menu: ContextMenu {
                    MenuItem { text:  vi18n.get("options.navigation.album.ouvrir") }
                    MenuItem { text:  vi18n.get("options.navigation.album.jouer") }
                    MenuItem { text:  vi18n.get("options.navigation.album.ajouter") }
                    MenuItem { text:  vi18n.get("options.navigation.album.menu") }
                }
                onCurrentIndexChanged:{
                    if(currentIndex !== ConfigService.dirAlbum) {
                        ConfigService.dirAlbum=currentIndex;
                    }
                }
            }

            ComboBox {
                width: parent.width
                label:  vi18n.get("options.navigation.albumsub")
                currentIndex :ConfigService.dirAlbumWithSub
                menu: ContextMenu {
                    MenuItem { text:  vi18n.get("options.navigation.album.ouvrir") }
                    MenuItem { text:  vi18n.get("options.navigation.album.jouer") }
                    MenuItem { text:  vi18n.get("options.navigation.album.ajouter") }
                    MenuItem { text:  vi18n.get("options.navigation.album.menu") }
                }
                onCurrentIndexChanged:{
                    if(currentIndex !== ConfigService.dirAlbumWithSub) {
                        ConfigService.dirAlbumWithSub=currentIndex;
                    }
                }
            }

            SectionHeader { text:  vi18n.get("options.lecteur") }
            TextSwitch {
                text:  vi18n.get("options.lecteur.auto")
                description:   vi18n.get("options.lecteur.auto.info")
                Component.onCompleted: checked = ConfigService.autoStart
                onCheckedChanged: ConfigService.autoStart=checked
            }

            TextSwitch {
                text:  vi18n.get("options.lecteur.controle")
                description:   vi18n.get("options.lecteur.controle.info")
                Component.onCompleted: checked = ConfigService.controle
                onCheckedChanged: ConfigService.controle=checked
            }

            TextSwitch {
                text:  vi18n.get("options.lecteur.pochette")
                description:   vi18n.get("options.lecteur.pochette.info")
                Component.onCompleted: checked = ConfigService.voirPochette
                onCheckedChanged: ConfigService.voirPochette=checked
            }

            Slider {
                id:opacite
                visible: ConfigService.voirPochette
                label: vi18n.get("options.lecteur.opacite")
                minimumValue: 30
                maximumValue: 100
                stepSize: 10
                //  valueText: sliderValue
                width: parent.width
                value: ConfigService.opacitePochette

                onValueChanged: {
                    if(ConfigService.opacitePochette===value) {
                        return;
                    }
                    ConfigService.opacitePochette=value;
                }
            }

            SectionHeader { text:  vi18n.get("options.art") }
            TextSwitch {
                width: parent.width
                text:  vi18n.get("options.art.telechargement")
                description: vi18n.get("options.art.telechargement.desc")
                Component.onCompleted: checked = ConfigService.pochette
                onCheckedChanged: ConfigService.pochette=checked
            }
            TextSwitch {
                width: parent.width
                text:  vi18n.get("options.art.wifi")
                description: vi18n.get("options.art.wifi.desc")
                Component.onCompleted: checked = ConfigService.pochetteWifi
                onCheckedChanged: ConfigService.pochetteWifi=checked
            }
            TextSwitch {
                width: parent.width
                text:  vi18n.get("options.art.notification")
                description: vi18n.get("options.art.notification.desc")
                Component.onCompleted: checked = ConfigService.pochetteFeedback
                onCheckedChanged: ConfigService.pochetteFeedback=checked
            }

            SectionHeader { text:  vi18n.get("options.bio") }
            TextSwitch {
                width: parent.width
                text:  vi18n.get("options.bio.lastfm")
                description: vi18n.get("options.bio.lastfm.desc")
                Component.onCompleted: checked = ConfigService.lastfm
                onCheckedChanged: ConfigService.lastfm=checked
            }
            TextSwitch {
                width: parent.width
                text:  vi18n.get("options.bio.paroledirect")
                description: vi18n.get("options.bio.paroledirect.desc")
                Component.onCompleted: checked = ConfigService.paroledirect
                onCheckedChanged: ConfigService.paroledirect=checked
            }
            TextSwitch {
                visible: ConfigService.paroledirect===false
                width: parent.width
                text:  vi18n.get("options.bio.paroleexterne")
                description: vi18n.get("options.bio.paroleexterne.desc")
                Component.onCompleted: checked = ConfigService.paroleexterne
                onCheckedChanged: ConfigService.paroleexterne=checked
            }
            TextSwitch {
                width: parent.width
                text:  vi18n.get("options.bio.cache")
                description: vi18n.get("options.bio.cache.desc")
                Component.onCompleted: checked = ConfigService.pasdecache
                onCheckedChanged: ConfigService.pasdecache=checked
            }

            SectionHeader { text:  vi18n.get("options.reseau") }
            TextSwitch {
                width: parent.width
                text:  vi18n.get("options.reseau.wifi")
                description: vi18n.get("options.reseau.wifi.desc")
                Component.onCompleted: checked = ConfigService.wifi
                onCheckedChanged: ConfigService.wifi=checked
            }

            SectionHeader { text:  vi18n.get("options.historique") }
            TextSwitch {
                width: parent.width
                text:  vi18n.get("options.historique.actif")
                description: vi18n.get("options.historique.actif.desc")
                Component.onCompleted: checked = ConfigService.historique
                onCheckedChanged: ConfigService.historique=checked
            }

            ComboBox {
                width: parent.width
                label:  vi18n.get("options.historique.duree")
                visible: ConfigService.historique
                currentIndex :ConfigService.modeHistorique
                menu: ContextMenu {
                    MenuItem { text:  vi18n.get("options.historique.duree.semaine") }
                    MenuItem { text:  vi18n.get("options.historique.duree.mois") }
                    MenuItem { text:  vi18n.get("options.historique.duree.toujours") }
                }
                onCurrentIndexChanged:{
                    if(currentIndex !== ConfigService.modeHistorique) {
                        ConfigService.modeHistorique=currentIndex;
                    }
                }
            }

            SectionHeader { text:  vi18n.get("options.recent") }
            ComboBox {

                width: parent.width
                label:  vi18n.get("options.recent.nb")
                currentIndex :  versIndex(configService.nbRecent)
                menu: ContextMenu {
                    MenuItem { text:  "50" }
                    MenuItem { text:  "100" }
                    MenuItem { text:  "250" }
                    MenuItem { text:  vi18n.get("options.recent.nb.all") }
                }
                onCurrentIndexChanged:{
                    var v=depuisIndex(currentIndex);

                    if(v !== ConfigService.nbRecent) {
                        ConfigService.nbRecent=v;
                        recentService.recharger();
                    }
                }

                function versIndex(v){
                    switch(v) {
                    case 100:return 1;
                    case 250:return 2;
                    case 10000:return 3;
                    }
                    return 0;
                }

                function depuisIndex(v){
                    switch(v) {
                    case 1:return 100;
                    case 2:return 250;
                    case 3:return 10000;
                    }
                    return 50;
                }
            }

            Text {
                width: parent.width
                color: Theme.primaryColor
                font.pixelSize: Theme.fontSizeMedium
                text:  vi18n.get("options.recent.description")
                wrapMode: Text.WordWrap
            }
            Button {
                text: vi18n.get("options.refresh")
                anchors {
                    horizontalCenter: parent.horizontalCenter
                    topMargin: Theme.horizontalPageMargin
                }
                enabled: recentService.enCours === false
                onClicked: recentService.recharger()
            }

            SectionHeader { text:  vi18n.get("options.recherche") }

            Text {
                width: parent.width
                color: Theme.primaryColor
                font.pixelSize: Theme.fontSizeMedium
                text:  vi18n.get("options.recherche.description")
                wrapMode: Text.WordWrap
            }
            Button {
                text: vi18n.get("options.refresh")
                anchors {
                    horizontalCenter: parent.horizontalCenter
                    topMargin: Theme.horizontalPageMargin
                }
                enabled: IndexeurService.enCours === false
                onClicked: IndexeurService.recharger()
            }

            // hack, c'est moche mais ça marche
            Item {
                width: parent.width
                height: Theme.horizontalPageMargin
            }
        }
    }
}

