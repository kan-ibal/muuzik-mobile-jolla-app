#include "qnetworkwatcher.h"
#include <QtQml>
#include <QMetaType>
#include <QDebug>
#include <QFile>
#include <QTextStream>
#include <QRegExp>
#include <QNetworkAddressEntry>
#include <QDBusConnection>
#include <QDBusInterface>

qnetworkwatcher::qnetworkwatcher(QObject *parent)
    : QObject(parent), network(NetworkType::NONE), isVpnActive(false) {
    connect(&net, SIGNAL(configurationChanged(QNetworkConfiguration)),
            this, SLOT(configurationChanged(QNetworkConfiguration)));

    connect(&net, SIGNAL(onlineStateChanged(bool)),
            this, SLOT(onlineStateChanged(bool)));

    // Connect to ConnMan Manager ServicesChanged on system D-Bus if available
    if (QDBusConnection::systemBus().isConnected()) {
        QDBusConnection::systemBus().connect(
            "net.connman",
            "/",
            "net.connman.Manager",
            "ServicesChanged",
            this,
            SLOT(servicesChanged())
        );

        // Also monitor ConnMan-VPN daemon on Sailfish OS
        QDBusConnection::systemBus().connect(
            "net.connman.vpn",
            "/",
            "net.connman.vpn.Manager",
            "ConnectionAdded",
            this,
            SLOT(servicesChanged())
        );
        QDBusConnection::systemBus().connect(
            "net.connman.vpn",
            "/",
            "net.connman.vpn.Manager",
            "ConnectionRemoved",
            this,
            SLOT(servicesChanged())
        );

        // Monitor Technology changes directly
        QDBusConnection::systemBus().connect(
            "net.connman",
            "/net/connman/technology/wifi",
            "net.connman.Technology",
            "PropertyChanged",
            this,
            SLOT(servicesChanged())
        );
        QDBusConnection::systemBus().connect(
            "net.connman",
            "/net/connman/technology/cellular",
            "net.connman.Technology",
            "PropertyChanged",
            this,
            SLOT(servicesChanged())
        );
        QDBusConnection::systemBus().connect(
            "net.connman",
            "/net/connman/technology/vpn",
            "net.connman.Technology",
            "PropertyChanged",
            this,
            SLOT(servicesChanged())
        );
    }

    checkNetwork();
}

/**
 * @brief getInstance
 * @return
 */
qnetworkwatcher& qnetworkwatcher::getInstance() {
    static qnetworkwatcher instance;
    return instance;
}

/**
 * @brief IMPORTANT ! you must register this object into qml, this method do it for you.
 * Service will be available on the name "qnetworkwatcher" (else define in QNETWORKWATCHER_QML_NAME).
 * You can also register service by yourself (or not, if you don't need it in qml)
 * @param context
 * @param name : register service on this name, default to QNETWORKWATCHER_QML_NAME
 */
void qnetworkwatcher::init(QQmlContext *context, QString name) {
#ifdef QT_DEBUG
    qDebug() << "qnetworkwatcher, registering qml service as : " << name;
#endif
    context->setContextProperty(name, &qnetworkwatcher::getInstance());

    qmlRegisterUncreatableType<NetworkType>("org.vostok.qnetworkwatcher", 1, 0, "NetworkType","");
}

/**
 * @brief qnetworkwatcher::configurationChanged
 * @param c
 */
void qnetworkwatcher::configurationChanged(const QNetworkConfiguration&) {
    checkNetwork();
}

/**
 * @brief qnetworkwatcher::onlineStateChanged
 */
void qnetworkwatcher::onlineStateChanged(bool) {
    checkNetwork();
}

/**
 * @brief qnetworkwatcher::servicesChanged
 */
void qnetworkwatcher::servicesChanged() {
    checkNetwork();
}

/**
 * @brief Queries ConnMan technologies via system D-Bus for active services
 */
void qnetworkwatcher::queryConnMan(bool &hasWifi, bool &hasCellular, bool &hasEth, bool &hasVpn) {
    if (!QDBusConnection::systemBus().isConnected()) {
        return;
    }

    // Query ConnMan Technology properties directly via D-Bus properties
    QDBusInterface wifiTech("net.connman", "/net/connman/technology/wifi", "net.connman.Technology", QDBusConnection::systemBus());
    if (wifiTech.isValid()) {
        wifiTech.setTimeout(250);
        QVariant val = wifiTech.property("Connected");
        if (val.isValid() && val.toBool()) {
            hasWifi = true;
        }
    }

    QDBusInterface cellularTech("net.connman", "/net/connman/technology/cellular", "net.connman.Technology", QDBusConnection::systemBus());
    if (cellularTech.isValid()) {
        cellularTech.setTimeout(250);
        QVariant val = cellularTech.property("Connected");
        if (val.isValid() && val.toBool()) {
            hasCellular = true;
        }
    }

    QDBusInterface ethTech("net.connman", "/net/connman/technology/ethernet", "net.connman.Technology", QDBusConnection::systemBus());
    if (ethTech.isValid()) {
        ethTech.setTimeout(250);
        QVariant val = ethTech.property("Connected");
        if (val.isValid() && val.toBool()) {
            hasEth = true;
        }
    }

    QDBusInterface vpnTech("net.connman", "/net/connman/technology/vpn", "net.connman.Technology", QDBusConnection::systemBus());
    if (vpnTech.isValid()) {
        vpnTech.setTimeout(250);
        QVariant val = vpnTech.property("Connected");
        if (val.isValid() && val.toBool()) {
            hasVpn = true;
        }
    }
}

