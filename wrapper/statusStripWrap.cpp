#include "wrapper/statusStripWrap.h"

#include "wrapper/common/appSession/appSessionWrap.h"

#include "statusstrip.h"

StatusStripWrap::StatusStripWrap(QObject* parent) : QObject(parent)
{
    auto* session = AppSessionWrap::instance();
    connect(session, &AppSessionWrap::currentModelChanged, this, &StatusStripWrap::changed);
    connect(session, &AppSessionWrap::characterChanged, this, &StatusStripWrap::changed);
    connect(session, &AppSessionWrap::connectionChanged, this, &StatusStripWrap::changed);
    connect(session, &AppSessionWrap::speedChanged, this, &StatusStripWrap::changed);
}

QString StatusStripWrap::modelText() const
{
    return QString::fromStdString(statusstrip::modelText(AppSessionWrap::instance()->currentModel().toStdString()));
}

QString StatusStripWrap::characterText() const
{
    return QString::fromStdString(statusstrip::characterText(AppSessionWrap::instance()->characterName().toStdString()));
}

QString StatusStripWrap::connectionText() const
{
    auto* session = AppSessionWrap::instance();
    return QString::fromStdString(statusstrip::connectionText(
        statusstrip::Connection(session->connection()), session->serverVersion().toStdString()));
}

bool StatusStripWrap::connected() const
{
    return AppSessionWrap::instance()->connection() == AppSessionWrap::Connected;
}

bool StatusStripWrap::offline() const
{
    return AppSessionWrap::instance()->connection() == AppSessionWrap::Offline;
}

QString StatusStripWrap::speedText() const
{
    auto* session = AppSessionWrap::instance();
    return QString::fromStdString(statusstrip::tokensPerSecondText(session->lastEvalCount(), session->lastEvalDurationNs()));
}
