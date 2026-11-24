#include "webradioutil.h"
#include <QUrlQuery>
#include <QList>
#include <QPair>
#include <QRegularExpression>

namespace {
QString decodeValue(const QString &raw) {
    return QUrl::fromPercentEncoding(raw.toUtf8()).trimmed();
}

QString extractFragmentStreamName(const QString &fragment) {
    if (fragment.isEmpty()) {
        return QString();
    }

    static const QRegularExpression nextParamRe(QStringLiteral("&[A-Za-z0-9_]+="));
    const int idx = fragment.indexOf(QStringLiteral("StreamName="), 0, Qt::CaseInsensitive);
    if (idx >= 0) {
        if (idx == 0 || fragment.at(idx - 1) == '&') {
            const int valStart = idx + 11; // strlen("StreamName=")
            const QRegularExpressionMatch m = nextParamRe.match(fragment, valStart);
            const QString rawVal = m.hasMatch()
                    ? fragment.mid(valStart, m.capturedStart() - valStart)
                    : fragment.mid(valStart);
            return decodeValue(rawVal);
        }
    }
    return QString();
}
}

bool WebRadioUtil::isStreamUrl(const QString &value) {
    const QString trimmed = value.trimmed();
    return trimmed.startsWith(QStringLiteral("http://"), Qt::CaseInsensitive) ||
           trimmed.startsWith(QStringLiteral("https://"), Qt::CaseInsensitive);
}

WebRadioInfo WebRadioUtil::parse(const QString &rawUrl) {
    WebRadioInfo info;
    const QString trimmed = rawUrl.trimmed();
    info.rawUrl = trimmed;
    info.isStream = isStreamUrl(trimmed);
    if (!info.isStream) {
        return info;
    }

    QString urlPart = trimmed;
    QString rawFragment;
    const int hashIdx = trimmed.indexOf('#');
    if (hashIdx >= 0) {
        urlPart = trimmed.left(hashIdx);
        rawFragment = trimmed.mid(hashIdx + 1);
    }

    const QString streamName = extractFragmentStreamName(rawFragment);
    if (!streamName.isEmpty()) {
        info.name = streamName;
    }

    QUrl parsedUrl(urlPart, QUrl::TolerantMode);
    if (parsedUrl.hasQuery()) {
        const QUrlQuery srcQuery(parsedUrl);
        QUrlQuery cleanQuery;
        const QList<QPair<QString, QString> > items = srcQuery.queryItems(QUrl::FullyDecoded);
        for (int i = 0; i < items.size(); ++i) {
            const QString &key = items.at(i).first;
            const QString &val = items.at(i).second;
            const QString lowerKey = key.toLower();

            if (lowerKey == QLatin1String("jupii_name")) {
                if (info.name.isEmpty()) {
                    info.name = val.trimmed();
                }
            } else if (lowerKey == QLatin1String("jupii_icon")) {
                info.iconUrl = val.trimmed();
            } else if (lowerKey == QLatin1String("jupii_author")) {
                info.author = val.trimmed();
            } else if (lowerKey == QLatin1String("jupii_app")) {
                info.app = val.trimmed();
            } else if (lowerKey == QLatin1String("jupii_origurl")) {
                info.origUrl = val.trimmed();
            } else if (!lowerKey.startsWith(QLatin1String("jupii_"))) {
                cleanQuery.addQueryItem(key, val);
            }
        }

        if (cleanQuery.isEmpty()) {
            parsedUrl.setQuery(QString());
        } else {
            parsedUrl.setQuery(cleanQuery);
        }
    }

    parsedUrl.setFragment(QString());
    if (!parsedUrl.isValid() && !info.origUrl.isEmpty()) {
        parsedUrl = QUrl(info.origUrl, QUrl::TolerantMode);
    }
    info.playbackUrl = parsedUrl;

    QString fallback = parsedUrl.fileName().trimmed();
    if (fallback.isEmpty()) {
        fallback = parsedUrl.host().trimmed();
    }
    if (fallback.isEmpty()) {
        fallback = trimmed;
    }
    info.fallbackTitle = fallback;

    return info;
}

QUrl WebRadioUtil::playbackUrl(const QString &rawUrl) {
    return parse(rawUrl).playbackUrl;
}
