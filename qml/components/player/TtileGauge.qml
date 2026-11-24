import QtQuick 2.5
import Sailfish.Silica 1.0
import "../"
import "../../js/app.js" as App

Item {
    id:jauge

    property var morceau: playerService.title

    property int _index: playerService.index   // index of the current title
    property int _taille: playerService.size     // size of the list
    property int _tempsTotalListe: playerService.dureeTotale // total runs
    property int _tempsbaseListe: 0                      // as mentioned in the previous section
    property int _ecouteTitre: playerService.getPosition()                    // already listened to the track

    width: parent.width
    height: fond.height + Theme.horizontalPageMargin + compteur.height

    //-----------------------------------------
    function mettreAJourInfoListe() {
        // we recalculate the base time
        var l = playerService.list;
        var tm = 0;
        var idx = playerService.index;
        if (l && idx > 0) {
            var limit = Math.min(idx, l.length);
            for (var i = 0; i < limit; i++) {
                if (l[i]) {
                    tm += l[i].duree;
                }
            }
        }
        _tempsbaseListe = tm;
    }

    ImageRound {
        id:fond
     //   visible: morceau
        source: "image://muuzik/img/son?"+ Theme.primaryColor // chargé dynamiquement
        width:  parent.width * 0.8
        height: width
        anchors {
            horizontalCenter: parent.horizontalCenter
        }

        Connections {
            target: ARTService
            onArtFound : {
                if (!morceau || vurl !== morceau.chemin) {
                    return;
                }
                fond.source = artPath;
            }

            onArtNotFound : {
                if (!morceau || vurl !== morceau.chemin) {
                    return;
                }
                fond.source = "image://muuzik/img/son?" + Theme.primaryColor;
            }
        }

        // la jauge de temps ecoulé
        ProgressCircle {
            value: jauge._tempsTotalListe > 0 ? (jauge._tempsbaseListe + jauge._ecouteTitre) / jauge._tempsTotalListe : 0

            progressColor:Theme.highlightColor
            backgroundColor: Theme.primaryColor

            width:parent.height
            height:width
            anchors {
                horizontalCenter: parent.horizontalCenter
                verticalCenter: parent.verticalCenter
            }

            // la jauge du nombre de titre
            ProgressCircle {
                value: jauge._taille > 0 ? (jauge._index+1) / jauge._taille : 0;

                progressColor:Theme.highlightColor
                backgroundColor: Theme.primaryColor
                width:parent.height *0.9
                height:width
                anchors {
                    horizontalCenter: parent.horizontalCenter
                    verticalCenter: parent.verticalCenter
                }
            }

            // les valeurs numeriques
            Text {
                anchors {
                    horizontalCenter: parent.horizontalCenter
                    verticalCenter: parent.verticalCenter
                }
                color: Theme.highlightColor

                font.pixelSize: Theme.fontSizeHuge
                font.bold: true
                text: App.getDuree(jauge._tempsbaseListe + jauge._ecouteTitre) +
                      "\n\n" + App.getDuree(jauge._tempsTotalListe)
            }
            Rectangle {
                anchors {
                    horizontalCenter: parent.horizontalCenter
                    verticalCenter: parent.verticalCenter
                }
                color: Theme.highlightColor
                width: Theme.fontSizeMedium *3
                height: Theme.paddingSmall
            }
        }
    }

    // le compteur de titre
    TitleCounter {
        id: compteur
        anchors {
            horizontalCenter: parent.horizontalCenter
            top:fond.bottom
            topMargin: Theme.horizontalPageMargin
        }
    }

    Component.onCompleted: {
        if(morceau) {
            ARTService.getArtAsync(morceau.chemin);
        }

        _ecouteTitre = playerService.getPosition();
        mettreAJourInfoListe();
    }

    onVisibleChanged: {
        if (visible) {
            _ecouteTitre = playerService.getPosition();
            mettreAJourInfoListe();
        }
    }

    Connections {
        target:playerService

        onDureeChanged:{
//            if(jauge.visible===false) {
//                return;
//            }

            _ecouteTitre = value;
        }

        onTitleChanged: {
            //   _tempsTitre = t.duree;
            _ecouteTitre = playerService.getPosition();

            // on reinitialise la pochette au cas on la prochaine ne soit pas trouvé
//            fond.source = "image://muuzik/img/son?" + Theme.primaryColor
            if (t) {
                ARTService.getArtAsync(t.chemin);
            }

            // on peut avoir "sauté" un titre, il faut recalculer
            mettreAJourInfoListe();
        }

        onListChanged: {
            mettreAJourInfoListe();
        }
    }
}
