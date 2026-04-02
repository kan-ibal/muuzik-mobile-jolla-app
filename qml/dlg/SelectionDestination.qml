import QtQuick 2.2
import Sailfish.Silica 1.0
import "../components"

import org.vostok.virtualfiles 1.0

Dialog {
    id: page
    property bool active: status===PageStatus.Active
    canAccept: selection.length !== 0

    property string uri: "/"       // base uri for the "view", defautl to "root"
    property string filter: ""      // def : pas de filtre
    property real type : VFEnum.BOTH

    property real batch_id : 0      // batch_id for data on asynch call
    property bool loaded: false

    property string selection:""

    //-----------------------------------------
    function open(item) {

        switch(item.type) {
            //---------------
        case VFInfo.DIRECTORY :
            if(type === VFEnum.BOTH || type === VFEnum.DIR) {
                selection = item.uri;
            }

            loaded = false;
            page.batch_id = VFService.get(item.uri,filter)
            break;

        case VFInfo.FILE :
            selection = item.uri;
            break;
        }

    }

    //-----------------------------------------
    function estImage(v) {
        try {
            var a = /\\.(jpg|jpeg|png|gif)$/i.exec(v);
            if(a.length>0) {
                return true;
            }
        }catch(e) {
        }

        return false;
    }

    //-------------- debut component
    Component {
        id: itemHolder

        ListItem {
            id: fileItem
            width: ListView.view.width

            contentHeight: img.height + (ConfigService.marge ? Theme.paddingSmall : 0)
            highlighted: fileItem.down || selection === uri

            Miniature {
                id: img
                anchors {
                    left: parent.left
                    leftMargin: Theme.horizontalPageMargin
                    verticalCenter: parent.verticalCenter
                }
                source: icone
            }

            Label {
                text: name
                anchors {
                    left: img.right
                    leftMargin: Theme.horizontalPageMargin
                    verticalCenter: parent.verticalCenter

                }
            }


            onClicked:  open(listModel.get(index));
        }
    }
    //-------------- fin component

    DialogHeader {
        id:dlgtitle
    }

    SilicaListView {
        id: fileList
        anchors.top: dlgtitle.bottom
        height: parent.height - dlgtitle.height - (nouveau_rep.visible ? nouveau_rep.height : 0 )
        width: parent.width
        contentWidth: parent.width
        clip: true

        header: PageHeader {
            id:pageTitle
            title: VFService.isRoot(uri) ? vi18n.get("navigateur.racine") : VFService.getName(uri)
        }

        model: ListModel {
            id:listModel
        }

        VerticalScrollDecorator { flickable:parent }

        delegate: itemHolder
    }

    Item {
        id: nouveau_rep

        visible: type != VFEnum.FILE
        width: parent.width
        height: nouveau_rep_bp.height + Theme.horizontalPageMargin * 2
        anchors.top: fileList.bottom

        Button {
            id: nouveau_rep_bp
            text:vi18n.get("navigateur.action.rep")
            anchors {
                centerIn: parent
            }

            onClicked: {
                var dialog = pageStack.push(Qt.resolvedUrl("./NomDialog.qml"),
                                            {
                                                "titre":  vi18n.get("navigateur.action.rep")
                                            });

                dialog.accepted.connect(function() {
                    page.selection = VFService.mkVUrl( page.uri, dialog.valeur );
                    pageStack.completeAnimation();
                    pageStack.navigateBack(PageStackAction.Immediate);
                    page.accept();
                });
            }
        }
    }

    Attente {
        enabled: loaded === false || status===PageStatus.Activating
    }

    onStatusChanged: {
        // wait for view to load
        if (status===PageStatus.Active  && loaded === false ) {

            if(type === VFEnum.BOTH || type === VFEnum.DIR) {
                selection = uri;
            }

            page.batch_id = VFService.get(uri,filter)
        }
    }

    Connections {
        target: VFService

        // listen for change on the list
        onListAvailable:{
            if(loaded) {
                return;
            }

            if( batchId != page.batch_id) {
                // not our list
                return;
            }

            var l = [];

            // le bp de retour
            if( VFService.isRoot(vurl) === false && type !== VFEnum.RECURSIVE ) {
                l.push({
                           name: vi18n.get("up"),
                           uri: VFService.getParentVUrl(vurl),
                           type:VFInfo.DIRECTORY,
                           icone: "image://theme/icon-m-back?" + Theme.primaryColor
                       });
            }

            for(var i in list) {
                var ico = (list[i].type === VFInfo.DIRECTORY ? "image://theme/icon-m-folder?" : "image://muuzik/img/son?") + Theme.primaryColor;
                l.push({
                           name: list[i].name,
                           type: list[i].type,
                           uri: list[i].vurl,
                           icone: estImage(list[i].vurl) ? list[i].vurl : ico
                       });
            }

            listModel.clear();
            listModel.append(l);
            uri = vurl;
            loaded = true;
        }
    }
}
