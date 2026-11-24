#pragma once

#include "modelManager/catalogue.h"
#include "modelManager/hardwareprobe.h"
#include "common/ollamaapi.h"

#include <QAbstractListModel>
#include <QFuture>
#include <QObject>
#include <QPointer>
#include <QStringList>
#include <QUrl>
#include <QtQml/qqmlregistration.h>

#include <atomic>
#include <memory>
#include <vector>

class OllamaClientWrap;
class QNetworkReply;

// The rows of the Recommended and Installed tabs.
class ModelListModel : public QAbstractListModel {
    Q_OBJECT
    QML_ANONYMOUS

public:
    struct Row {
        QString name;
        QString note;
        QString details;
        QString badge;
        int badgeLevel = -1;           // 0 good, 1 slower, 2 too big; -1 no badge
        bool installed = false;
        bool current = false;
    };
    enum Roles { NameRole = Qt::UserRole + 1, NoteRole, DetailsRole, BadgeRole, BadgeLevelRole, InstalledRole, CurrentRole };

    using QAbstractListModel::QAbstractListModel;
    int rowCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;
    void setRows(std::vector<Row> rows);
    int count() const { return int(rows_.size()); }

private:
    std::vector<Row> rows_;
};

// The model manager for QML: what this machine has, the recommended models with their fit,
// the installed ones, installing (with progress and cancel), removing, using, and importing a
// local .gguf file (hash, upload, create).
class ModelManagerWrap : public QObject {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QString hardwareText READ hardwareText NOTIFY hardwareChanged)
    Q_PROPERTY(QStringList categories READ categories CONSTANT)
    Q_PROPERTY(QString category READ category WRITE setCategory NOTIFY filterChanged)
    Q_PROPERTY(bool showAll READ showAll WRITE setShowAll NOTIFY filterChanged)
    Q_PROPERTY(QString snapshotText READ snapshotText CONSTANT)
    Q_PROPERTY(ModelListModel* recommended READ recommended CONSTANT)
    Q_PROPERTY(ModelListModel* installed READ installed CONSTANT)
    Q_PROPERTY(QString installedSummary READ installedSummary NOTIFY installedChanged)
    Q_PROPERTY(QString installingName READ installingName NOTIFY installChanged)
    Q_PROPERTY(double installProgress READ installProgress NOTIFY installChanged)
    Q_PROPERTY(QString installText READ installText NOTIFY installChanged)
    Q_PROPERTY(QString importFileText READ importFileText NOTIFY importChanged)
    Q_PROPERTY(bool importing READ importing NOTIFY importChanged)
    Q_PROPERTY(double importProgress READ importProgress NOTIFY importChanged)
    Q_PROPERTY(QString importText READ importText NOTIFY importChanged)
    Q_PROPERTY(QString message READ message NOTIFY messageChanged)

public:
    explicit ModelManagerWrap(QObject* parent = nullptr);
    ~ModelManagerWrap() override;

    QString hardwareText() const;
    QStringList categories() const;
    QString category() const { return category_; }
    void setCategory(const QString& category);
    bool showAll() const { return showAll_; }
    void setShowAll(bool showAll);
    QString snapshotText() const;
    ModelListModel* recommended() { return &recommended_; }
    ModelListModel* installed() { return &installed_; }
    QString installedSummary() const { return installedSummary_; }
    QString installingName() const { return installingName_; }
    double installProgress() const { return installProgress_; }
    QString installText() const { return installText_; }
    QString importFileText() const;
    bool importing() const { return importing_; }
    double importProgress() const { return importProgress_; }
    QString importText() const { return importText_; }
    QString message() const { return message_; }

    Q_INVOKABLE void refresh();
    Q_INVOKABLE void install(const QString& tag);
    Q_INVOKABLE void cancelInstall();
    Q_INVOKABLE void use(const QString& name);
    Q_INVOKABLE void remove(const QString& name);
    Q_INVOKABLE void chooseImportFile(const QUrl& file);
    Q_INVOKABLE void importGguf(const QString& name);
    Q_INVOKABLE void cancelImport();

signals:
    void hardwareChanged();
    void filterChanged();
    void installedChanged();
    void installChanged();
    void importChanged();
    void messageChanged();

private:
    void probeHardware();
    void rebuildRecommended();
    void setMessage(const QString& text);
    void setInstall(const QString& name, double progress, const QString& text);
    void setImport(bool running, double progress, const QString& text);
    void onPullLine(const std::string& line);
    void uploadBlob(const QString& name, const QString& digest);
    void createModel(const QString& name, const QString& digest);

    OllamaClientWrap* client_ = nullptr;
    Catalogue catalogue_;
    Hardware hardware_;
    bool hardwareKnown_ = false;
    QString category_ = QStringLiteral("General");
    bool showAll_ = false;
    ModelListModel recommended_;
    ModelListModel installed_;
    std::vector<ollama::InstalledModel> installedModels_;
    QString installedSummary_;

    QPointer<QNetworkReply> pullReply_;
    ollama::PullTotals pullTotals_;
    QString pullError_;
    QString installingName_;
    double installProgress_ = 0.0;
    QString installText_;

    QString importPath_;
    qint64 importSize_ = 0;
    bool importing_ = false;
    double importProgress_ = 0.0;
    QString importText_;
    QPointer<QNetworkReply> importReply_;
    std::shared_ptr<std::atomic<bool>> hashCancel_;
    QFuture<void> hashing_;
    QString importError_;

    QString message_;
};
