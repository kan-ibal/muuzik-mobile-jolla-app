import QtQuick 2.2
import Sailfish.Silica 1.0
import Nemo.Notifications 1.0
import "../js/app.js" as App

Dialog {
    canAccept:lecteurService.dureeTotale> 0;

    Column {
        width: parent.width

        DialogHeader {
            title: vi18n.get("nuit.titre")
        }

        SectionHeader { text: vi18n.get("nuit.temps.titre") }

        Slider {
            label: vi18n.get("nuit.temps.limite")
            minimumValue: 0
            maximumValue: lecteurService.dureeTotale
            stepSize: 60
            width: parent.width
            value: modeNuitService.tempsFinal
            valueText: vi18n.get("nuit.temps.label").arg(App.getDuree(value));
            onValueChanged: {
                if(modeNuitService.tempsFinal===value) {
                    return;
                }
                modeNuitService.tempsFinal=value;
            }
        }

        Text {
            visible: lecteurService.dureeTotale<=0
            width: parent.width
            text:  vi18n.get("nuit.temps.inconnu")
            wrapMode: Text.WordWrap
            color: Theme.highlightColor
        }

        SectionHeader { text: vi18n.get("nuit.volume.titre") }
        TextSwitch {
            text:  vi18n.get("nuit.volume")
            description: vi18n.get("nuit.volume.desc")
            Component.onCompleted: checked = modeNuitService.reductionVolume
            onCheckedChanged: modeNuitService.reductionVolume=checked
        }

        Slider {
            visible: modeNuitService.reductionVolume
            label: vi18n.get("nuit.volume.minimal")
            minimumValue: 0
            maximumValue: VolumeService.stepCount
            stepSize: 1
            width: parent.width
            value: modeNuitService.volumeFinal
            valueText: vi18n.get("nuit.volume.label").arg(
                          VolumeService.stepCount >0 ? value * (100 /VolumeService.stepCount) : 0 );

            onValueChanged: {
                if(modeNuitService.volumeFinal===value) {
                    return;
                }

                var v=VolumeService.currentStep;
                if(value>v) {
                    value = v;
                    notification.publish();
                }

                modeNuitService.volumeFinal=value;
            }
        }

        Notification {
                id: notification
                previewBody: vi18n.get("nuit.alert.volumetrophaut");
                icon: "image://theme/icon-s-high-importance?" + Theme.primaryColor
            }
    }

    onAccepted: {
        modeNuitService.demarrer();
    }
}
