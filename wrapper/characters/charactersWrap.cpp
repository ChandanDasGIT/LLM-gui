#include "wrapper/characters/charactersWrap.h"

#include "wrapper/common/appSession/appSessionWrap.h"

#include "common/characterstore.h"

namespace {

CharacterStore characterStore()
{
    return CharacterStore((AppSessionWrap::dataFolder() + QStringLiteral("/characters")).toStdString());
}

} // namespace

CharactersWrap::CharactersWrap(QObject* parent) : QAbstractListModel(parent)
{
    auto* session = AppSessionWrap::instance();
    connect(session, &AppSessionWrap::charactersChanged, this, &CharactersWrap::reload);
    connect(session, &AppSessionWrap::characterChanged, this, [this] {
        if (rowCount() > 0) emit dataChanged(index(0), index(rowCount() - 1), {ActiveRole});
        emit activeChanged();
    });
    reload();
}

void CharactersWrap::reload()
{
    std::vector<std::string> problems;
    beginResetModel();
    characters_ = characterStore().loadAll(&problems);
    endResetModel();
    emit activeChanged();
    if (!problems.empty())
        setMessage(QStringLiteral("Skipped unreadable character files: %1")
                       .arg(QString::fromStdString(problems.front()) + (problems.size() > 1 ? QStringLiteral(" …") : QString())));
}

int CharactersWrap::rowCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : int(characters_.size());
}

QVariant CharactersWrap::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() >= rowCount()) return {};
    const Character& c = characters_[std::size_t(index.row())];
    switch (role) {
    case IdRole: return QString::fromStdString(c.id);
    case NameRole: return QString::fromStdString(c.name);
    case SystemPromptRole: return QString::fromStdString(c.systemPrompt);
    case GreetingRole: return QString::fromStdString(c.greeting);
    case PreferredModelRole: return QString::fromStdString(c.preferredModel);
    case UseTemperatureRole: return c.temperature.has_value();
    case TemperatureRole: return c.temperature.value_or(0.8);
    case BuiltInRole: return c.isDefault();
    case ActiveRole: return QString::fromStdString(c.id) == AppSessionWrap::instance()->characterId();
    default: return {};
    }
}

QHash<int, QByteArray> CharactersWrap::roleNames() const
{
    return {
        {IdRole, "characterId"}, {NameRole, "name"}, {SystemPromptRole, "systemPrompt"},
        {GreetingRole, "greeting"}, {PreferredModelRole, "preferredModel"},
        {UseTemperatureRole, "useTemperature"}, {TemperatureRole, "temperature"},
        {BuiltInRole, "builtIn"}, {ActiveRole, "active"},
    };
}

QVariantMap CharactersWrap::get(int row) const
{
    QVariantMap out;
    if (row < 0 || row >= rowCount()) return out;
    const auto roles = roleNames();
    for (auto it = roles.cbegin(); it != roles.cend(); ++it) out.insert(QString::fromUtf8(it.value()), data(index(row), it.key()));
    return out;
}

int CharactersWrap::activeIndex() const
{
    return rowOf(AppSessionWrap::instance()->characterId().toStdString());
}

int CharactersWrap::rowOf(const std::string& id) const
{
    for (std::size_t i = 0; i < characters_.size(); ++i)
        if (characters_[i].id == id) return int(i);
    return -1;
}

void CharactersWrap::setMessage(const QString& text)
{
    if (text == message_) return;
    message_ = text;
    emit messageChanged();
}

void CharactersWrap::pushToSession(const Character& c, bool withPreferredModel)
{
    AppSessionWrap::instance()->setActiveCharacter(
        QString::fromStdString(c.id), QString::fromStdString(c.name), QString::fromStdString(c.systemPrompt),
        QString::fromStdString(c.greeting), c.temperature ? *c.temperature : -1.0,
        withPreferredModel ? QString::fromStdString(c.preferredModel) : QString());
}

int CharactersWrap::save(int row, const QString& name, const QString& systemPrompt, const QString& greeting,
                         const QString& preferredModel, bool useTemperature, double temperature)
{
    if (row < 0 || row >= rowCount()) return -1;
    Character c = characters_[std::size_t(row)];
    c.name = name.trimmed().toStdString();
    c.systemPrompt = systemPrompt.toStdString();
    c.greeting = greeting.toStdString();
    c.preferredModel = preferredModel.trimmed().toStdString();
    c.temperature = useTemperature ? std::optional<double>(temperature) : std::nullopt;
    std::string error;
    const std::string id = characterStore().save(c, &error);
    if (id.empty()) {
        setMessage(QString::fromStdString(error));
        return -1;
    }
    setMessage(QString());
    c.id = id;
    const bool active = QString::fromStdString(id) == AppSessionWrap::instance()->characterId();
    AppSessionWrap::instance()->notifyCharactersChanged();   // reloads this list too
    if (active) pushToSession(c, false);                       // the open chat uses the edit at once
    return rowOf(id);
}

int CharactersWrap::create()
{
    Character c;
    c.name = "New character";
    std::string error;
    const std::string id = characterStore().save(c, &error);
    if (id.empty()) {
        setMessage(QString::fromStdString(error));
        return -1;
    }
    setMessage(QString());
    AppSessionWrap::instance()->notifyCharactersChanged();
    return rowOf(id);
}

int CharactersWrap::duplicate(int row)
{
    if (row < 0 || row >= rowCount()) return -1;
    std::string error;
    const std::string id = characterStore().duplicate(characters_[std::size_t(row)].id, &error);
    if (id.empty()) {
        setMessage(QString::fromStdString(error));
        return -1;
    }
    setMessage(QString());
    AppSessionWrap::instance()->notifyCharactersChanged();
    return rowOf(id);
}

bool CharactersWrap::remove(int row)
{
    if (row < 0 || row >= rowCount()) return false;
    const Character gone = characters_[std::size_t(row)];
    std::string error;
    if (!characterStore().remove(gone.id, &error)) {
        setMessage(QString::fromStdString(error));
        return false;
    }
    setMessage(QString());
    const bool wasActive = QString::fromStdString(gone.id) == AppSessionWrap::instance()->characterId();
    AppSessionWrap::instance()->notifyCharactersChanged();
    if (wasActive) pushToSession(Character::builtInDefault(), false);
    return true;
}

void CharactersWrap::use(int row)
{
    if (row < 0 || row >= rowCount()) return;
    pushToSession(characters_[std::size_t(row)], true);
}