/**
 * @brief Inspects QNetworkConfigurationManager active configurations
 */
void qnetworkwatcher::queryConfigurations(bool &hasWifi, bool &hasCellular, bool &hasEth, bool &hasVpn) {
    const QList<QNetworkConfiguration> configs = net.allConfigurations(QNetworkConfiguration::Active);
    for (const QNetworkConfiguration &config : configs) {
        if (!config.isValid()) {
            continue;
        }

        QString typeName = config.bearerTypeName().toLower();
        QString name = config.name().toLower();

        if (config.purpose() == QNetworkConfiguration::PrivatePurpose ||
            typeName.contains("vpn") || name.contains("vpn")) {
            hasVpn = true;
        }

        switch (config.bearerType()) {
        case QNetworkConfiguration::BearerWLAN:
            hasWifi = true;
            break;
        case QNetworkConfiguration::BearerEthernet:
            hasEth = true;
            break;
        case QNetworkConfiguration::BearerUnknown:
            if (typeName.contains("wlan") || typeName.contains("wifi")) {
                hasWifi = true;
            } else if (typeName.contains("eth")) {
                hasEth = true;
            } else if (typeName.contains("cellular") || typeName.contains("lte") ||
                       typeName.contains("wcdma") || typeName.contains("gsm")) {
                hasCellular = true;
            }
            break;
        default:
            // Bearer2G, Bearer3G, Bearer4G, BearerWCDMA, BearerHSPA, BearerLTE, etc.
            hasCellular = true;
            break;
        }
    }

    // Also check defaultConfiguration
    QNetworkConfiguration def = net.defaultConfiguration();
    if (def.isValid()) {
        QString defTypeName = def.bearerTypeName().toLower();
        QString defName = def.name().toLower();
        if (def.purpose() == QNetworkConfiguration::PrivatePurpose ||
            defTypeName.contains("vpn") || defName.contains("vpn")) {
            hasVpn = true;
        }

        // Check children if default config is a ServiceNetwork
        const QList<QNetworkConfiguration> children = def.children();
        for (const QNetworkConfiguration &child : children) {
            if (!child.isValid()) {
                continue;
            }
            if (child.bearerType() == QNetworkConfiguration::BearerWLAN) {
                hasWifi = true;
            } else if (child.bearerType() == QNetworkConfiguration::BearerEthernet) {
                hasEth = true;
            } else if (child.bearerType() != QNetworkConfiguration::BearerUnknown) {
                hasCellular = true;
            }
        }
    }
}

/**
 * @brief Inspects Linux system network interfaces on Sailfish OS
 */
void qnetworkwatcher::queryInterfaces(bool &hasWifi, bool &hasCellular, bool &hasEth, bool &hasVpn) {
    const QList<QNetworkInterface> interfaces = QNetworkInterface::allInterfaces();
    for (const QNetworkInterface &iface : interfaces) {
        QNetworkInterface::InterfaceFlags flags = iface.flags();
        if (!(flags & QNetworkInterface::IsUp) || !(flags & QNetworkInterface::IsRunning)) {
            continue;
        }
        if (flags & QNetworkInterface::IsLoopBack) {
            continue;
        }

        // Must have at least one valid address
        bool hasAddress = false;
        const QList<QNetworkAddressEntry> entries = iface.addressEntries();
        for (const QNetworkAddressEntry &entry : entries) {
            if (!entry.ip().isLoopback() && !entry.ip().isNull()) {
                hasAddress = true;
                break;
            }
        }
        if (!hasAddress) {
            continue;
        }

        QString ifName = iface.name().toLower();
        if (ifName.startsWith("tun") || ifName.startsWith("tap") || ifName.startsWith("vpn") ||
            ifName.startsWith("wg") || ifName.startsWith("ppp") || ifName.startsWith("ipsec")) {
            hasVpn = true;
        } else if (ifName.startsWith("wlan") || ifName.startsWith("wl")) {
            hasWifi = true;
        } else if (ifName.startsWith("eth") || ifName.startsWith("en")) {
            hasEth = true;
        } else if (ifName.startsWith("rmnet") || ifName.startsWith("ccmni") || ifName.startsWith("wwan") ||
                   ifName.startsWith("cellular") || ifName.startsWith("qmimux") || ifName.startsWith("cdc")) {
            hasCellular = true;
        }
    }
}

