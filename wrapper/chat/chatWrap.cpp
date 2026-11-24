#include "wrapper/chat/chatWrap.h"

#include "wrapper/common/appSession/appSessionWrap.h"
#include "wrapper/common/ollamaClient/ollamaClientWrap.h"

#include "chat/base64.h"
#include "common/characterstore.h"
#include "common/chatstore.h"
#include "common/ollamaapi.h"

#include <QDateTime>
#include <QFile>
#include <QFileInfo>
#include <QNetworkReply>
#include <QVariantMap>

namespace {

constexpr qint64 maxImageBytes = 20 * 1024 * 1024;

bool isImageFile(const QString& path)
{
    const QString suffix = QFileInfo(path).suffix().toLower();
    return suffix == "png" || suffix == "jpg" || suffix == "jpeg" || suffix == "webp";
}

QString stateName(ChatEntry::State state)
{
    switch (state) {
    case ChatEntry::State::Streaming: return QStringLiteral("streaming");
    case ChatEntry::State::Finished: return QStringLiteral("finished");
    case ChatEntry::State::Stopped: return QStringLiteral("stopped");
    case ChatEntry::State::Failed: return QStringLiteral("failed");
    }
    return {};
}

QString kindName(ChatEntry::Role role)
{
    switch (role) {
    case ChatEntry::Role::User: return QStringLiteral("user");
    case ChatEntry::Role::Assistant: return QStringLiteral("assistant");
    case ChatEntry::Role::Notice: return QStringLiteral("notice");
    }
    return {};
}

std::string readBase64(const std::string& path)
{
    QFile file(QString::fromStdString(path));
    if (!file.open(QIODevice::ReadOnly)) return {};
    const QByteArray bytes = file.readAll();
    return base64Encode(std::string_view(bytes.constData(), std::size_t(bytes.size())));
}

ChatStore chatStore()
{
    return ChatStore((AppSessionWrap::dataFolder() + QStringLiteral("/chats")).toStdString());
}

QString gigabytes(long long bytes)
{
    return QString::number(double(bytes) / 1e9, 'f', 1) + QStringLiteral(" GB");
}

} // namespace

ChatWrap::ChatWrap(QObject* parent) : QAbstractListModel(parent), client_(new OllamaClientWrap(this))
{
    auto* session = AppSessionWrap::instance();
    connect(session, &AppSessionWrap::characterChanged, this, &ChatWrap::onCharacterChanged);
    connect(session, &AppSessionWrap::currentModelChanged, this, [this] {
        loadCapabilities();
        emit modelsChanged();
        save();
    });
    connect(session, &AppSessionWrap::currentChatChanged, this, [this] {
        if (AppSessionWrap::instance()->currentChatId() != chatId_) openCurrentChat();
    });
    connect(session, &AppSessionWrap::connectionChanged, this, [this] {
        if (AppSessionWrap::instance()->connection() != AppSessionWrap::Connected) return;
        loadModels();
        if (capabilitiesModel_ != AppSessionWrap::instance()->currentModel()) loadCapabilities();
    });
    connect(session, &AppSessionWrap::modelsChanged, this, &ChatWrap::loadModels);
    connect(session, &AppSessionWrap::charactersChanged, this, &ChatWrap::loadCharacters);
    loadCharacters();
    openCurrentChat();
    loadModels();
    loadCapabilities();
}

ChatWrap::~ChatWrap()
{
    if (reply_) reply_->abort();
}

int ChatWrap::rowCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : int(conversation_.entries().size());
}

QVariant ChatWrap::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() >= rowCount()) return {};
    const ChatEntry& e = conversation_.entries()[std::size_t(index.row())];
    switch (role) {
    case KindRole: return kindName(e.role);
    case ContentRole: return QString::fromStdString(e.content);
    case ThinkingRole: return QString::fromStdString(e.thinking);
    case StateRole: return stateName(e.state);
    case ErrorRole: return QString::fromStdString(e.error);
    case ImagesRole: {
        QStringList urls;
        for (const auto& path : e.imagePaths)
            if (QFileInfo::exists(QString::fromStdString(path))) urls << QUrl::fromLocalFile(QString::fromStdString(path)).toString();
        return urls;
    }
    case MissingImagesRole: {
        int missing = 0;
        for (const auto& path : e.imagePaths)
            if (!QFileInfo::exists(QString::fromStdString(path))) ++missing;
        return missing;
    }
    default: return {};
    }
}

QHash<int, QByteArray> ChatWrap::roleNames() const
{
    return {
        {KindRole, "kind"}, {ContentRole, "content"}, {ThinkingRole, "thinking"},
        {StateRole, "replyState"}, {ErrorRole, "error"}, {ImagesRole, "images"},
        {MissingImagesRole, "missingImages"},
    };
}

