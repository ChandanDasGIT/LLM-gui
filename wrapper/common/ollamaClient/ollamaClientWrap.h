#pragma once

#include <QByteArray>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QObject>
#include <QString>

#include <functional>
#include <string>

class QIODevice;

// Every HTTP call to Ollama goes through here. Streamed replies are handed over line by line
// as they arrive. The server address is the session's (AppSessionWrap::serverUrl).
class OllamaClientWrap : public QObject {
    Q_OBJECT

public:
    struct Result {
        int status = 0;                                 // HTTP status; 0 when none arrived
        QByteArray body;                                // for streams: what was not a complete line
        QNetworkReply::NetworkError error = QNetworkReply::NoError;
        QString errorString;
        bool ok() const { return error == QNetworkReply::NoError && status >= 200 && status < 300; }
        bool canceled() const { return error == QNetworkReply::OperationCanceledError; }
    };
    using DoneHandler = std::function<void(const Result&)>;
    using LineHandler = std::function<void(const std::string& line)>;

    explicit OllamaClientWrap(QObject* parent = nullptr);

    // timeoutMs 0: no limit (a model can take long to load).
    QNetworkReply* get(const QString& path, DoneHandler done, int timeoutMs = 0);
    QNetworkReply* send(const QByteArray& verb, const QString& path, const QByteArray& body,
                        DoneHandler done, int timeoutMs = 0);
    QNetworkReply* stream(const QString& path, const QByteArray& body, LineHandler onLine, DoneHandler done);
    // The device must stay open until done is called.
    QNetworkReply* upload(const QString& path, QIODevice* data, qint64 size, DoneHandler done);

    // One line for the user: the server's own error text, "Ollama is not running at ...", or the
    // network error.
    static QString describe(const Result& result);

private:
    QNetworkRequest request(const QString& path, int timeoutMs) const;
    QNetworkReply* track(QNetworkReply* reply, DoneHandler done);

    QNetworkAccessManager network_;
};
