import QtQuick 2.0
import Sailfish.Silica 1.0
import Sailfish.Media 1.0
import Nemo.Policy 1.0

import "pages"
import "cover"
import "components"
import "dlg"

ApplicationWindow
{
    id:app
    
    initialPage: Component {
        Menu {}
//        InfoMedia {
//            uri:"/_nouveau2/Esben And The Witch - Older Terrors (2016)/01. Sylvan.mp3"
//        }

//        InfoRep {
//            uri:"/_nouveau2/Wild Belle/2016 - Dreamland"
//        }


    }

    cover: Qt.resolvedUrl("cover/Cover.qml")
    
    allowedOrientations: Orientation.All
    _defaultPageOrientations: Orientation.All

    MediaKey {
        id: mediaNext
        enabled: app.applicationActive
        key: Qt.Key_MediaNext
        onPressed : {
            console.log("Key_MediaNext")
            playerService.next()
        }
    }

    MediaKey {
        id: mediaPrev
        enabled: app.applicationActive
        key: Qt.Key_MediaPrevious
        onPressed : {
            console.log("Key_MediaPrevious")
            playerService.previous()
        }
    }

    MediaKey {
        id: mediaTogglePlayPause
        enabled: app.applicationActive
        key: Qt.Key_MediaTogglePlayPause
        onPressed : {
            console.log("Key_MediaTogglePlayPause")
            playerService.play()
        }
    }

    MediaKey {
        id: mediaPlay
        enabled: app.applicationActive
        key: Qt.Key_MediaPlay
        onPressed : {
            console.log("Key_MediaPlay")
            if (!playerService.status) {
                playerService.play()
            }
        }
    }

    MediaKey {
        id: mediaPause
        enabled: app.applicationActive
        key: Qt.Key_MediaPause
        onPressed : {
            console.log("Key_MediaPause")
            if (playerService.status) {
                playerService.play()
            }
        }
    }

    MediaKey {
        id: mediaStop
        enabled: app.applicationActive
        key: Qt.Key_MediaStop
        onPressed : {
            console.log("Key_MediaStop")
            playerService.stop()
        }
    }

    Permissions {
        enabled: app.applicationActive
        autoRelease: true
        applicationClass: "player"

        Resource {
            id: headsetAccessResource
            type: Resource.HeadsetButtons
            optional: true
        }
    }

    Component.onCompleted: {
         playerService.restoreSession();
    }

    Component.onDestruction: {
        playerService.saveSession();
    }
}


