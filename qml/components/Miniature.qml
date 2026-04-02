import QtQuick 2.2
import Sailfish.Silica 1.0

Item {
    id:miniature
    property string source
    property string overlay

    property real taille: Theme.iconSizeLarge
    property real ratio: 1.5

    width: taille
    height: taille

    Image {
        anchors.centerIn: parent

        width: taille
        height:taille

        source: miniature.source
        sourceSize.width: taille
        sourceSize.height: taille

        asynchronous: true
    }

    Image {
        id: img
        anchors.centerIn: parent

        width: taille * ratio
        height:taille * ratio

        source: miniature.overlay
        sourceSize.width: taille
        sourceSize.height: taille

        asynchronous: true
    }

    Behavior on overlay {

        ParallelAnimation {
            NumberAnimation {
                target: miniature
                property: "ratio"
                duration: 850
                from: 0.5
                to: 1

                easing.type: Easing.OutElastic;
                easing.amplitude: 2;
                easing.period: 1.0;
            }

            PropertyAnimation {
                id: anim1
                target: img
                property: "opacity"; from:0; to: 1; duration: 600;
            }
        }
    }
}
