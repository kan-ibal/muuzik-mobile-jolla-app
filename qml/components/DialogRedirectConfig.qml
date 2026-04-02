import QtQuick 2.2
import Sailfish.Silica 1.0

Dialog {

    acceptDestination: Qt.resolvedUrl("../pages/Configuration.qml")
    acceptDestinationAction: PageStackAction.Replace

    Column {
        width: parent.width

        DialogHeader {
            title: vi18n.get("options.niveau.titre")
        }

        Text {
            x:Theme.horizontalPageMargin
            width: parent.width
            text: vi18n.get("options.niveau.desc")
            color: Theme.highlightColor
            wrapMode: Text.WordWrap
        }
    }

    Component.onCompleted: ConfigService.setNiveauConfigOk();
}
