#pragma once

#include "common/character.h"

#include <QAbstractListModel>
#include <QString>
#include <QVariantMap>
#include <QtQml/qqmlregistration.h>

#include <vector>

// The character manager for QML: every character as a list model row (Default first) and the
// actions on them. Files live in ~/.local/share/LLM-gui/characters (backend characterstore).
class CharactersWrap : public QAbstractListModel {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(int activeIndex READ activeIndex NOTIFY activeChanged)
    Q_PROPERTY(QString message READ message NOTIFY messageChanged)

public:
    enum Roles {
        IdRole = Qt::UserRole + 1,
        NameRole,
        SystemPromptRole,
        GreetingRole,
        PreferredModelRole,
        UseTemperatureRole,
        TemperatureRole,
        BuiltInRole,
        ActiveRole,
    };

    explicit CharactersWrap(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    int activeIndex() const;
    // The last problem (or empty).
    QString message() const { return message_; }

    // One row as {characterId, name, systemPrompt, greeting, preferredModel, useTemperature,
    // temperature, builtIn, active}; empty when out of range.
    Q_INVOKABLE QVariantMap get(int row) const;

    // Each returns the row of the character afterwards, or -1 with message set.
    Q_INVOKABLE int save(int row, const QString& name, const QString& systemPrompt, const QString& greeting,
                         const QString& preferredModel, bool useTemperature, double temperature);
    Q_INVOKABLE int create();
    Q_INVOKABLE int duplicate(int row);
    Q_INVOKABLE bool remove(int row);
    // Makes it the character of the open chat.
    Q_INVOKABLE void use(int row);

signals:
    void activeChanged();
    void messageChanged();

private:
    void reload();
    int rowOf(const std::string& id) const;
    void setMessage(const QString& text);
    void pushToSession(const Character& c, bool withPreferredModel);

    std::vector<Character> characters_;
    QString message_;
};
