import QtQuick 2.2
import Sailfish.Silica 1.0

Item {
    id: labelplus
    height:(txt.visible?txt.height:0) + (desc.visible?desc.height:0) + padding

    property alias text: txt.text
    property alias value: desc.text
    property real padding: Theme.paddingSmall/2
    property string horizontalAlignment : Text.AlignLeft
    property int elide : Text.ElideRight
    property bool highlighted: true

    property alias texte: txt
    property alias description: desc

    property string icone :""

    Image {
        id: ico
        anchors {
            left: parent.left
            rightMargin: Theme.paddingSmall
            verticalCenter: txt.verticalCenter
        }
        width: Theme.iconSizeSmall
        height:Theme.iconSizeSmall
        source: icone
        visible: icone.length >0
    }

    Label {
        id: txt
        horizontalAlignment: horizontalAlignment
        color: labelplus.highlighted ? Theme.highlightColor : Theme.primaryColor
        elide: labelplus.elide
        anchors {
            left: icone.length >0 ? ico.right : parent.left
			leftMargin: icone.length >0 ? Theme.paddingSmall : 0
			right:parent.right
        }
        visible: text.length>0
    }

    Label {
        id: desc
        horizontalAlignment: horizontalAlignment
        color: labelplus.highlighted ? Theme.secondaryHighlightColor : Theme.secondaryColor
        font.pixelSize: Theme.fontSizeExtraSmall
        elide: labelplus.elide
        anchors {
            top:txt.bottom
            topMargin: padding
            left: parent.left
            right:parent.right
        }
        visible: text.length>0

        Behavior on text {
            PropertyAnimation {
                id: anim
                target: desc
                property: "opacity"; from:0; to: 1; duration: 400;
            }
        }

    }
}

