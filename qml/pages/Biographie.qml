import QtQuick 2.2
import Sailfish.Silica 1.0
import "../components"

Page {
    id:page
    property bool lastfmok: ConfigService.lastfm
    property bool areseau: networkwatcher.wifi || (ConfigService.wifi===false && networkwatcher.wcdma)
    property bool done:false
    property string artiste: ""

    property string mbid: ""

    function recherche() {

        if(artiste.length===0) {
            done=true;
            return;
        }

        vph.enabled=false;

        var tmp = LastfmService.biographie(artiste);
        if(tmp.length>0) {
            remplirBio(JSON.parse(tmp));
            done=true;
            return;
        }

        if(areseau===false) {
            // va pas plus loin, pas de reseau
            vre.enabled=true;
            //  done=true;
            return;
        }
    }

    function  remplirBio(d) {
        console.log(d.artist.name + " " + d.artist.mbid);

        mbid=d.artist.mbid;
        titre.title=d.artist.name;
        contenu.text=d.artist.bio.content;
        trombi.source=trouverImg(d.artist.image);
        artistelast.adresse=d.artist.url
        tags.text=tagstr(d.artist.tags.tag);

        var t=d.artist.similar.artist;
        for(var i in t) {
            similaire_model.append({
                                       "name":t[i].name,
                                       "url":t[i].url,
                                       "img":trouverImg(t[i].image)
                                   });
        }

        var al=LastfmService.topAlbum(mbid);
        if(al.length>0) {
            remplirTopAlbum(JSON.parse(al))
        }
    }

    function  remplirTopAlbum(d) {
        var t = d.topalbums.album;

        for(var i in t) {
            topalbum_model.append({
                                      "name":t[i].name,
                                      "url":t[i].url,
                                      "img":trouverImg(t[i].image)
                                  });
        }

        topalbum.visible=true;
    }

    function trouverImg(ar) {
        for(var i in ar) {
            if(ar[i].size==="extralarge") {
                return ar[i]["#text"];
            }
        }
        return "";
    }

    function tagstr(t) {
        var sep="";
        var res="";

        for(var i in t) {
            res = res+sep+t[i].name;
            sep=", "
        }

        return res;
    }

    SilicaFlickable {
        anchors.fill: parent
        clip: true
        contentHeight:bio.height
        contentWidth: parent.width

        PullDownMenu {
            MenuItem {
                text:  vi18n.get("menu.config")
                onClicked: {pageStack.push(Qt.resolvedUrl("Configuration.qml"));}
            }
            MenuItem {
                text:  vi18n.get("bio.effacer")
                enabled: contenu.text.length>0
                onClicked: {
                    LastfmService.effacerBio(artiste);
                    LastfmService.effacerBio(mbid);
                    pageStack.navigateBack();
                }
            }
        }

        VerticalScrollDecorator {flickable:parent}

        // bio
        Column {
            id:bio
            visible: contenu.text.length>0

            anchors {
                left: parent.left;
                right: parent.right;
            }

            spacing: Theme.paddingLarge

            PageHeader {
                id:titre
            }

            Item {
                id:info
                width: parent.width
                height: Math.max(tags.height,trombi.height)
                anchors.bottomMargin: Theme.paddingLarge

                Label {
                    id:tags
                    anchors {
                        left: parent.left
                        right: trombi.left
                        leftMargin: Theme.horizontalPageMargin
                        rightMargin: Theme.paddingLarge
                    }

                    x:Theme.horizontalPageMargin
                    wrapMode: Text.WordWrap
                    elide: Text.ElideRight
                    color: Theme.secondaryHighlightColor
                    font.italic: true
                }

                Image {
                    id: trombi
                    width: parent.width*0.45
                    height: parent.width*0.45
                    anchors {
                        right: parent.right
                        rightMargin: Theme.horizontalPageMargin
                    }
                }
            }

            Text {
                id: contenu
                text: ""
                width: parent.width-(Theme.horizontalPageMargin *2)
                x:Theme.horizontalPageMargin
                color: Theme.highlightColor
                wrapMode: Text.WordWrap
                elide: Text.ElideRight
                font.pixelSize: Theme.fontSizeMedium
                textFormat: Text.StyledText
                linkColor: Theme.primaryColor
            }

            SectionHeader { text: vi18n.get("bio.album") }

            SlideshowView {
                id: topalbum
                visible: false
                width: parent.width
                height: width / 3
                itemWidth: width / 3

                model :ListModel { id: topalbum_model }

                delegate: BackgroundItem {
                    width: parent.itemWidth
                    height: parent.itemWidth

                    Image {
                        anchors.fill: parent
                        source: img
                    }

                    Text {
                        anchors {
                            right:parent.right
                            bottom: parent.bottom;
                        }

                        text: name
                        font.pixelSize: Theme.fontSizeTiny
                        color: "white"
                    }

                    onClicked:Qt.openUrlExternally(url)
                }
            }

            SectionHeader { text: vi18n.get("bio.similaire") }

            SlideshowView {
                id: silimaire
                width: parent.width
                height: width / 3
                itemWidth: width / 3

                model :ListModel { id: similaire_model }

                delegate: BackgroundItem {
                    width: parent.itemWidth
                    height: parent.itemWidth

                    Image {
                        anchors.fill: parent
                        source: img
                    }

                    Text {
                        anchors {
                            right:parent.right
                            bottom: parent.bottom;
                        }

                        text: name
                        font.pixelSize: Theme.fontSizeTiny
                        color: "white"
                    }

                    onClicked:Qt.openUrlExternally(url)
                }
            }

           // lien last.fm
            BackgroundItem {
                id:artistelast
                property string adresse

                width: parent.width
                height: Theme.itemSizeExtraLarge

                onClicked: Qt.openUrlExternally(adresse)

                Label {
                    width:parent.width
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    text: vi18n.get("bio.lastfm.url")
                    font.underline: true
                    font.bold: true
                }
            }
        }

        Lastfm {
            width: parent.width
            visible: lastfmok===false
            onButtonClicked: {
                ConfigService.setLastfm(true);
                if(areseau) {
                    recherche();
                }
            }
        }

        // info reseau
        ViewPlaceholder {
            id:vre
            enabled: false;
            text:  vi18n.get("info.reseau.titre")
            hintText:  vi18n.get("info.reseau.desc")
        }

        // pas de contenu
        ViewPlaceholder {
            id:vph
            enabled: false
            // enabled: contenu.text.length===0 && done===true
            text:  vi18n.get("bio.aucune.titre")
            hintText: vi18n.get("bio.aucune.sum")
        }
    }

    Attente {
        enabled: (LastfmService.enCours || status===PageStatus.Activating) && (done===false)
    }

    onStatusChanged: {
        if (status===PageStatus.Active && done===false && lastfmok===true) {
            recherche();
        }
    }

    Connections {
        target:LastfmService
        onTrouveBio:{
            if (status!==PageStatus.Active || done === true) {
                return;
            }

            if( donnee !== undefined && donnee.length >0 ) {
                remplirBio(JSON.parse(donnee));
            }
            done=true;

            if(contenu.text.length===0) {
                // pas de texte
                vph.enabled=true;
            }
        }

        onTrouveTopAlbum : {
            if (status!==PageStatus.Active) {
                return;
            }

            if( donnee !== undefined && donnee.length >0 ) {
                remplirTopAlbum(JSON.parse(donnee));
            }
        }
    }

    Connections {
        target:networkwatcher
        onConnectedChanged : {
            if(areseau===false || done===true) {
                return;
            }

            vre.enabled=false;
            recherche();
        }
    }
}
