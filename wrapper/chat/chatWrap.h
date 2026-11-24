#pragma once

#include "chat/conversation.h"
#include "chat/slashcommands.h"

#include <QAbstractListModel>
#include <QPointer>
#include <QStringList>
#include <QUrl>
#include <QVariantList>
#include <QtQml/qqmlregistration.h>

#include <functional>

class OllamaClientWrap;
class QNetworkReply;

// The chat for QML: the open chat's transcript as a list model (one row per message), the
// installed models and the characters for the pickers, and the prompt actions. Replies stream
// from Ollama's /api/chat. A chat is saved (backend chatstore) once its first prompt is sent.
class ChatWrap : public QAbstractListModel {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
    Q_PROPERTY(bool isNewChat READ isNewChat NOTIFY newChatChanged)
    Q_PROPERTY(bool canAttach READ canAttach NOTIFY capabilitiesChanged)
    Q_PROPERTY(bool thinks READ thinks NOTIFY capabilitiesChanged)
    Q_PROPERTY(QStringList pendingImages READ pendingImages NOTIFY pendingImagesChanged)
    Q_PROPERTY(QVariantList models READ models NOTIFY modelsChanged)
    Q_PROPERTY(int modelIndex READ modelIndex NOTIFY modelsChanged)
    Q_PROPERTY(QVariantList characters READ characters NOTIFY charactersChanged)
    Q_PROPERTY(int characterIndex READ characterIndex NOTIFY charactersChanged)

public:
    enum Roles {
        KindRole = Qt::UserRole + 1,   // "user", "assistant" or "notice"
        ContentRole,
        ThinkingRole,
        StateRole,                     // "streaming", "finished", "stopped" or "failed"
        ErrorRole,
        ImagesRole,                    // file URLs of the images still on disk
        MissingImagesRole,             // how many were moved or deleted since
    };

    explicit ChatWrap(QObject* parent = nullptr);
    ~ChatWrap() override;

    int rowCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    bool busy() const { return conversation_.streaming(); }
    bool isNewChat() const { return !conversation_.hasUserEntry(); }
    bool canAttach() const { return vision_; }
    bool thinks() const { return thinking_; }
    QStringList pendingImages() const;
    // Rows: name, details, vision. The current model is listed even when Ollama lacks it.
    QVariantList models() const { return models_; }
    int modelIndex() const;
    // Rows: id, name, greeting.
    QVariantList characters() const { return characters_; }
    int characterIndex() const;

    // A prompt or a slash command. False when nothing was done (empty, or a reply is running),
    // so the prompt box keeps its text.
    Q_INVOKABLE bool send(const QString& text);
    Q_INVOKABLE void stop();
    Q_INVOKABLE void attach(const QList<QUrl>& files);
    Q_INVOKABLE void removeAttachment(int index);
    Q_INVOKABLE void pickModel(int index);
    Q_INVOKABLE void pickCharacter(int index);
    // A new, empty chat with the current character's greeting.
    Q_INVOKABLE void newChat();

signals:
    void busyChanged();
    void newChatChanged();
    void capabilitiesChanged();
    void pendingImagesChanged();
    void modelsChanged();
    void charactersChanged();
    void openModelsRequested();
    void openCharactersRequested();

private:
    void openCurrentChat();
    void startEmptyChat();
    void runCommand(const SlashCommand& command);
    void startReply();
    void onChatLine(const std::string& line);
    void endReply(const QString& error);
    void lastRowChanged();
    void appendRow(const std::function<void()>& add);
    void addNotice(const QString& text);
    void loadCapabilities();
    void loadModels();
    void loadCharacters();
    void onCharacterChanged();
    void save();

    Conversation conversation_;
    QString chatId_;                   // empty until the first prompt is sent
    QString title_;
    bool loading_ = false;             // opening a saved chat: changes are not saved back
    OllamaClientWrap* client_ = nullptr;
    QPointer<QNetworkReply> reply_;
    std::size_t replyRow_ = 0;
    QStringList pendingPaths_;
    bool vision_ = false;
    bool thinking_ = false;
    QString capabilitiesModel_;
    QVariantList models_;
    QVariantList characters_;
};
