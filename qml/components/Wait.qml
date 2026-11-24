import QtQuick 2.2
import Sailfish.Silica 1.0

Item {
    x:0
    y:0
    width: Screen.width
    height: Screen.height
    enabled: false
    opacity: enabled ? 1.0 : 0

    Behavior on opacity { FadeAnimation { duration: 300 } }

    BusyIndicator {
        size: BusyIndicatorSize.Large
        anchors.centerIn: parent
        running:enabled
    }
}
