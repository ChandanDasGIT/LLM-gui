#pragma once

#include <QObject>
#include <QString>
#include <QtQml/qqmlregistration.h>

// The status strip's texts for QML: model, character, connection and the last reply's speed,
// from the session (backend statusstrip makes the texts).
class StatusStripWrap : public QObject {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QString modelText READ modelText NOTIFY changed)
    Q_PROPERTY(QString characterText READ characterText NOTIFY changed)
    Q_PROPERTY(QString connectionText READ connectionText NOTIFY changed)
    Q_PROPERTY(bool connected READ connected NOTIFY changed)
    Q_PROPERTY(bool offline READ offline NOTIFY changed)
    Q_PROPERTY(QString speedText READ speedText NOTIFY changed)

public:
    explicit StatusStripWrap(QObject* parent = nullptr);

    QString modelText() const;
    QString characterText() const;
    QString connectionText() const;
    bool connected() const;
    bool offline() const;
    QString speedText() const;

signals:
    void changed();
};