/**
 * @brief Inspects /proc/net/route for active Linux routes and default gateway
 */
void qnetworkwatcher::queryRoutes(bool &hasWifi, bool &hasCellular, bool &hasEth, bool &hasVpn) {
    QFile routeFile("/proc/net/route");
    if (!routeFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return;
    }

    QTextStream in(&routeFile);
    in.readLine(); // skip table header
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (line.isEmpty()) {
            continue;
        }
        QStringList tokens = line.split(QRegExp("\\s+"), QString::SkipEmptyParts);
        if (tokens.size() >= 4) {
            QString iface = tokens.at(0).toLower();
            QString dest = tokens.at(1);
            int flags = tokens.at(3).toInt(NULL, 16);

            // RTF_UP = 0x0001
            if ((flags & 0x0001) != 0) {
                if (iface.startsWith("tun") || iface.startsWith("tap") ||
                    iface.startsWith("vpn") || iface.startsWith("wg")) {
                    hasVpn = true;
                }

                // If default route (dest == 00000000)
                if (dest == "00000000") {
                    if (iface.startsWith("tun") || iface.startsWith("tap") ||
                        iface.startsWith("vpn") || iface.startsWith("wg")) {
                        hasVpn = true;
                    } else if (iface.startsWith("wlan") || iface.startsWith("wl")) {
                        hasWifi = true;
                    } else if (iface.startsWith("eth") || iface.startsWith("en")) {
                        hasEth = true;
                    } else if (iface.startsWith("rmnet") || iface.startsWith("ccmni") ||
                               iface.startsWith("wwan") || iface.startsWith("cellular") ||
                               iface.startsWith("qmimux") || iface.startsWith("cdc")) {
                        hasCellular = true;
                    }
                }
            }
        }
    }
    routeFile.close();
}

/**
 * @brief Checks and updates network status, mapping VPN via Wi-Fi to WIFI
 * and VPN via WCDMA/LTE to WCDMA.
 */
void qnetworkwatcher::checkNetwork() {
    NetworkType::Type n = network; // save old type
    bool oldStatus = (network != NetworkType::NONE);
    bool oldVpn = isVpnActive;

    bool hasWifi = false;
    bool hasCellular = false;
    bool hasEth = false;
    bool hasVpn = false;

    // 1. Query ConnMan via system D-Bus
    queryConnMan(hasWifi, hasCellular, hasEth, hasVpn);

    // 2. Query QNetworkConfigurationManager
    queryConfigurations(hasWifi, hasCellular, hasEth, hasVpn);

    // 3. Query Linux network interfaces
    queryInterfaces(hasWifi, hasCellular, hasEth, hasVpn);

    // 4. Query Linux kernel route table
    queryRoutes(hasWifi, hasCellular, hasEth, hasVpn);

    isVpnActive = hasVpn;

    // Map network type according to user requirements:
    // "VPN via wifi should be treated as wifi and VPN via WCDMA LTE should be treated as mobile connection."
    if (hasWifi) {
        network = NetworkType::WIFI;
    } else if (hasEth) {
        network = NetworkType::ETH;
    } else if (hasCellular) {
        network = NetworkType::WCDMA;
    } else if (net.isOnline()) {
        switch (net.defaultConfiguration().bearerType()) {
        case QNetworkConfiguration::BearerWLAN:
            network = NetworkType::WIFI;
            break;
        case QNetworkConfiguration::BearerEthernet:
            network = NetworkType::ETH;
            break;
        case QNetworkConfiguration::BearerUnknown:
            if (hasVpn) {
                // VPN active but underlying physical bearer not directly named;
                // fallback to mobile/cellular connection
                network = NetworkType::WCDMA;
            } else {
                network = NetworkType::NONE;
            }
            break;
        default:
            network = NetworkType::WCDMA;
            break;
        }
    } else {
        network = NetworkType::NONE;
    }

    if (oldVpn != isVpnActive) {
        emit vpnOn();
        emit vpnChanged();
    }

    if (network == n) {
        // same type of connection, nothing to do
        return;
    }

#ifdef QT_DEBUG
    qDebug() << "qnetworkwatcher, changed : " << network << " (vpn:" << isVpnActive << ")";
#endif

    emit networkChanged();

    bool currentStatus = (network != NetworkType::NONE);
    if (oldStatus != currentStatus) {
        emit connectedChanged();
    }

    if (network == NetworkType::WIFI || n == NetworkType::WIFI) {
        emit wifiOn();
    }

    if (network == NetworkType::WCDMA || n == NetworkType::WCDMA) {
        emit wcdmaOn();
    }

    if (network == NetworkType::ETH || n == NetworkType::ETH) {
        emit ethOn();
    }
}
