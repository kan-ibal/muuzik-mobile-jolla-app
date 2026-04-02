import QtQuick 2.2
import Sailfish.Silica 1.0
import Nemo.Notifications 1.0
import "../components"
import "../hints"
import "../js/app.js" as App
import "../js/affichage.js" as Affichage

import org.vostok.virtualfiles 1.0
import org.vostok.metadata 1.0


Page {
    id: page
    property bool active: status===PageStatus.Active

    property string uri: "/"       // base uri for the "view", defautl to "root"
    property string filter: "\\.(mp3|ogg|flac|m4a|m4b|wav|aif|aiff|opus|alac)$"
    property real batch_id : 0      // batch_id for data on asynch call
    property bool loaded: false

    property real _nbaudio:0        // nombre de media dans la vue actuelle

    property string actionVurl:"";
    property bool enAction: actionVurl && actionVurl.length>0

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
    function ajouterFavoris(index) {
        var item =listModel.get(index);
        listModel.setProperty(index, "favoris", FavorisService.ajouter(item.uri));
    }

    //-----------------------------------------
    function supprimerFavoris(index) {
        FavorisService.supprimer(listModel.get(index).favoris);
        listModel.setProperty(index, "favoris",-1);
    }

    //-----------------------------------------
    function actionBp(item) {
        switch(item.type) {
            //---------------
        case VFInfo.DIRECTORY :
            if( item.countDir ===0 && item.countFile !== 0) {
                switch(ConfigService.dirAlbum) {
                case 0 :
                    openUri(item.uri);
                    return true;
                case 1 :
                    ecouter(item.uri);
                    return true;
                case 2 :
                    ajouter(item.uri);
                    return true;
                }
                break;
            }

            if(item.countDir !== 0 && item.countFile !== 0 ) {
                switch(ConfigService.dirAlbumWithSub) {
                case 0 :
                    openUri(item.uri);
                    return true;
                case 1 :
                    ecouter(item.uri);
                    return true;
                case 2 :
                    ajouter(item.uri);
                    return true;
                }
                break;
            }

            openUri(item.uri);
            return true;
        }

        // on ouvre le menu
        return false;
    }

    //-----------------------------------------
    function debutAction(vurl) {
        if(enAction || remorsepop.visible) {
            return;
        }

        actionVurl = vurl;
    }
    //-----------------------------------------
    function finAction() {
        actionVurl = "";
    }

    //-----------------------------------------
    function outilsAction(code) {
        switch(code) {
        case "info" :
            infoSelection();
            break;
        case "bio" :
            bioSelection();
            break;
        case "supprimer" :
            supprimerSelection();
            break;
        case "renommer" :
            renommerSelection();
            break;
        case "deplacer" :
            deplacerSelection();
            break;
        default :
            finAction();
        }
    }

    //-----------------------------------------
    function infoSelection() {

        var tmp = actionVurl
        finAction();

        if( VFService.isDirectory(tmp) ) {
            pageStack.push(Qt.resolvedUrl("InfoRep.qml"), {uri:tmp});
            return;
        }

        pageStack.push(Qt.resolvedUrl("InfoMedia.qml"), {uri:tmp});
    }

    //-----------------------------------------
    function bioSelection() {

        var tmp = actionVurl
        finAction();

        var m = MetaDataService.getMetaDataAsQVariant(tmp,true);
        if(m) {
            pageStack.push(Qt.resolvedUrl("Info.qml"),
                           {direct:true,chemin:tmp,artiste:m.artiste,titre:m.titre});
            return;
        }

        var nom = App.getNomSimple(actionVurl);
        pageStack.push(Qt.resolvedUrl("Info.qml"),
                       {direct:true,chemin:tmp,artiste:nom,titre:nom})
    }

    //-----------------------------------------
    function deplacerSelection() {

        var dialog = pageStack.push(Qt.resolvedUrl("../dlg/SelectionDestination.qml"),
                                    {
                                        "uri": "/",
                                        "filter": "",
                                        "type": VFEnum.DIR
                                    });

        dialog.accepted.connect(function() {

            // on peut déplacer les médias dans n'importe quel rep,
            // les rep eux pas vers un rep child !
            if( VFService.isDirectory(actionVurl) ) {
                if(VFService.isParent(actionVurl, dialog.selection )) {
                    notification.publish();
                    finAction();
                    return;
                }
            }

            if(VFService.getParentVUrl(actionVurl) === dialog.selection) {
                notification.publish();
                finAction();
                return;
            }

            VFService.move(actionVurl, dialog.selection);

            //      pageStack.completeAnimation();
            //    pageStack.navigateBack(PageStackAction.Immediate);
            //  pageStack.navigateBack();

            finAction();
        });
    }

    //-----------------------------------------
    function renommerSelection() {

        var dialog = pageStack.push(Qt.resolvedUrl("../dlg/NomDialog.qml"),
                                    {
                                        "valeur": VFService.getName(actionVurl),
                                        "titre":  vi18n.get("navigateur.action.renommer")
                                    });

        dialog.accepted.connect(function() {
            var old = VFService.getName(actionVurl);

            if( dialog.valeur !== old && dialog.valeur.length > 0) {
                var target = VFService.getFileName(actionVurl).replace(old, dialog.valeur);
                VFService.rename(actionVurl, target);
            } else {
                notification.publish();
            }

            //        pageStack.completeAnimation();
            //     pageStack.navigateBack(PageStackAction.Immediate);
            //pageStack.navigateBack();

            finAction();
        });
    }

    //----------------------------------------
    function supprimerSelection() {
        if(remorsepop.visible) {
            return;
        }

        remorsepop.execute( "Deleting", function() {

            for(var i =listModel.count-1 ; i>=0;i--) {
                var el =listModel.get(i);

                if(el.uri !== actionVurl ) {
                    continue;
                }

                VFService.remove(el.uri);
                finAction();

                // NOTA : sera sup de la liste par le watcher fichierService/supprimer
                //  GridView.view.model.remove(i);
                // listModel.remove(i);
            }
        });
    }

    ListModel {
        id:outilsModel

        ListElement {
            src:"image://theme/icon-m-document";
            titre:"navigateur.action.bio";
            code:"bio";
        }

        ListElement {
            src:"image://theme/icon-m-delete";
            titre:"navigateur.action.supprimer";
            code:"supprimer";
        }

        ListElement {
            src:"image://theme/icon-m-about";
            titre:"navigateur.action.information";
            code:"info";
        }

        ListElement {
            src:"image://theme/icon-m-transfer";
            titre:"navigateur.action.deplacer";
            code:"deplacer";
        }

        ListElement {
            src:"image://theme/icon-m-edit-selected";
            titre:"navigateur.action.renommer";
            code:"renommer";
        }

    }

    //-------------- debut component
    RemorsePopup { id: remorsepop }

    //-------------- debut component
    Component {
        id: itemHolder

        ListItem {
            id: fileItem
            menu: contextMenu
            width: ListView.view.width
            contentHeight: Math.max(img.height,titre.height) +
                           (ConfigService.marge ? Theme.paddingSmall : 0)

            property string album: ""
            property string groupe: ""
            property int annee: 0
            property int durree: 0
            // countFile ; countDir

            enabled: enAction === false
            highlighted: fileItem.down || actionVurl === uri

            Miniature {
                id: img
                anchors {
                    left: parent.left
                    leftMargin: Theme.horizontalPageMargin
                    verticalCenter: parent.verticalCenter
                }
                source: icone
            }

            LabelNavigation {
                id:titre
                anchors {
                    left: img.right
                    leftMargin: Theme.horizontalPageMargin
                    right: parent.right
                    rightMargin: Theme.horizontalPageMargin
                    verticalCenter: parent.verticalCenter
                }
                titre:name
                description: ""
                icone: favoris >= 0 ? "image://theme/icon-m-favorite-selected?" + Theme.primaryColor : ""
                highlighted: false;
            }

            Component.onCompleted: {
                switch(type) {
                    //---------------
                case VFInfo.DIRECTORY :
                    //   recup la cover
                    if(countFile>0 && countDir===0) {
                        ARTService.getArtAsync(uri);
                    }

                    titre.description = Affichage.getDescriptionPourAlbum("",0,countDir,countFile);

                    // pas de media, pas de stat ou meta (!)
                    if(countFile == 0) {
                        break;
                    }

                    MetaDataService.getStatFromAlbumASynch(uri);

                    // on evite de modifier le nom s'il y a des rep
                    if(countDir > 0) {
                        break;
                    }

                    // est on recup info "album"
                    if(ConfigService.dirTag) {
                        MetaDataService.getMetaDataASynch(uri,true);
                    }
                    break;

                    //---------------
                case VFInfo.FILE :
                    MetaDataService.getMetaDataASynch(uri);
                    break;
                }
            }

            Connections {
                target: ARTService
                onArtFound : {
                    if( vurl !== uri) {
                        return;
                    }

                    img.overlay = artPath;
                }
            }

            Connections {
                target: MetaDataService

                onMetaDataAvailable: {
                    if( uri !== vurl) {
                        return;
                    }

                    switch(type) {
                        //---------------
                    case VFInfo.DIRECTORY :
                        // @INFO pour les albums, on peut fixer la description sur deux callback,
                        //onMetaDataAvailable  et onStatAvailable
                        fileItem.album  = meta.album
                        fileItem.groupe = meta.artiste
                        fileItem.annee = meta.annee

                        titre.description = Affichage.getDescriptionPourAlbum(meta.artiste,fileItem.durree,countDir,countFile);

                        // et le titre
                        if( meta.album.length == 0) {
                            break;
                        }

                        titre.titre = (meta.annee >0 ? meta.annee + " - " : "") + meta.album
                        break;

                    case VFInfo.FILE :
                        titre.description = Affichage.getDescriptionPourTitre(meta.album, meta.artiste, meta.annee, meta.duree);
                        break;
                    }
                }

                onStatAvailable: {
                    if(vurl !== uri) {
                        return;
                    }

                    fileItem.durree = stat.duree;
                    titre.description = Affichage.getDescriptionPourAlbum(fileItem.groupe,stat.duree,countDir,stat.pistes);
                }
            }

            onClicked: {
                console.log("opening : " + listModel.get(index));

                if(actionBp(listModel.get(index))===false) {
                    openMenu();
                }
            }

            Component {
                id: contextMenu
                ContextMenu {
                    MenuItem {
                        visible: type === VFInfo.DIRECTORY
                        text:  vi18n.get("navigateur.rep.voir")
                        onClicked: openUri(uri)
                    }
                    MenuItem {
                        visible: type === VFInfo.DIRECTORY
                        text:  vi18n.get("navigateur.rep.ecouter")
                        onClicked:  ecouter(uri)
                    }
                    MenuItem {
                        visible: type === VFInfo.DIRECTORY
                        text:  vi18n.get("navigateur.rep.ajouter")
                        onClicked: ajouter(uri)
                    }
                    MenuItem {
                        visible: type === VFInfo.DIRECTORY && countFile > 0 && favoris < 1
                        text:  vi18n.get("navigateur.rep.favoris")
                        onClicked: ajouterFavoris(index)
                    }
                    MenuItem {
                        visible: type === VFInfo.DIRECTORY && countFile > 0 && favoris > 0
                        text:  vi18n.get("navigateur.rep.favoris.sup")
                        onClicked: supprimerFavoris(index)
                    }
                    MenuItem {
                        visible: type === VFInfo.FILE
                        text:  vi18n.get("navigateur.audio.ecouter")
                        onClicked:  ecouter(uri)
                    }
                    MenuItem {
                        visible: type === VFInfo.FILE
                        text:  vi18n.get("navigateur.audio.ajouter")
                        onClicked: ajouter(uri)
                    }
                    MenuItem {
                        text: vi18n.get("navigateur.action")
                        onClicked: debutAction(uri)
                    }
                }
            }

        }
    }
    //-------------- fin component

    SilicaListView {
        id: fileList
        anchors.fill: parent
        contentWidth: parent.width
        anchors.bottomMargin: barrebouton.open ? barrebouton.height : 0
        clip: true
        visible: loaded

        model: ListModel {
            id:listModel
        }

        VerticalScrollDecorator { flickable:parent }

        PullDownMenu {
            MenuItem {
                text:  vi18n.get("menu.config")
                onClicked: {pageStack.push(Qt.resolvedUrl("Configuration.qml"));}
            }
            MenuItem {
                text:  vi18n.get("menu.menu")
                onClicked: {
                    pageStack.clear();
                    pageStack.replace(Qt.resolvedUrl("Menu.qml"));
                }
            }
            MenuItem {
                visible: modeNuitService.actif===true
                text:  vi18n.get("menu.nuit.off")
                onClicked: {
                    modeNuitService.arreter();
                }
            }
            MenuItem {
                text:  vi18n.get("menu.enlecture")
                visible: page.orientation === Orientation.Portrait || page.orientation === Orientation.PortraitInverted
                onClicked: {
                    pageStack.push(Qt.resolvedUrl("ListeLecture.qml"));
                }
            }
            MenuItem {
                text:  vi18n.get("navigateur.rep.ajouter")
                enabled: _nbaudio >0
                onClicked: ajouter(uri)
            }
            MenuItem {
                text:  vi18n.get("navigateur.rep.ecouter")
                enabled: _nbaudio >0
                onClicked: ecouter(uri)
            }
        }

        header: PageHeader {
            id:pageTitle
            title: VFService.isRoot(uri) ? vi18n.get("navigateur.racine") : VFService.getName(uri)
        }

        delegate: itemHolder

        ViewPlaceholder {
            id:vph
            enabled: loaded && listModel.count===0
            text:  vi18n.get("navigateur.vide")
            hintText:  vi18n.get("navigateur.vide.sum")
        }
    }

    DockedPanel {
        id: barrebouton
        //       width: Screen.width
        width: parent.width
        height: bpc.height
        dock:  Dock.Bottom
        parent: page.parent
        open: enAction && remorsepop.visible===false

        onOpenChanged: {
            if(!open) {
                return;
            }
            hin2.lancer()
        }

        Column {
            id: bpc
            width: parent.width

            Item {
                height: Theme.itemSizeMedium
                width: parent.width

                IconButton {
                    icon.source: "image://theme/icon-m-close"
                    width: Theme.itemSizeExtraSmall
                    height: width
                    anchors {
                        verticalCenter: parent.verticalCenter
                        right: parent.right
                        rightMargin: Theme.horizontalPageMargin
                    }
                    onClicked: finAction()
                }
            }

            SlideshowView {
                id:outils

                width: parent.width
                itemWidth: width / 4
                height: Theme.iconSizeExtraLarge

                snapMode: PathView.SnapToItem
                currentIndex: 1

                model: outilsModel

                delegate:  BoutonPlus {
                    width: outils.itemWidth
                    height: outils.height

                    icone: src + "?" + Theme.primaryColor
                    label: vi18n.get(titre)
                    fonte: Theme.fontSizeExtraSmall
                    bordure: "transparent"
                    onClicked: outilsAction(code)
                }
            }
        }
    }



    Attente {
        enabled: loaded === false || status===PageStatus.Activating
    }

    HintPress {
        id:hin1
        clee : "/vostok/muuzik/hint/navigateur"
        texte: vi18n.get("hint.navigateur")
    }

    Hint {
        id:hin2
        clee : "/vostok/muuzik/hint/navigateur/tool"
        texte: ""
        direction: TouchInteraction.Swipe
        starty: Screen.height * 0.9
    }

    Notification {
        id: notification
        previewBody: vi18n.get("navigateur.action.error");
        icon: "image://theme/icon-s-high-importance?" + Theme.primaryColor
    }

    onStatusChanged: {
        // wait for view to load
        if (status===PageStatus.Active && loaded === false ) {
            console.log("loading data for " + uri)
            page.batch_id = VFService.get(uri,filter)
            console.log("... our batch is " + page.batch_id)
        }
    }

    Component.onCompleted: {
        hin1.lancer();
    }

    Connections {
        target: VFService

        // listen for change on the list
        onListAvailable:{
            if(loaded) {
                return;
            }

            console.log("data received, batch " + batchId, " expected " + page.batch_id)
            if( batchId != page.batch_id) {
                // not our list
                return;
            }

            var l = [];
            var emp = ConfigService.showEmptyDir;

            for(var i in list) {
                if(list[i].type === VFInfo.DIRECTORY) {
                    var cd = VFService.countDirectories(list[i].vurl);
                    var cf = VFService.countFiles(list[i].vurl,filter);

                    // pas de rep vide
                    if( cd === 0 && cf === 0 && emp === false) {
                        continue;
                    }

                    l.push({
                               name: list[i].name,
                               type: list[i].type,
                               uri: list[i].vurl,
                               puri: App.getRepParent(list[i].vurl),
                               icone: (cf === 0 || cd > 0 ? "image://theme/icon-m-folder?" : "image://muuzik/img/album?") + Theme.primaryColor,
                               //art: ta,
                               countDir: cd,
                               countFile: cf,
                               favoris: FavorisService.getFavoris(list[i].vurl)
                           });

                    continue;
                }

                // un media audio
                l.push({
                           name: list[i].name,
                           type: list[i].type,
                           uri: list[i].vurl,
                           icone: "image://muuzik/img/son?" + Theme.primaryColor,
                           favoris: -1 // test fait pour toutes les lignes : evitons une erreur
                       });
                _nbaudio++;
            }
            listModel.clear();
            listModel.append(l);
            loaded = true;
        }

        onRemoved: {
            for(var i =0; i < listModel.count ; i++) {
                var tmp = listModel.get(i);
                if(tmp.uri === vurl) {

                    listModel.remove(i);
                    return;
                }
            }
        }

        onMoved: {
            // on recharge la liste, plus simple
            loaded = false;
            page.batch_id = VFService.get(uri,filter)
        }

        onRenamed: {
            // on recharge la liste, plus simple
            loaded = false;
            page.batch_id = VFService.get(uri,filter)
        }
    }
}

