#pragma once

#include <QByteArray>
#include <QHash>
#include <QList>
#include <QObject>
#include <QString>
#include <QDir>
#include <QTcpServer>

// A stand-in for Ollama on 127.0.0.1 and a free port, so wrapper tests never reach the real
// server. Each route answers with a fixed body, or with streamed lines (one JSON per line).
class FakeOllama : public QObject {
    Q_OBJECT

public:
    struct Response {
        int status = 200;
        QByteArray body;               // sent whole when lines is empty
        QList<QByteArray> lines;       // streamed, each followed by "\n"
        int lineDelayMs = 5;
        bool holdOpen = false;         // after the lines, keep the connection open until the client closes it
    };
    struct Request {
        QByteArray method;
        QByteArray path;
        QByteArray body;
    };

    explicit FakeOllama(QObject* parent = nullptr);
    QString url() const;

    void on(const QByteArray& method, const QByteArray& path, const Response& response);
    void onJson(const QByteArray& method, const QByteArray& path, const QByteArray& json, int status = 200);

    QList<Request> requests;
    // Requests for this path, in order.
    QList<Request> requestsTo(const QByteArray& path) const;
    // Held streams the client closed (a stop or cancel reached the server).
    int closedByClient = 0;

private:
    void serve(class QTcpSocket* socket);
    void respond(QTcpSocket* socket, const Request& request);

    QTcpServer server_;
    QHash<QByteArray, Response> routes_;
};

class QTemporaryDir;

// Every wrapper test starts with this: an app object, throwaway settings and data folders
// (XDG_DATA_HOME points into them), a fresh fake server, and the session pointed at it.
// Nothing of the user's settings, characters or Ollama is touched.
struct FakeSession {
    FakeSession();
    ~FakeSession();
    QString dataFolder() const;

    QTemporaryDir* folder;
    FakeOllama* ollama;
};
