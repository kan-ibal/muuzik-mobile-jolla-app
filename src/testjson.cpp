#include "testjson.h"
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>

#include "virtualfiles/VFservice.h"

TestJson::TestJson(QObject *parent) : QObject(parent)
{

}

QString TestJson::readFile(const QString &filename) {
    QFile f(filename);
    if (!f.open(QFile::ReadOnly | QFile::Text)) {
        return QString();
    } else {
        QTextStream in(&f);
        return in.readAll();
    }
}

void TestJson::doTest(const QString vurl ) {
    QString data = TestJson::readFile(VFI.getPathFromVUrl(vurl));

    if(data.isEmpty()) {
        qDebug() << "no data";
         return;
    }

    QJsonDocument json_doc = QJsonDocument::fromJson(data.toUtf8());
    if (json_doc.isNull()) {
       qDebug() << "Failed to create JSON doc.";
        return ;
    }
    if (!json_doc.isObject()) {
        qDebug() << "JSON is not an object.";
        return ;
    }

    QJsonObject json_obj = json_doc.object();
    if (json_obj.isEmpty()) {
        qDebug() << "JSON object is empty.";
        return;
    }

    QVariantMap  result = json_obj.toVariantMap();

    QList<QVariant> images = result["album"].toMap()["image"].toList();
    foreach (QVariant v, images) {
        qDebug() << v;

        QVariantMap m = v.value<QVariantMap>();
        qDebug() << m["size"] << " - " << m["#text"];


        if(m["size"].value<QString>().toLower().compare("mega")) {
            qDebug() << "---> "  << m["#text"].value<QString>();
        }

    }
    //QVariant(QVariantMap, QMap(("#text", QVariant(QString, "https://lastfm-img2.akamaized.net/i/u/34s/0be56102ab67e64ce824fd6b4017056b.png"))("size", QVariant(QString, "small"))))
}
