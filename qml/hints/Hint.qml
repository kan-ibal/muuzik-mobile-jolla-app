import QtQuick 2.0
import Sailfish.Silica 1.0
import Nemo.Configuration 1.0

Loader {
    id:comp
    anchors.fill: parent
    anchors.centerIn: parent
    active: counter.actif && _lancement

    signal fin
    signal debut

    property bool debug:false

    property alias clee: counter.key
    property string texte
    property bool texteHaut:false

    property int direction: TouchInteraction.Right
    property int interactionMode :TouchInteraction.Swipe
    property real startx: parent.width/2
    property real starty: parent.height/2

    property int _limite:1
    property bool _lancement:false

    //----------------------------------------
    function lancer() {
        if(counter.actif===false) {
            fin();
            return;
        }

        _lancement=true;
    }

    //----------------------------------------
    function limiteAtteinte() {
        return counter.actif;
    }

    sourceComponent: Component {
        Item {
            property bool pageActive: status === PageStatus.Active && comp.active
            onPageActiveChanged: {
                if (pageActive) {
                    debut();
                    timer.restart();
                    counter.increase();
                    pageActive = false;
                }
            }

            anchors.fill: parent
            anchors.centerIn: parent

            Timer {
                id: timer
                interval: 500
                onTriggered: touchInteractionHint.restart();
            }

            InteractionHintLabel {
                id: ht
                text: comp.texte
                anchors.bottom: comp.texteHaut ? undefined : parent.bottom
                anchors.top: comp.texteHaut ? parent.top : undefined
                opacity: touchInteractionHint.running ? 1.0 : 0.0
                Behavior on opacity { FadeAnimation { duration: 1000 } }
            }

            TouchInteractionHint {
                id: touchInteractionHint
                direction: comp.direction
                interactionMode: comp.interactionMode
                anchors {
                    horizontalCenter: undefined
                    verticalCenter: undefined
                }

                startX: comp.startx - (width/2)
                startY: comp.starty - (height/2)
                x:startX
                y:startY

                onRunningChanged: {
                    if(running===false) {
                        comp._lancement=false;
                        fin();
                    }
                }
            }
        }
    }

    ConfigurationValue {
        id:counter
        defaultValue: 0

        property bool actif: value<_limite

        function increase() {
            if(comp.debug) {
                return;
            }
            value +=1;
            value = value >=_limite ? _limite : value;
        }
    }
}
