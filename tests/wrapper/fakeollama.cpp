#include "fakeollama.h"
#include "testkit.h"

#include <cstdio>

#include <QPointer>
#include <QTcpSocket>
#include <QTimer>

#include <memory>

FakeOllama::FakeOllama(QObject* parent) : QObject(parent)
{
    server_.listen(QHostAddress::LocalHost, 0);
    connect(&server_, &QTcpServer::newConnection, this, [this] {
        while (auto* socket = server_.nextPendingConnection()) serve(socket);
    });
}

QString FakeOllama::url() const
{
    return QStringLiteral("http://127.0.0.1:%1").arg(server_.serverPort());
}

void FakeOllama::on(const QByteArray& method, const QByteArray& path, const Response& response)
{
    routes_[method + ' ' + path] = response;
}

void FakeOllama::onJson(const QByteArray& method, const QByteArray& path, const QByteArray& json, int status)
{
    Response r;
    r.status = status;
    r.body = json;
    on(method, path, r);
}

QList<FakeOllama::Request> FakeOllama::requestsTo(const QByteArray& path) const
{
    QList<Request> out;
    for (const auto& r : requests)
        if (r.path == path) out << r;
    return out;
}

void FakeOllama::serve(QTcpSocket* socket)
{
    auto buffer = std::make_shared<QByteArray>();
    auto answered = std::make_shared<bool>(false);
    connect(socket, &QTcpSocket::disconnected, socket, &QObject::deleteLater);
    connect(socket, &QTcpSocket::readyRead, this, [this, socket, buffer, answered] {
        buffer->append(socket->readAll());
        if (*answered) return;
        const int headerEnd = buffer->indexOf("\r\n\r\n");
        if (headerEnd < 0) return;
        const QList<QByteArray> headerLines = buffer->left(headerEnd).split('\n');
        const QList<QByteArray> first = headerLines.value(0).trimmed().split(' ');
        qint64 length = 0;
        for (const auto& line : headerLines) {
            const int colon = line.indexOf(':');
            if (colon > 0 && line.left(colon).trimmed().toLower() == "content-length")
                length = line.mid(colon + 1).trimmed().toLongLong();
        }
        if (buffer->size() - (headerEnd + 4) < length) return;
        Request request{first.value(0), first.value(1), buffer->mid(headerEnd + 4, length)};
        *answered = true;
        requests << request;
        respond(socket, request);
    });
}

void FakeOllama::respond(QTcpSocket* socket, const Request& request)
{
    const auto route = routes_.constFind(request.method + ' ' + request.path);
    Response response;
    if (route == routes_.constEnd()) {
        response.status = 404;
        response.body = "{\"error\":\"no fake route for " + request.method + ' ' + request.path + "\"}";
    } else {
        response = *route;
    }

    QByteArray head = "HTTP/1.1 " + QByteArray::number(response.status) + " Fake\r\n"
                      "Content-Type: application/json\r\nConnection: close\r\n";
    if (request.method == "HEAD") {
        socket->write(head + "Content-Length: 0\r\n\r\n");
        socket->disconnectFromHost();
        return;
    }
    if (response.lines.isEmpty()) {
        socket->write(head + "Content-Length: " + QByteArray::number(response.body.size()) + "\r\n\r\n" + response.body);
        socket->disconnectFromHost();
        return;
    }
    socket->write(head + "\r\n");
    QPointer<QTcpSocket> guard(socket);
    for (int i = 0; i < response.lines.size(); ++i) {
        const QByteArray line = response.lines[i] + '\n';
        QTimer::singleShot(response.lineDelayMs * (i + 1), socket, [guard, line] {
            if (guard) guard->write(line);
        });
    }
    const int end = response.lineDelayMs * (int(response.lines.size()) + 1);
    if (response.holdOpen) {
        connect(socket, &QTcpSocket::disconnected, this, [this] { ++closedByClient; });
    } else {
        QTimer::singleShot(end, socket, [guard] {
            if (guard) guard->disconnectFromHost();
        });
    }
}

#include "wrapper/common/appSession/appSessionWrap.h"

#include <QCoreApplication>
#include <QSettings>
#include <QTemporaryDir>
#include <QtTest>

namespace {
// Any Qt log line during a wrapper test is a failure, as it is for the app (the log must stay empty).
int qtLogLines = 0;
void countLogLine(QtMsgType, const QMessageLogContext&, const QString& message)
{
    ++qtLogLines;
    std::fprintf(stderr, "    Qt log: %s\n", qPrintable(message));
}
}

FakeSession::FakeSession()
{
    qInstallMessageHandler(countLogLine);
    qtLogLines = 0;
    if (!QCoreApplication::instance()) {
        static int argc = 1;
        static char name[] = "wrappertests";
        static char* argv[]{name, nullptr};
        static QCoreApplication app(argc, argv);
    }
    folder = new QTemporaryDir(QDir::tempPath() + "/llmgui-wrappertests-XXXXXX");
    QCoreApplication::setOrganizationName("LLM-gui-tests");
    QCoreApplication::setApplicationName("LLM-gui-tests");
    QSettings::setPath(QSettings::NativeFormat, QSettings::UserScope, folder->path() + "/config");
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, folder->path() + "/config");
    qputenv("XDG_DATA_HOME", (folder->path() + "/data").toLocal8Bit());
    qputenv("XDG_CONFIG_HOME", (folder->path() + "/config").toLocal8Bit());
    QSettings().clear();

    ollama = new FakeOllama;
    ollama->onJson("GET", "/api/version", R"({"version":"0.40.16"})");
    QSettings().setValue("ollama/url", ollama->url());
    QSettings().sync();
    AppSessionWrap::instance()->reloadSettings();
}

FakeSession::~FakeSession()
{
    // The session's connection check must not be left running on Qt's network thread.
    (void)QTest::qWaitFor([] { return AppSessionWrap::instance()->connection() != AppSessionWrap::Checking; }, 3000);
    if (qtLogLines > 0) testkit::fail(__FILE__, __LINE__, "Qt wrote to the log during this test");
    delete ollama;
    QSettings().clear();
    delete folder;
}

QString FakeSession::dataFolder() const
{
    return folder->path() + "/data";
}
