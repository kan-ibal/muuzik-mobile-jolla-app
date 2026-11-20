
//--------------------------------------
var getDescriptionPourAlbum=function(groupe, temps, cd, cf) {

    var c = 0;
    c += cd>0 ? 2 : 0;
    c += cf>0 ? 4 : 0;
    c += temps>0 ? 8 : 0
    c += groupe.length>0 ? 16 : 0

    var t="";
    if( temps>0)  {
        t = App.getDuree(temps) + " - ";
    }

    switch(c) {
    case 2 :    // seulement des reps
        return vi18n.get("navigateur.rep.sub").arg(t).arg(cd);

    case 4 :    // seulement des medias
    case 12 : // media et temps
        return vi18n.get("navigateur.rep.album").arg(t).arg(cf);

    case 6 : // media et repertoires, pas plus d'info
    case 14: // media rep et temps
        return vi18n.get("navigateur.rep.description").arg(t).arg(cd).arg(cf);

    case 20 : // media et nom de groupe
    case 28 : // medias, groupe et temps
    case 30 : // media, rep, temp et groupe
        return vi18n.get("navigateur.rep.metainfo").arg(t).arg(cf).arg(groupe);

    case 0 :
        return vi18n.get("navigateur.rep.vide");
    }

    return "";
}

//--------------------------------------
var getDescriptionPourTitre=function(album, groupe, annee, temps) {

    var c = 0;
    c += album.length > 0 ? 2 : 0;
    c += groupe.length > 0 ? 4 : 0;
    c += annee > 0 ? 8 : 0;
    c += temps > 0 ? 16 : 0;

    var t= "";
    if( temps>0)  {
        t = App.getDuree(temps) + " - ";
    }

    switch(c) {
    case 2 : // seulement album
        return album;

    case 4 : // seulement groupe
        return groupe;

    case 18 : // album + temp
        return vi18n.get("navigateur.audio.album.simple").arg(t).arg(album);

    case 20 :
        return vi18n.get("navigateur.audio.titre.simple").arg(t).arg(groupe);

    case 40 : //toutes les info
    case 32 : // pas d'annee
        return vi18n.get("navigateur.audio.description").arg(t).arg(album).arg(annee).arg(groupe);
    }

    return "";
}
