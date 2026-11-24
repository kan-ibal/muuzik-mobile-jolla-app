/*
  Copyright (C) 2016 Jerome levreau / vostoksystem.
  Contact: contact <contact@vostoksytem.eu>
  All rights reserved.

  Apache 2.0 Licence

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE FOR
  ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
  ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  */

#ifndef SDCARDSERVICE_H
#define SDCARDSERVICE_H

#include <QObject>
#include <QString>
#include <QQmlContext>
#include <QDebug>

#define SDCARDLOCATOR_QML_NAME "SdcardLocator"

/**
 * @brief this helper class is to be used with phone (sailfish os / jolla) for sdcard ops/info
 * @version 1.4
 * @author levreau.jerome@vostoksystem.eu
 */
class SdcardLocator : public QObject {

    Q_OBJECT
    Q_PROPERTY( QString location READ getLocation NOTIFY sdcardChanged )
    Q_PROPERTY( bool present READ isPresent NOTIFY sdcardChanged )
    Q_PROPERTY( QString type READ getType NOTIFY sdcardChanged )

public:
    /**
     * @brief instance (singleton) for the service
     * @return
     */
    static SdcardLocator& getInstance();

    /**
     * @brief register the service into the context so it can be used in qml
     * @param context
     * @param name : register service on this name,  default to SDCARDLOCATOR_QML_NAME
     */
    static void init(QQmlContext *context, QString name=SDCARDLOCATOR_QML_NAME);

    /**
      * @brief return the full path to the root of the sdcard
      * @return location or "" if not present; never null
      */
    Q_INVOKABLE QString getLocation() const {return location;}

    /**
      * @brief check if sdcard present
      * @return true if present
      */
    Q_INVOKABLE bool isPresent() const {return location.length()>0;}

    /**
      * @brief return the type of filesystem
      * @return type or empty string; never null
      */
    Q_INVOKABLE QString getType() const {return type;}

    /**
      * @brief check if url is  on the sdcard.
      * @param url
      * @return false if not or no sdcard
      * @since 1.4
      */
    Q_INVOKABLE bool isOnSdcard(const QString &url);

    /**
      * @brief rescan removable media mount points
      */
    Q_INVOKABLE void refresh();

signals:
    void sdcardChanged();

private:
    explicit SdcardLocator(QObject *parent = 0);

    QString location;
    QString type;

    /**
     * @brief check for a sdcard
     */
    void checkSdcard();
};

#endif // SDCARDSERVICE_H
