import QtQuick 2.2
import Sailfish.Silica 1.0
import Nemo.Notifications 1.0
import "../js/app.js" as App

Dialog {
    canAccept:playerService.dureeTotale> 0;

    Column {
        width: parent.width

        DialogHeader {
            title: vi18n.get("nuit.titre")
        }

        SectionHeader { text: vi18n.get("nuit.temps.titre") }

        Slider {
            label: vi18n.get("nuit.temps.limite")
            minimumValue: 0
            maximumValue: Math.max(60, playerService.dureeTotale)
            enabled: playerService.dureeTotale > 0
            stepSize: 60
            width: parent.width
            value: nightModeService.endTime
            valueText: vi18n.get("nuit.temps.label").arg(App.getDuree(value));
            onValueChanged: {
                if(nightModeService.endTime===value) {
                    return;
                }
                nightModeService.endTime=value;
            }
        }

        Text {
            visible: playerService.dureeTotale<=0
            width: parent.width
            text:  vi18n.get("nuit.temps.inconnu")
            wrapMode: Text.WordWrap
            color: Theme.highlightColor
        }

        SectionHeader { text: vi18n.get("nuit.volume.titre") }
        TextSwitch {
            text:  vi18n.get("nuit.volume")
            description: vi18n.get("nuit.volume.desc")
            Component.onCompleted: checked = nightModeService.reductionVolume
            onCheckedChanged: nightModeService.reductionVolume=checked
        }

        Slider {
            visible: nightModeService.reductionVolume
            label: vi18n.get("nuit.volume.minimal")
            minimumValue: 0
            maximumValue: Math.max(1, VolumeService.stepCount)
            enabled: VolumeService.stepCount > 0
            stepSize: 1
            width: parent.width
            value: nightModeService.finalVolume
            valueText: vi18n.get("nuit.volume.label").arg(
                          VolumeService.stepCount >0 ? value * (100 /VolumeService.stepCount) : 0 );

            onValueChanged: {
                if(nightModeService.finalVolume===value) {
                    return;
                }

                var v=VolumeService.currentStep;
                if(value>v) {
                    value = v;
                    notification.publish();
                }

                nightModeService.finalVolume=value;
            }
        }

        Notification {
                id: notification
                previewBody: vi18n.get("nuit.alert.volumetrophaut");
                icon: "image://theme/icon-s-high-importance?" + Theme.primaryColor
            }
    }

    onAccepted: {
        nightModeService.start();
    }
}
