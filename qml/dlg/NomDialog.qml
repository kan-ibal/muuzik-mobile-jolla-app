import QtQuick 2.2
import Sailfish.Silica 1.0

Dialog {
    id:dlg
    property alias valeur : n.text
    property alias titre: t.text
    property alias label: n.label
    property string dif: ""
    property int mini :1

    Column {
        width: parent.width

        DialogHeader { }

        SectionHeader { id:t }

        TextField {
            id: n
            width: parent.width
            focus:true

            EnterKey.enabled: dlg.canAccept
            EnterKey.onClicked: {
                Qt.inputMethod.commit()
                if (dlg.canAccept && dlg.status === PageStatus.Active && !pageStack.busy) {
                    dlg.accept()
                }
            }
        }
    }

    canAccept: (n.text.length >= dlg.mini) && (n.text !== dif)
}