QStringList ChatWrap::pendingImages() const
{
    QStringList urls;
    for (const auto& path : pendingPaths_) urls << QUrl::fromLocalFile(path).toString();
    return urls;
}

int ChatWrap::modelIndex() const
{
    const QString current = AppSessionWrap::instance()->currentModel();
    for (int i = 0; i < models_.size(); ++i)
        if (models_[i].toMap().value("name").toString() == current) return i;
    return -1;
}

int ChatWrap::characterIndex() const
{
    const QString current = AppSessionWrap::instance()->characterId();
    for (int i = 0; i < characters_.size(); ++i)
        if (characters_[i].toMap().value("id").toString() == current) return i;
    return -1;
}

void ChatWrap::appendRow(const std::function<void()>& add)
{
    const int row = rowCount();
    beginInsertRows({}, row, row);
    add();
    endInsertRows();
}

void ChatWrap::addNotice(const QString& text)
{
    appendRow([&] { conversation_.addNotice(text.toStdString()); });
}

// ---------------------------------------------------------------- opening, new chats, saving

void ChatWrap::openCurrentChat()
{
    if (reply_) reply_->abort();
    auto* session = AppSessionWrap::instance();
    const QString id = session->currentChatId();
    const auto record = id.isEmpty() ? std::nullopt : chatStore().load(id.toStdString());
    if (!record) {
        if (!id.isEmpty()) session->setCurrentChatId(QString());   // deleted meanwhile
        startEmptyChat();
        return;
    }
    loading_ = true;
    chatId_ = id;
    title_ = QString::fromStdString(record->title);
    beginResetModel();
    conversation_.setSystemPrompt(record->systemPrompt);
    conversation_.restore(record->messages, readBase64);
    endResetModel();
    session->setActiveCharacter(QString::fromStdString(record->characterId), QString::fromStdString(record->characterName),
                                QString::fromStdString(record->systemPrompt), QString::fromStdString(record->greeting),
                                record->temperature ? *record->temperature : -1.0, QString());
    if (!record->model.empty()) session->setCurrentModel(QString::fromStdString(record->model));
    loading_ = false;
    emit busyChanged();
    emit newChatChanged();
    emit charactersChanged();
}

void ChatWrap::startEmptyChat()
{
    auto* session = AppSessionWrap::instance();
    chatId_.clear();
    title_.clear();
    beginResetModel();
    conversation_.clear();
    conversation_.setSystemPrompt(session->characterSystemPrompt().toStdString());
    if (!session->characterGreeting().isEmpty()) conversation_.addGreeting(session->characterGreeting().toStdString());
    endResetModel();
    if (!pendingPaths_.isEmpty()) {
        pendingPaths_.clear();
        emit pendingImagesChanged();
    }
    emit busyChanged();
    emit newChatChanged();
}

void ChatWrap::newChat()
{
    stop();
    auto* session = AppSessionWrap::instance();
    if (session->currentChatId().isEmpty()) startEmptyChat();
    else session->setCurrentChatId(QString());   // the change opens an empty chat
}

void ChatWrap::onCharacterChanged()
{
    auto* session = AppSessionWrap::instance();
    conversation_.setSystemPrompt(session->characterSystemPrompt().toStdString());
    if (!loading_ && isNewChat()) startEmptyChat();   // the new character's greeting
    emit charactersChanged();
    save();
}

void ChatWrap::save()
{
    if (loading_ || (chatId_.isEmpty() && !conversation_.hasUserEntry())) return;
    auto* session = AppSessionWrap::instance();
    ChatRecord record;
    record.id = chatId_.toStdString();
    record.title = title_.toStdString();
    record.model = session->currentModel().toStdString();
    record.characterId = session->characterId().toStdString();
    record.characterName = session->characterName().toStdString();
    record.systemPrompt = session->characterSystemPrompt().toStdString();
    record.greeting = session->characterGreeting().toStdString();
    if (session->hasCharacterTemperature()) record.temperature = session->characterTemperature();
    record.messages = conversation_.stored();
    std::string error;
    const std::string id = chatStore().save(record, QDateTime::currentMSecsSinceEpoch(), &error);
    if (id.empty()) {
        addNotice(QStringLiteral("Could not save this chat: %1").arg(QString::fromStdString(error)));
        return;
    }
    const bool first = chatId_.isEmpty();
    chatId_ = QString::fromStdString(id);
    if (first) session->setCurrentChatId(chatId_);
    session->notifyChatsChanged();
}

// ---------------------------------------------------------------- prompts and commands

