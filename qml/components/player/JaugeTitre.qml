import QtQuick 2.5
import Sailfish.Silica 1.0
import "../"
import "../../js/app.js" as App

Item {
    id:jauge

    property var morceau: lecteurService.titre

    property int _index: lecteurService.index   // index du titre en cours
    property int _taille: lecteurService.taille     // taille de la liste
    property int _tempsTotalListe: lecteurService.dureeTotale //totla pistes
    property int _tempsbaseListe: 0                      // deja des titre precedent
    property int _ecouteTitre: 0                    // deja ecouté de la piste

    width: parent.width
    height: fond.height + compteur.height

    //-----------------------------------------
    function mettreAJourInfoListe() {
        // on recalcule le temps de base

        var l = lecteurService.liste;
        var tm=0;
        for(var i=0 ; i<l.length ; i++ ) {
            if(i===_index) {
                break;
            }
            tm+=l[i].duree;
        }
        _tempsbaseListe=tm;
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
                try {
                    if( vurl !== morceau.chemin ) {
                        return;
                    }
                    fond.source = artPath;
                }catch(e) {}
            }

            onArtNotFound : {
                try {
                    if( vurl !== morceau.chemin ) {
                        return;
                    }
                    fond.source = "image://muuzik/img/son?"+ Theme.primaryColor
                }catch(e) {}
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
    CompteurTitre {
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

        mettreAJourInfoListe();
    }

    Connections {
        target:lecteurService

        onDureeChanged:{
//            if(jauge.visible===false) {
//                return;
//            }

            _ecouteTitre = value;
        }

        onTitreChanged: {
            //   _tempsTitre = t.duree;
            _ecouteTitre = 0;

            // on reinitialise la pochette au cas on la prochaine ne soit pas trouvé
//            fond.source = "image://muuzik/img/son?" + Theme.primaryColor
            ARTService.getArtAsync(t.chemin);

            // on peut avoir "sauté" un titre, il faut recalculer
            mettreAJourInfoListe();
        }

        onListeChanged: {
            mettreAJourInfoListe();
        }
    }
}
