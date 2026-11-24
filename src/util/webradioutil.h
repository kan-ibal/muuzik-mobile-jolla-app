#ifndef WEBRADIOUTIL_H
#define WEBRADIOUTIL_H

#include <QString>
#include <QUrl>

struct WebRadioInfo {
    bool isStream;
    QString rawUrl;
    QUrl playbackUrl;
    QString name;
    QString author;
    QString app;
    QString iconUrl;
    QString origUrl;
    QString fallbackTitle;

    WebRadioInfo() : isStream(false) {}
};

class WebRadioUtil {
public:
    static bool isStreamUrl(const QString &value);
    static WebRadioInfo parse(const QString &rawUrl);
    static QUrl playbackUrl(const QString &rawUrl);
};

#endif // WEBRADIOUTIL_H
