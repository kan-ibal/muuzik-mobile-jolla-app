#include "mpris.h"
#include <QGuiApplication>
#include <QWindow>

Mpris::Mpris( QObject *parent): QDBusAbstractAdaptor(parent) {

    uri.append("file");
    mime.append("audio/mp3");
    mime.append("audio/mpeg");
    mime.append("audio/ogg");
    mime.append("audio/x-vorbis+ogg");
    mime.append("audio/flac");
    mime.append("audio/x-flac");
    mime.append("audio/wav");
    mime.append("audio/mp4");
    mime.append("audio/aac");
    mime.append("audio/opus");
}

/**
 * @brief Mpris::Raise
 */
void Mpris::Raise(){
    foreach (QWindow *window, QGuiApplication::topLevelWindows()) {
        if (window) {
            window->show();
            window->raise();
            window->requestActivate();
        }
    }
}

/**
 * @brief Mpris::Quit
 */
void Mpris::Quit(){
}
