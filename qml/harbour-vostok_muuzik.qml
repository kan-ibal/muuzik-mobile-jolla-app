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
        enabled: true
        key: Qt.Key_MediaNext
        onPressed : {
            console.log("Key_MediaNext")
            lecteurService.suivant()
        }
    }

    MediaKey {
        id: mediaPrev
        enabled: true
        key: Qt.Key_MediaPrevious
        onPressed : {
            console.log("Key_MediaPrevious")
            lecteurService.precedent()
        }
    }

    MediaKey {
        id: mediaPlay
        enabled: true
        key: Qt.Key_MediaTogglePlayPause
        onPressed : {
            console.log("Key_MediaTogglePlayPause")
            lecteurService.lire()
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
         lecteurService.restaurerSession();
    }

    Component.onDestruction: {
        lecteurService.sauverSession();
    }
}