bool ChatWrap::send(const QString& text)
{
    if (busy()) return false;
    const QString trimmed = text.trimmed();
    if (trimmed.isEmpty() && pendingPaths_.isEmpty()) return false;

    const SlashCommand command = parseSlashCommand(trimmed.toStdString());
    if (command.kind != SlashCommand::Kind::None) {
        runCommand(command);
        return true;
    }

    if (AppSessionWrap::instance()->currentModel().isEmpty()) {
        addNotice(QStringLiteral("No model chosen. Pick one below, or open /models to install one."));
        return false;
    }

    std::vector<std::string> paths;
    std::vector<std::string> images;
    for (const QString& path : std::as_const(pendingPaths_)) {
        const std::string data = readBase64(path.toStdString());
        if (data.empty()) {
            addNotice(QStringLiteral("Could not read %1").arg(path));
            return false;
        }
        paths.push_back(path.toStdString());
        images.push_back(data);
    }
    const bool first = isNewChat();
    if (first) title_ = QString::fromStdString(chatstore::titleFrom(trimmed.isEmpty() ? "Image" : trimmed.toStdString()));
    appendRow([&] { conversation_.addUser(trimmed.toStdString(), std::move(paths), std::move(images)); });
    if (!pendingPaths_.isEmpty()) {
        pendingPaths_.clear();
        emit pendingImagesChanged();
    }
    if (first) emit newChatChanged();
    save();
    startReply();
    return true;
}

void ChatWrap::runCommand(const SlashCommand& command)
{
    switch (command.kind) {
    case SlashCommand::Kind::Clear:
        newChat();
        break;
    case SlashCommand::Kind::Models:
        emit openModelsRequested();
        break;
    case SlashCommand::Kind::Characters:
        emit openCharactersRequested();
        break;
    case SlashCommand::Kind::Model: {
        const QString name = QString::fromStdString(command.argument);
        client_->send("POST", QStringLiteral("/api/show"), QByteArray::fromStdString(ollama::modelRequest(command.argument)),
                      [this, name](const OllamaClientWrap::Result& r) {
            if (r.ok()) {
                AppSessionWrap::instance()->setCurrentModel(name);
                addNotice(QStringLiteral("Model: %1").arg(name));
            } else if (r.status == 404) {
                addNotice(QStringLiteral("Model %1 is not installed. Open /models to install it.").arg(name));
            } else {
                addNotice(OllamaClientWrap::describe(r));
            }
        });
        break;
    }
    default:
        addNotice(QString::fromStdString(slashCommandError(command)));
        break;
    }
}

// ---------------------------------------------------------------- the streamed reply

void ChatWrap::startReply()
{
    auto* session = AppSessionWrap::instance();
    ollama::ChatOptions options;
    options.think = thinking_;
    if (session->hasCharacterTemperature()) options.temperature = session->characterTemperature();
    const std::string body =
        ollama::chatRequest(session->currentModel().toStdString(), conversation_.requestMessages(), options);

    appendRow([&] { replyRow_ = conversation_.startReply(); });
    emit busyChanged();
    reply_ = client_->stream(QStringLiteral("/api/chat"), QByteArray::fromStdString(body),
                             [this](const std::string& line) { onChatLine(line); },
                             [this](const OllamaClientWrap::Result& r) {
        if (r.canceled()) return;    // stop() already ended the reply
        endReply(r.ok() ? QString() : OllamaClientWrap::describe(r));
    });
}

void ChatWrap::onChatLine(const std::string& line)
{
    if (!conversation_.streaming()) return;
    const auto chunk = ollama::parseChatLine(line);
    if (!chunk) return;
    if (!chunk->error.empty()) {
        endReply(QString::fromStdString(chunk->error));
        return;
    }
    if (!chunk->thinking.empty()) conversation_.appendThinking(chunk->thinking);
    if (!chunk->content.empty()) conversation_.appendContent(chunk->content);
    if (!chunk->thinking.empty() || !chunk->content.empty()) lastRowChanged();
    if (chunk->done) {
        AppSessionWrap::instance()->setLastSpeed(chunk->evalCount, chunk->evalDurationNs);
        endReply(QString());
    }
}

void ChatWrap::endReply(const QString& error)
{
    if (!conversation_.streaming()) return;
    // No error and no "done": the server ended the stream early; what came is kept.
    if (error.isEmpty()) conversation_.finishReply();
    else conversation_.failReply(error.toStdString());
    lastRowChanged();
    emit busyChanged();
    save();
}

void ChatWrap::stop()
{
    if (!conversation_.streaming()) return;
    conversation_.stopReply();
    lastRowChanged();
    emit busyChanged();
    if (reply_) reply_->abort();
    save();
}

void ChatWrap::lastRowChanged()
{
    const QModelIndex row = index(int(replyRow_));
    emit dataChanged(row, row);
}

// ---------------------------------------------------------------- images

