import QtQuick 2.5
import Sailfish.Silica 1.0
import "../../js/app.js" as App

// le compteur de titre
Label {
    id: compteur

    property int _index: playerService.index   // index du titre en cours
    property int _taille: playerService.size     // taille de la liste

    text: _taille > 0 ? (_index+1) + " / " + _taille : "---";

    font.pixelSize: Theme.fontSizeSmall
}
