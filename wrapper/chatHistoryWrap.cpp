#include "wrapper/chatHistoryWrap.h"

#include "wrapper/common/appSession/appSessionWrap.h"

#include <QDateTime>

namespace {

ChatStore chatStore()
{
    return ChatStore((AppSessionWrap::dataFolder() + QStringLiteral("/chats")).toStdString());
}

} // namespace

ChatHistoryWrap::ChatHistoryWrap(QObject* parent) : QAbstractListModel(parent)
{
    auto* session = AppSessionWrap::instance();
    connect(session, &AppSessionWrap::chatsChanged, this, &ChatHistoryWrap::reload);
    connect(session, &AppSessionWrap::currentChatChanged, this, &ChatHistoryWrap::currentChanged);
    reload();
}

void ChatHistoryWrap::reload()
{
    beginResetModel();
    chats_ = chatStore().list();
    loadedAtMs_ = QDateTime::currentMSecsSinceEpoch();
    endResetModel();
    emit currentChanged();
}

int ChatHistoryWrap::rowCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : int(chats_.size());
}

QVariant ChatHistoryWrap::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() >= rowCount()) return {};
    const ChatSummary& c = chats_[std::size_t(index.row())];
    switch (role) {
    case IdRole: return QString::fromStdString(c.id);
    case TitleRole: return QString::fromStdString(c.title);
    case MetaRole: {
        QString meta = QString::fromStdString(c.model);
        if (!c.characterName.empty()) meta += (meta.isEmpty() ? QString() : QStringLiteral(" · ")) + QString::fromStdString(c.characterName);
        return meta;
    }
    case DayRole: return QString::fromStdString(chatstore::dayGroup(c.updatedMs, loadedAtMs_));
    default: return {};
    }
}

QHash<int, QByteArray> ChatHistoryWrap::roleNames() const
{
    return {{IdRole, "chatId"}, {TitleRole, "title"}, {MetaRole, "meta"}, {DayRole, "day"}};
}

int ChatHistoryWrap::currentIndex() const
{
    const std::string current = AppSessionWrap::instance()->currentChatId().toStdString();
    for (std::size_t i = 0; i < chats_.size(); ++i)
        if (chats_[i].id == current) return int(i);
    return -1;
}

void ChatHistoryWrap::select(int row)
{
    if (row < 0 || row >= rowCount()) return;
    AppSessionWrap::instance()->setCurrentChatId(QString::fromStdString(chats_[std::size_t(row)].id));
}

void ChatHistoryWrap::newChat()
{
    AppSessionWrap::instance()->setCurrentChatId(QString());
}

void ChatHistoryWrap::remove(int row)
{
    if (row < 0 || row >= rowCount()) return;
    const QString id = QString::fromStdString(chats_[std::size_t(row)].id);
    chatStore().remove(id.toStdString());
    if (AppSessionWrap::instance()->currentChatId() == id) AppSessionWrap::instance()->setCurrentChatId(QString());
    AppSessionWrap::instance()->notifyChatsChanged();
}
