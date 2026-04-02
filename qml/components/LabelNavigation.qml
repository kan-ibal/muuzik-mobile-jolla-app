import QtQuick 2.2
import Sailfish.Silica 1.0

Item {
    id: main
    height: txt.height + desc.height + padding

    property alias titre: txt.text
    property alias description: desc.text
    property real padding: Theme.paddingSmall/2
    property string horizontalAlignment : Text.AlignLeft
    property int elide : Text.ElideRight
    property bool highlighted: true

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
        color: highlighted ? Theme.highlightColor : Theme.primaryColor
        elide: main.elide
        anchors {
            left: icone.length >0 ? ico.right : parent.left
            right:parent.right
        }
    }

    Label {
        id: desc
        horizontalAlignment: horizontalAlignment
        color: highlighted ? Theme.secondaryHighlightColor : Theme.secondaryColor
        font.pixelSize: Theme.fontSizeExtraSmall
        elide: main.elide
        visible: text.length>0

        anchors {
            top:txt.bottom
            topMargin: padding
            left: parent.left
            right:parent.right
        }

        Behavior on text {
            PropertyAnimation {
                id: anim
                target: desc
                property: "opacity"; from:0; to: 1; duration: 400;
            }
        }

    }
}

