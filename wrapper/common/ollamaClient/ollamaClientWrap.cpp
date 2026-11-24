#include "wrapper/common/ollamaClient/ollamaClientWrap.h"

#include "wrapper/common/appSession/appSessionWrap.h"

#include "common/ollamaapi.h"

#include <QIODevice>
#include <QNetworkRequest>
#include <QUrl>

#include <memory>

OllamaClientWrap::OllamaClientWrap(QObject* parent) : QObject(parent) {}

QNetworkRequest OllamaClientWrap::request(const QString& path, int timeoutMs) const
{
    QString base = AppSessionWrap::instance()->serverUrl();
    while (base.endsWith('/')) base.chop(1);
    QNetworkRequest req(QUrl(base + path));
    req.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    req.setTransferTimeout(timeoutMs);
    return req;
}

QNetworkReply* OllamaClientWrap::track(QNetworkReply* reply, DoneHandler done)
{
    connect(reply, &QNetworkReply::finished, this, [reply, done = std::move(done)] {
        Result result;
        result.status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        result.body = reply->isOpen() ? reply->readAll() : QByteArray();   // an aborted reply is closed
        result.error = reply->error();
        result.errorString = reply->errorString();
        // An HTTP error status is reported by status and body, not as a network failure.
        if (result.status >= 400 && result.error != QNetworkReply::OperationCanceledError)
            result.error = QNetworkReply::ProtocolFailure;
        reply->deleteLater();
        if (done) done(result);
    });
    return reply;
}

QNetworkReply* OllamaClientWrap::get(const QString& path, DoneHandler done, int timeoutMs)
{
    return track(network_.get(request(path, timeoutMs)), std::move(done));
}

QNetworkReply* OllamaClientWrap::send(const QByteArray& verb, const QString& path, const QByteArray& body,
                                      DoneHandler done, int timeoutMs)
{
    if (verb == "HEAD") return track(network_.head(request(path, timeoutMs)), std::move(done));
    return track(network_.sendCustomRequest(request(path, timeoutMs), verb, body), std::move(done));
}

QNetworkReply* OllamaClientWrap::stream(const QString& path, const QByteArray& body, LineHandler onLine,
                                        DoneHandler done)
{
    QNetworkReply* reply = network_.post(request(path, 0), body);
    auto lines = std::make_shared<ollama::LineBuffer>();
    connect(reply, &QIODevice::readyRead, this, [reply, lines, onLine] {
        const QByteArray bytes = reply->readAll();
        for (const auto& line : lines->feed(std::string_view(bytes.constData(), std::size_t(bytes.size()))))
            onLine(line);
    });
    return track(reply, [lines, onLine, done = std::move(done)](const Result& r) {
        Result result = r;
        // Bytes that arrived with the end, then a last line without a line end.
        if (!result.body.isEmpty())
            for (const auto& line : lines->feed(std::string_view(result.body.constData(), std::size_t(result.body.size()))))
                onLine(line);
        const std::string rest = lines->takeRest();
        if (!rest.empty() && !result.canceled()) onLine(rest);
        result.body.clear();
        if (done) done(result);
    });
}

QNetworkReply* OllamaClientWrap::upload(const QString& path, QIODevice* data, qint64 size, DoneHandler done)
{
    QNetworkRequest req = request(path, 0);
    req.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/octet-stream"));
    req.setHeader(QNetworkRequest::ContentLengthHeader, size);
    return track(network_.post(req, data), std::move(done));
}

QString OllamaClientWrap::describe(const Result& result)
{
    const std::string serverText =
        ollama::errorText(std::string_view(result.body.constData(), std::size_t(result.body.size())));
    if (!serverText.empty()) return QString::fromStdString(serverText);
    switch (result.error) {
    case QNetworkReply::NoError:
        return QString();
    case QNetworkReply::ConnectionRefusedError:
    case QNetworkReply::HostNotFoundError:
    case QNetworkReply::RemoteHostClosedError:
        return QStringLiteral("Ollama is not running at %1").arg(AppSessionWrap::instance()->serverUrl());
    case QNetworkReply::OperationCanceledError:
        return QStringLiteral("Canceled");
    case QNetworkReply::ProtocolFailure:
        if (result.status >= 400) return QStringLiteral("Ollama answered with HTTP %1").arg(result.status);
        [[fallthrough]];
    default:
        return result.errorString;
    }
}