void ChatWrap::attach(const QList<QUrl>& files)
{
    if (!vision_) {
        addNotice(QStringLiteral("The current model cannot see images."));
        return;
    }
    bool added = false;
    for (const QUrl& url : files) {
        const QString path = url.isLocalFile() ? url.toLocalFile() : url.toString();
        const QFileInfo info(path);
        if (!info.isFile() || !isImageFile(path)) {
            addNotice(QStringLiteral("Not a PNG, JPEG or WebP image: %1").arg(info.fileName()));
        } else if (info.size() > maxImageBytes) {
            addNotice(QStringLiteral("Image larger than 20 MB: %1").arg(info.fileName()));
        } else if (!pendingPaths_.contains(info.absoluteFilePath())) {
            pendingPaths_ << info.absoluteFilePath();
            added = true;
        }
    }
    if (added) emit pendingImagesChanged();
}

void ChatWrap::removeAttachment(int index)
{
    if (index < 0 || index >= pendingPaths_.size()) return;
    pendingPaths_.removeAt(index);
    emit pendingImagesChanged();
}

// ---------------------------------------------------------------- pickers

void ChatWrap::pickModel(int index)
{
    if (index < 0 || index >= models_.size()) return;
    AppSessionWrap::instance()->setCurrentModel(models_[index].toMap().value("name").toString());
}

void ChatWrap::pickCharacter(int index)
{
    if (index < 0 || index >= characters_.size()) return;
    const QString id = characters_[index].toMap().value("id").toString();
    const CharacterStore store((AppSessionWrap::dataFolder() + QStringLiteral("/characters")).toStdString());
    for (const auto& c : store.loadAll()) {
        if (QString::fromStdString(c.id) != id) continue;
        AppSessionWrap::instance()->setActiveCharacter(
            id, QString::fromStdString(c.name), QString::fromStdString(c.systemPrompt),
            QString::fromStdString(c.greeting), c.temperature ? *c.temperature : -1.0,
            QString::fromStdString(c.preferredModel));
        return;
    }
}

void ChatWrap::loadCharacters()
{
    const CharacterStore store((AppSessionWrap::dataFolder() + QStringLiteral("/characters")).toStdString());
    characters_.clear();
    for (const auto& c : store.loadAll()) {
        characters_ << QVariantMap{
            {"id", QString::fromStdString(c.id)},
            {"name", QString::fromStdString(c.name)},
            {"greeting", QString::fromStdString(c.greeting)},
        };
    }
    emit charactersChanged();
}

void ChatWrap::loadModels()
{
    client_->get(QStringLiteral("/api/tags"), [this](const OllamaClientWrap::Result& r) {
        const auto tags = r.ok() ? ollama::parseTags(std::string_view(r.body.constData(), std::size_t(r.body.size())))
                                 : std::nullopt;
        if (!tags) return;   // keep the last list while Ollama is away
        models_.clear();
        for (const auto& m : *tags) {
            QString details = gigabytes(m.sizeBytes);
            if (!m.parameterSize.empty()) details += QStringLiteral(" · ") + QString::fromStdString(m.parameterSize);
            models_ << QVariantMap{{"name", QString::fromStdString(m.name)}, {"details", details}, {"vision", false}};
        }
        const QString current = AppSessionWrap::instance()->currentModel();
        if (!current.isEmpty() && modelIndex() < 0)
            models_ << QVariantMap{{"name", current}, {"details", QStringLiteral("not installed")}, {"vision", false}};
        emit modelsChanged();
    }, 5000);
}

void ChatWrap::loadCapabilities()
{
    const QString model = AppSessionWrap::instance()->currentModel();
    const auto set = [this, model](bool vision, bool thinking) {
        capabilitiesModel_ = model;
        if (vision == vision_ && thinking == thinking_) return;
        vision_ = vision;
        thinking_ = thinking;
        if (!vision_ && !pendingPaths_.isEmpty()) {
            pendingPaths_.clear();
            emit pendingImagesChanged();
        }
        emit capabilitiesChanged();
    };
    if (model.isEmpty()) {
        set(false, false);
        return;
    }
    client_->send("POST", QStringLiteral("/api/show"), QByteArray::fromStdString(ollama::modelRequest(model.toStdString())),
                  [this, model, set](const OllamaClientWrap::Result& r) {
        if (model != AppSessionWrap::instance()->currentModel()) return;   // changed meanwhile
        const auto info = r.ok() ? ollama::parseShow(std::string_view(r.body.constData(), std::size_t(r.body.size())))
                                 : std::nullopt;
        if (info) set(info->has("vision"), info->has("thinking"));
        else {
            set(false, false);
            capabilitiesModel_.clear();   // ask again once Ollama answers
        }
    });
}
