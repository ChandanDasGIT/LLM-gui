#include "wrapper/common/appSession/appSessionWrap.h"

#include "wrapper/common/ollamaClient/ollamaClientWrap.h"

#include "common/ollamaapi.h"

#include <QCoreApplication>
#include <QJSEngine>
#include <QQmlEngine>
#include <QSettings>
#include <QStandardPaths>

namespace {
const QString defaultServerUrl = QStringLiteral("http://localhost:11434");
constexpr int pollMs = 5000;
constexpr int checkTimeoutMs = 3000;
}

namespace {
AppSessionWrap* theSession = nullptr;
}

AppSessionWrap* AppSessionWrap::instance()
{
    if (!theSession) new AppSessionWrap(QCoreApplication::instance());   // the constructor sets theSession
    return theSession;
}

AppSessionWrap* AppSessionWrap::create(QQmlEngine*, QJSEngine*)
{
    AppSessionWrap* session = instance();
    QJSEngine::setObjectOwnership(session, QJSEngine::CppOwnership);
    return session;
}

AppSessionWrap::AppSessionWrap(QObject* parent) : QObject(parent)
{
    theSession = this;   // first, so what the constructor starts can reach the session
    client_ = new OllamaClientWrap(this);
    poll_.setInterval(pollMs);
    connect(&poll_, &QTimer::timeout, this, &AppSessionWrap::checkConnection);
    reloadSettings();
    poll_.start();
}

void AppSessionWrap::reloadSettings()
{
    QSettings settings;
    serverUrl_ = settings.value("ollama/url", defaultServerUrl).toString().trimmed();
    if (serverUrl_.isEmpty()) serverUrl_ = defaultServerUrl;
    currentModel_ = settings.value("chat/model").toString();
    currentChatId_ = settings.value("chat/current").toString();
    characterId_ = settings.value("character/id", "default").toString();
    characterName_ = settings.value("character/name", "Default").toString();
    characterSystemPrompt_ = settings.value("character/systemPrompt").toString();
    characterGreeting_ = settings.value("character/greeting").toString();
    characterTemperature_ = settings.value("character/temperature", -1.0).toDouble();
    lastEvalCount_ = 0;
    lastEvalDurationNs_ = 0;
    emit serverUrlChanged();
    emit currentModelChanged();
    emit characterChanged();
    emit speedChanged();
    emit currentChatChanged();
    setConnection(Checking, QString());
    checking_ = false;
    checkConnection();
}

void AppSessionWrap::setCurrentChatId(const QString& id)
{
    if (id == currentChatId_) return;
    currentChatId_ = id;
    QSettings().setValue("chat/current", id);
    emit currentChatChanged();
}

QString AppSessionWrap::dataFolder()
{
    return QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + QStringLiteral("/LLM-gui");
}

void AppSessionWrap::setServerUrl(const QString& url)
{
    const QString clean = url.trimmed().isEmpty() ? defaultServerUrl : url.trimmed();
    if (clean == serverUrl_) return;
    serverUrl_ = clean;
    QSettings().setValue("ollama/url", serverUrl_);
    emit serverUrlChanged();
    setConnection(Checking, QString());
    checking_ = false;
    checkConnection();
}

void AppSessionWrap::setCurrentModel(const QString& model)
{
    if (model == currentModel_) return;
    currentModel_ = model;
    QSettings().setValue("chat/model", model);
    emit currentModelChanged();
}

void AppSessionWrap::checkConnection()
{
    if (checking_) return;
    checking_ = true;
    const QString askedUrl = serverUrl_;
    client_->get(QStringLiteral("/api/version"), [this, askedUrl](const OllamaClientWrap::Result& r) {
        checking_ = false;
        if (askedUrl != serverUrl_) return;   // the address changed meanwhile
        const auto version = r.ok() ? ollama::parseVersion(std::string_view(r.body.constData(), std::size_t(r.body.size())))
                                    : std::nullopt;
        if (version) setConnection(Connected, QString::fromStdString(*version));
        else setConnection(Offline, QString());
    }, checkTimeoutMs);
}

void AppSessionWrap::setConnection(Connection connection, const QString& version)
{
    if (connection == connection_ && version == serverVersion_) return;
    connection_ = connection;
    serverVersion_ = version;
    emit connectionChanged();
}

void AppSessionWrap::setActiveCharacter(const QString& id, const QString& name, const QString& systemPrompt,
                                        const QString& greeting, double temperature, const QString& preferredModel)
{
    characterId_ = id;
    characterName_ = name;
    characterSystemPrompt_ = systemPrompt;
    characterGreeting_ = greeting;
    characterTemperature_ = temperature < 0.0 ? -1.0 : temperature;
    QSettings settings;
    settings.setValue("character/id", characterId_);
    settings.setValue("character/name", characterName_);
    settings.setValue("character/systemPrompt", characterSystemPrompt_);
    settings.setValue("character/greeting", characterGreeting_);
    settings.setValue("character/temperature", characterTemperature_);
    if (!preferredModel.isEmpty()) setCurrentModel(preferredModel);
    emit characterChanged();
}

void AppSessionWrap::setLastSpeed(long long evalCount, long long evalDurationNs)
{
    lastEvalCount_ = evalCount;
    lastEvalDurationNs_ = evalDurationNs;
    emit speedChanged();
}
