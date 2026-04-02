import QtQuick 2.2
import Sailfish.Silica 1.0

Label {
    property bool position: false
    color: Theme.secondaryHighlightColor
    font.pixelSize: Theme.fontSizeLarge
    anchors.left: position === false ? parent.left : undefined
    anchors.right: position ? parent.right : undefined
    anchors.margins: Theme.paddingLarge
}
