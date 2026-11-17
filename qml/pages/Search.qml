import QtQuick 2.2
import Sailfish.Silica 1.0
import "../components"
import "../hints"
import "../js/app.js" as App
import pl.kanibal.virtualfiles 1.0
import org.vostok.metadata 1.0

Page {
    id: page
    property bool active: status === PageStatus.Active
    property string currentQuery: ""
    property bool canBeEmpty: false

    //-----------------------------------------
    function openUri(uri) {
        console.log("opening --> " + uri)
        pageStack.push(Qt.resolvedUrl("Navigateur.qml"), { uri: uri })
    }

    //-----------------------------------------
    function playUri(uri, shuffle) {
        shuffle = typeof shuffle !== "undefined" ? shuffle : true
        lecteurService.remplacer([uri], shuffle)
        pageStack.push(Qt.resolvedUrl("ListeLecture.qml"))
    }

    //-----------------------------------------
    function addToPlaylist(uri) {
        pageStack.push(Qt.resolvedUrl("AjouterA.qml"), { url: uri })
    }

    //-----------------------------------------
    function appendAndPlay(uri) {
        lecteurService.ajouter([uri], false)
        pageStack.push(Qt.resolvedUrl("ListeLecture.qml"))
    }

    Component {
        id: resultDelegateComponent

        ListItem {
            id: itemDelegate
            property string itemVurl: vurl
            property string itemName: name
            property int itemType: type

            menu: contextMenu
            width: ListView.view.width
            contentHeight: Math.max(thumbnail.height, descriptionLabel.height) +
                           (ConfigService.marge ? Theme.paddingSmall : 0)

            Miniature {
                id: thumbnail
                anchors {
                    left: parent.left
                    leftMargin: Theme.horizontalPageMargin
                    verticalCenter: parent.verticalCenter
                }
            }

            LabelPlus {
                id: descriptionLabel
                anchors {
                    left: thumbnail.right
                    leftMargin: Theme.horizontalPageMargin
                    right: parent.right
                    rightMargin: Theme.horizontalPageMargin
                    verticalCenter: parent.verticalCenter
                }
                text: itemDelegate.itemName
                value: ""
                highlighted: false
            }

            Component.onCompleted: {
                switch (itemDelegate.itemType) {
                case VFInfo.DIRECTORY:
                    thumbnail.source = "image://theme/icon-m-folder?" + Theme.primaryColor
                    ARTService.getArtAsync(itemDelegate.itemVurl)
                    MetaDataService.getStatFromAlbumASynch(itemDelegate.itemVurl)
                    break

                case VFInfo.FILE:
                    thumbnail.source = "image://muuzik/img/son?" + Theme.primaryColor
                    MetaDataService.getMetaDataASynch(itemDelegate.itemVurl)
                    break
                }
            }

            Connections {
                target: ARTService
                onArtFound: {
                    if (itemDelegate.itemVurl !== vurl) {
                        return
                    }

                    thumbnail.overlay = artPath
                }
            }

            Connections {
                target: MetaDataService

                onMetaDataAvailable: {
                    if (itemDelegate.itemVurl !== vurl || !meta) {
                        return
                    }

                    var durationText = ""
                    if (meta.duree > 0) {
                        durationText = App.getDuree(meta.duree) + " - "
                    }

                    if (meta.artiste.length === 0 && meta.album.length === 0) {
                        descriptionLabel.value = vi18n.get("navigateur.audio.vide").arg(durationText)
                        return
                    }

                    if (meta.album.length === 0) {
                        descriptionLabel.value = vi18n.get("navigateur.audio.description.simple").arg(durationText).arg(meta.artiste)
                        return
                    }

                    if (meta.artiste.length === 0) {
                        descriptionLabel.value = vi18n.get("navigateur.audio.description.simple").arg(durationText).arg(meta.album)
                        return
                    }

                    descriptionLabel.value = vi18n.get("navigateur.audio.description").arg(durationText).arg(meta.artiste).arg(meta.album)
                }

                onStatAvailable: {
                    if (itemDelegate.itemVurl !== vurl || !stat) {
                        return
                    }

                    descriptionLabel.value = stat.duree > 0 ?
                                vi18n.get("favoris.description").arg(App.getDuree(stat.duree)).arg(stat.pistes) :
                                vi18n.get("favoris.description.simple").arg(stat.pistes)
                }
            }

            onClicked: {
                console.log("open " + itemDelegate.itemVurl)

                switch (itemDelegate.itemType) {
                case VFInfo.DIRECTORY:
                    playUri(itemDelegate.itemVurl)
                    break
                case VFInfo.FILE:
                    appendAndPlay(itemDelegate.itemVurl)
                    break
                }
            }

            Component {
                id: contextMenu
                ContextMenu {
                    MenuItem {
                        text: vi18n.get("navigateur.rep.voir")
                        onClicked: openUri(itemDelegate.itemType === VFInfo.DIRECTORY ? itemDelegate.itemVurl : App.getRepParent(itemDelegate.itemVurl))
                    }
                    MenuItem {
                        text: vi18n.get("navigateur.rep.ajouter")
                        onClicked: addToPlaylist(itemDelegate.itemVurl)
                    }
                    MenuItem {
                        text: vi18n.get("navigateur.rep.ecouter")
                        onClicked: playUri(itemDelegate.itemVurl)
                    }
                }
            }
        }
    }

    SilicaListView {
        id: resultsList
        anchors.fill: parent
        clip: true
        contentWidth: parent.width
        focus: visible
        visible: IndexeurService.enCours === false && IndexeurService.dbok

        model: RechercherService.liste

        VerticalScrollDecorator { flickable: parent }

        PullDownMenu {
            MenuItem {
                text: vi18n.get("menu.config")
                onClicked: { pageStack.push(Qt.resolvedUrl("Configuration.qml")) }
            }
            MenuItem {
                text: vi18n.get("recherche.miseajour")
                onClicked: {
                    currentQuery = ""
                    IndexeurService.recharger()
                }
            }
            MenuItem {
                text: vi18n.get("menu.enlecture")
                onClicked: {
                    pageStack.push(Qt.resolvedUrl("ListeLecture.qml"))
                }
            }
        }

        header: SearchField {
            id: searchField
            width: parent.width
            placeholderText: vi18n.get("recherche.titre")
            label: vi18n.get("recherche.titre")
            focus: true

            inputMethodHints: Qt.ImhNoAutoUppercase | Qt.ImhNoPredictiveText

            EnterKey.enabled: true
            EnterKey.iconSource: "image://theme/icon-m-enter-accept?" + Theme.primaryColor
            EnterKey.onClicked: {
                Qt.inputMethod.commit()
                performSearch()
                focus = false
            }

            onTextChanged: {
                canBeEmpty = false
                if (text.length === 0) {
                    currentQuery = ""
                    resultsList.currentIndex = -1
                    RechercherService.vider()
                    canBeEmpty = false
                    autoSearchTimer.stop()
                    return
                }
                autoSearchTimer.restart()
            }

            Timer {
                id: autoSearchTimer
                running: false
                interval: 1500
                onTriggered: performSearch()
            }

            function performSearch() {
                autoSearchTimer.stop()
                if (IndexeurService.enCours === true) {
                    console.log("indexing in progress")
                    return
                }

                var query = text.trim()
                if (query.length === 0 || query === currentQuery) {
                    return
                }
                currentQuery = query

                canBeEmpty = false
                RechercherService.rechercher(query)
            }

            Label {
                text: vi18n.get("recherche.vide")
                visible: (RechercherService.count === 0) && canBeEmpty
                color: Theme.highlightColor
                anchors {
                    horizontalCenter: parent.horizontalCenter
                    top: parent.bottom
                    topMargin: Theme.paddingLarge
                }
            }
        }

        currentIndex: -1

        delegate: resultDelegateComponent
    }

    Hint {
        id: searchHint
        clee: "/vostok/muuzik/hint/recherche"
        texte: vi18n.get("hint.recherche")
        direction: TouchInteraction.Down
        interactionMode: TouchInteraction.Pull
        debug: false
    }

    Component.onCompleted: {
        searchHint.lancer()
    }

    // Database initialization notice
    Dbinit {
        visible: IndexeurService.enCours === false && IndexeurService.dbok === false
        onButtonClicked: IndexeurService.recharger()
    }

    Attente {
        enabled: IndexeurService.enCours || status === PageStatus.Activating
    }

    Connections {
        target: RechercherService
        onListeChanged: {
            canBeEmpty = true
        }
    }
}
