import QtQuick 2.5
import Sailfish.Silica 1.0

Canvas {
    id: image

    antialiasing: true

    width:parent.width;

    property string source;

    property string _PreviousSource;         // url de l'image precedement chargé

    //-----------------------------
    function chargementImage() {

        if(isImageLoaded(source)) {
            _PreviousSource = source;
            requestPaint();
            return;
        }

        if( isImageLoading(source)) {
            return;
        }

        loadImage(source);
    }

    onPaint: {
        // pas de source ou pas chargé (en chargement / erreur)
        if( (source.length ===0) || (visible === false) || (available === false)) {
            return;
        }

        if( isImageLoading(source) && (_PreviousSource.length === 0)) {
            return;
        }

        var ctx = getContext("2d");

        ctx.save();

        ctx.beginPath();
        ctx.arc( width/2, width/2, width/2, 0, 2 * Math.PI, false);
        ctx.stroke();
        ctx.clip();

        try {
            ctx.clearRect(0, 0, width, width);
            ctx.drawImage( isImageLoaded(source) ?  source : _PreviousSource, 0,0,width,width);
        } catch(e) {
        }
        ctx.restore();
    }

    onVisibleChanged: {
        if(visible === false) {
            return;
        }
        requestPaint();
    }

    onImageLoaded: {
        if(isImageLoaded(_PreviousSource)) {
            unloadImage(_PreviousSource);
        }

        _PreviousSource = source;
        requestPaint();
    }

    onSourceChanged: {
        if(available===false) {
            return;
        }

        chargementImage();
    }

    onAvailableChanged: {
        if(available===false) {
            return;
        }
        chargementImage();
    }

}
