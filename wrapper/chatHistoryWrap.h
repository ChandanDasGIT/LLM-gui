#pragma once

#include "common/chatstore.h"

#include <QAbstractListModel>
#include <QtQml/qqmlregistration.h>

#include <vector>

// The history sidebar for QML: the saved chats, newest first, with their day group. Selecting
// one opens it in the chat; "new" opens an empty chat.
class ChatHistoryWrap : public QAbstractListModel {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(int currentIndex READ currentIndex NOTIFY currentChanged)

public:
    enum Roles {
        IdRole = Qt::UserRole + 1,
        TitleRole,
        MetaRole,                      // "model · character"
        DayRole,                       // "Today", "Yesterday" or "Earlier"
    };

    explicit ChatHistoryWrap(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    // -1 while a new chat is open.
    int currentIndex() const;

    Q_INVOKABLE void select(int row);
    Q_INVOKABLE void newChat();
    Q_INVOKABLE void remove(int row);

signals:
    void currentChanged();

private:
    void reload();

    std::vector<ChatSummary> chats_;
    long long loadedAtMs_ = 0;
};
