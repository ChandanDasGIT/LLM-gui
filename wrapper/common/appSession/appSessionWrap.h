#pragma once

#include <QObject>
#include <QString>
#include <QTimer>
#include <QtQml/qqmlregistration.h>

class OllamaClientWrap;
class QQmlEngine;
class QJSEngine;

// What every part of the app shares: the Ollama address and whether it answers, the open chat
// with its model and character, and the speed of the last reply. Kept in QSettings
// (~/.config/LLM-gui/LLM-gui.conf). Views tell each other about changes through the notify calls.
class AppSessionWrap : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
    Q_PROPERTY(QString serverUrl READ serverUrl WRITE setServerUrl NOTIFY serverUrlChanged)
    Q_PROPERTY(QString currentModel READ currentModel WRITE setCurrentModel NOTIFY currentModelChanged)
    Q_PROPERTY(int connection READ connection NOTIFY connectionChanged)
    Q_PROPERTY(QString serverVersion READ serverVersion NOTIFY connectionChanged)
    Q_PROPERTY(QString characterId READ characterId NOTIFY characterChanged)
    Q_PROPERTY(QString characterName READ characterName NOTIFY characterChanged)
    Q_PROPERTY(QString currentChatId READ currentChatId NOTIFY currentChatChanged)

public:
    // Same order as statusstrip::Connection.
    enum Connection { Checking, Connected, Offline };
    Q_ENUM(Connection)

    static AppSessionWrap* instance();
    static AppSessionWrap* create(QQmlEngine*, QJSEngine*);

    QString serverUrl() const { return serverUrl_; }
    void setServerUrl(const QString& url);
    QString currentModel() const { return currentModel_; }
    void setCurrentModel(const QString& model);

    int connection() const { return connection_; }
    QString serverVersion() const { return serverVersion_; }
    Q_INVOKABLE void checkConnection();

    QString characterId() const { return characterId_; }
    QString characterName() const { return characterName_; }
    QString characterSystemPrompt() const { return characterSystemPrompt_; }
    QString characterGreeting() const { return characterGreeting_; }
    bool hasCharacterTemperature() const { return characterTemperature_ >= 0.0; }
    double characterTemperature() const { return characterTemperature_; }
    // temperature < 0: none. A preferred model, when given, becomes the current model.
    void setActiveCharacter(const QString& id, const QString& name, const QString& systemPrompt,
                            const QString& greeting, double temperature, const QString& preferredModel);

    long long lastEvalCount() const { return lastEvalCount_; }
    long long lastEvalDurationNs() const { return lastEvalDurationNs_; }
    void setLastSpeed(long long evalCount, long long evalDurationNs);

    // Empty: a new chat that is not saved yet.
    QString currentChatId() const { return currentChatId_; }
    void setCurrentChatId(const QString& id);

    // ~/.local/share/LLM-gui (characters/ and chats/ live in it).
    static QString dataFolder();

    void notifyChatsChanged() { emit chatsChanged(); }
    void notifyCharactersChanged() { emit charactersChanged(); }
    void notifyModelsChanged() { emit modelsChanged(); }

    // Reads everything again from QSettings and checks the connection.
    void reloadSettings();

signals:
    void serverUrlChanged();
    void currentModelChanged();
    void connectionChanged();
    void characterChanged();
    void speedChanged();
    void currentChatChanged();
    void chatsChanged();              // a chat was saved or deleted
    void charactersChanged();         // a character was saved or deleted
    void modelsChanged();             // a model was installed or removed

private:
    explicit AppSessionWrap(QObject* parent = nullptr);
    void setConnection(Connection connection, const QString& version);

    QString serverUrl_;
    QString currentModel_;
    Connection connection_ = Checking;
    QString serverVersion_;
    QString currentChatId_;
    QString characterId_;
    QString characterName_;
    QString characterSystemPrompt_;
    QString characterGreeting_;
    double characterTemperature_ = -1.0;
    long long lastEvalCount_ = 0;
    long long lastEvalDurationNs_ = 0;
    OllamaClientWrap* client_ = nullptr;
    QTimer poll_;
    bool checking_ = false;
};
