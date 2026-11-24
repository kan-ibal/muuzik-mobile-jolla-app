import QtQuick 2.2
import Sailfish.Silica 1.0

Dialog {

    Column {
        width: parent.width

        DialogHeader {
            title: vi18n.get("tag.titre")
        }

        Text {
            x:Theme.horizontalPageMargin
            width: parent.width
            text: vi18n.get("options.niveau.desc")
            color: Theme.highlightColor
            wrapMode: Text.WordWrap
        }
    }

    Component.onCompleted: {}
}
