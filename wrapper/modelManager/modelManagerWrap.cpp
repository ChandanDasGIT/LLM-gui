#include "wrapper/modelManager/modelManagerWrap.h"

#include "wrapper/common/appSession/appSessionWrap.h"
#include "wrapper/common/ollamaClient/ollamaClientWrap.h"

#include "modelManager/sha256.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QNetworkReply>
#include <QProcess>
#include <QtConcurrent/QtConcurrentRun>

namespace {

QString gigabytes(long long bytes)
{
    return QString::number(double(bytes) / 1e9, 'f', 1) + QStringLiteral(" GB");
}

QString readText(const QString& path)
{
    QFile file(path);
    return file.open(QIODevice::ReadOnly) ? QString::fromUtf8(file.readAll()) : QString();
}

std::string_view view(const QByteArray& bytes)
{
    return std::string_view(bytes.constData(), std::size_t(bytes.size()));
}

} // namespace

// ---------------------------------------------------------------- ModelListModel

int ModelListModel::rowCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : int(rows_.size());
}

QVariant ModelListModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() >= rowCount()) return {};
    const Row& r = rows_[std::size_t(index.row())];
    switch (role) {
    case NameRole: return r.name;
    case NoteRole: return r.note;
    case DetailsRole: return r.details;
    case BadgeRole: return r.badge;
    case BadgeLevelRole: return r.badgeLevel;
    case InstalledRole: return r.installed;
    case CurrentRole: return r.current;
    default: return {};
    }
}

QHash<int, QByteArray> ModelListModel::roleNames() const
{
    return {
        {NameRole, "name"}, {NoteRole, "note"}, {DetailsRole, "details"}, {BadgeRole, "badge"},
        {BadgeLevelRole, "badgeLevel"}, {InstalledRole, "installed"}, {CurrentRole, "current"},
    };
}

void ModelListModel::setRows(std::vector<Row> rows)
{
    beginResetModel();
    rows_ = std::move(rows);
    endResetModel();
}

// ---------------------------------------------------------------- set-up and lists

ModelManagerWrap::ModelManagerWrap(QObject* parent)
    : QObject(parent), client_(new OllamaClientWrap(this)), recommended_(this), installed_(this)
{
    std::string error;
    if (auto c = catalogue::parse(view(readText(QStringLiteral(":/resources/catalogue.json")).toUtf8()), &error)) catalogue_ = *c;
    else setMessage(QStringLiteral("The recommended list could not be read: %1").arg(QString::fromStdString(error)));

    auto* session = AppSessionWrap::instance();
    connect(session, &AppSessionWrap::modelsChanged, this, &ModelManagerWrap::refresh);
    connect(session, &AppSessionWrap::currentModelChanged, this, &ModelManagerWrap::refresh);
    connect(session, &AppSessionWrap::connectionChanged, this, [this] {
        if (AppSessionWrap::instance()->connection() == AppSessionWrap::Connected) refresh();
    });
    probeHardware();
    rebuildRecommended();
    refresh();
}

ModelManagerWrap::~ModelManagerWrap()
{
    if (hashCancel_) hashCancel_->store(true);
    hashing_.waitForFinished();
    if (pullReply_) pullReply_->abort();
    if (importReply_) importReply_->abort();
}

void ModelManagerWrap::probeHardware()
{
    hardwareprobe::parseMeminfo(readText(QStringLiteral("/proc/meminfo")).toStdString(), hardware_);
    // AMD: sysfs; NVIDIA (checked after, it wins when present): nvidia-smi.
    const QDir drm(QStringLiteral("/sys/class/drm"));
    for (const QString& card : drm.entryList({QStringLiteral("card?"), QStringLiteral("card??")}, QDir::Dirs | QDir::System)) {
        const QString text = readText(drm.filePath(card + QStringLiteral("/device/mem_info_vram_total")));
        if (!text.isEmpty()) hardwareprobe::parseAmdVram(text.toStdString(), hardware_);
    }
    hardwareKnown_ = true;
    auto* smi = new QProcess(this);
    connect(smi, &QProcess::finished, this, [this, smi](int exitCode, QProcess::ExitStatus status) {
        if (status == QProcess::NormalExit && exitCode == 0) {
            Hardware nvidia = hardware_;
            nvidia.gpuName.clear();
            nvidia.vramBytes = 0;
            if (hardwareprobe::parseNvidiaSmi(QString::fromUtf8(smi->readAllStandardOutput()).toStdString(), nvidia))
                hardware_ = nvidia;
        }
        smi->deleteLater();
        emit hardwareChanged();
        rebuildRecommended();
    });
    connect(smi, &QProcess::errorOccurred, this, [smi](QProcess::ProcessError error) {
        if (error == QProcess::FailedToStart) smi->deleteLater();   // no NVIDIA driver tools
    });
    smi->start(QStringLiteral("nvidia-smi"),
               {QStringLiteral("--query-gpu=name,memory.total"), QStringLiteral("--format=csv,noheader,nounits")});
}

QString ModelManagerWrap::hardwareText() const
{
    return QString::fromStdString(hardwareprobe::summary(hardware_));
}

QStringList ModelManagerWrap::categories() const
{
    QStringList out;
    for (const auto& c : catalogue_.categories) out << QString::fromStdString(c);
    return out;
}

void ModelManagerWrap::setCategory(const QString& category)
{
    if (category == category_) return;
    category_ = category;
    emit filterChanged();
    rebuildRecommended();
}

void ModelManagerWrap::setShowAll(bool showAll)
{
    if (showAll == showAll_) return;
    showAll_ = showAll;
    emit filterChanged();
    rebuildRecommended();
}

QString ModelManagerWrap::snapshotText() const
{
    return QStringLiteral("Fit is an estimate from download size and memory, not a speed promise. List from ollama.com, %1.")
        .arg(QString::fromStdString(catalogue_.snapshotDate));
}

void ModelManagerWrap::rebuildRecommended()
{
    const QString current = AppSessionWrap::instance()->currentModel();
    std::vector<ModelListModel::Row> rows;
    for (const auto& e : catalogue::filter(catalogue_, category_.toStdString(), hardware_, showAll_)) {
        ModelListModel::Row row;
        row.name = QString::fromStdString(e.tag);
        row.note = QString::fromStdString(e.note);
        row.details = QString::fromStdString(e.sizeText) + QStringLiteral(" · licence: ")
                      + (e.licence.empty() ? QStringLiteral("not shown") : QString::fromStdString(e.licence));
        const auto fit = catalogue::rateFit(e.sizeBytes, hardware_);
        row.badge = QString::fromStdString(catalogue::fitText(fit));
        row.badgeLevel = fit == catalogue::Fit::FitsGpu ? 0 : fit == catalogue::Fit::TooBig ? 2 : 1;
        for (const auto& m : installedModels_) {
            const QString name = QString::fromStdString(m.name);
            if (name == row.name || name == row.name + QStringLiteral(":latest")) row.installed = true;
        }
        row.current = row.name == current;
        rows.push_back(std::move(row));
    }
    recommended_.setRows(std::move(rows));
}

void ModelManagerWrap::refresh()
{
    client_->get(QStringLiteral("/api/tags"), [this](const OllamaClientWrap::Result& r) {
        const auto tags = r.ok() ? ollama::parseTags(view(r.body)) : std::nullopt;
        if (!tags) {
            installedSummary_ = OllamaClientWrap::describe(r);
            emit installedChanged();
            return;
        }
        installedModels_ = *tags;
        const QString current = AppSessionWrap::instance()->currentModel();
        long long total = 0;
        std::vector<ModelListModel::Row> rows;
        for (const auto& m : installedModels_) {
            ModelListModel::Row row;
            row.name = QString::fromStdString(m.name);
            row.note = QString::fromStdString(m.family);
            QStringList parts{gigabytes(m.sizeBytes)};
            if (!m.parameterSize.empty()) parts << QString::fromStdString(m.parameterSize);
            if (!m.quantization.empty()) parts << QString::fromStdString(m.quantization);
            row.details = parts.join(QStringLiteral(" · "));
            row.installed = true;
            row.current = row.name == current;
            total += m.sizeBytes;
            rows.push_back(std::move(row));
        }
        installed_.setRows(std::move(rows));
        installedSummary_ = installedModels_.empty()
                                ? QStringLiteral("No models installed yet. Pick one under Recommended.")
                                : QStringLiteral("%1 model%2 · %3 on disk")
                                      .arg(installedModels_.size())
                                      .arg(installedModels_.size() == 1 ? "" : "s")
                                      .arg(gigabytes(total));
        emit installedChanged();
        rebuildRecommended();
    }, 5000);
}

void ModelManagerWrap::setMessage(const QString& text)
{
    if (text == message_) return;
    message_ = text;
    emit messageChanged();
}

// ---------------------------------------------------------------- install, use, remove

void ModelManagerWrap::setInstall(const QString& name, double progress, const QString& text)
{
    installingName_ = name;
    installProgress_ = progress;
    installText_ = text;
    emit installChanged();
}

void ModelManagerWrap::install(const QString& tag)
{
    const QString name = tag.trimmed();
    if (name.isEmpty()) return;
    if (pullReply_) {
        setMessage(QStringLiteral("Wait for %1 to finish, or cancel it.").arg(installingName_));
        return;
    }
    setMessage(QString());
    pullTotals_ = {};
    pullError_.clear();
    setInstall(name, 0.0, QStringLiteral("Starting…"));
    pullReply_ = client_->stream(QStringLiteral("/api/pull"), QByteArray::fromStdString(ollama::pullRequest(name.toStdString())),
                                 [this](const std::string& line) { onPullLine(line); },
                                 [this, name](const OllamaClientWrap::Result& r) {
        const bool canceled = r.canceled();
        QString error = pullError_;
        if (error.isEmpty() && !canceled && !r.ok()) error = OllamaClientWrap::describe(r);
        const bool success = !canceled && error.isEmpty() && installText_ == QStringLiteral("Done");
        setInstall(QString(), 0.0, QString());
        if (canceled) setMessage(QStringLiteral("Install of %1 canceled.").arg(name));
        else if (!error.isEmpty()) setMessage(QStringLiteral("Could not install %1: %2").arg(name, error));
        else if (!success) setMessage(QStringLiteral("Ollama stopped before %1 was complete.").arg(name));
        else setMessage(QStringLiteral("Installed %1.").arg(name));
        if (success) AppSessionWrap::instance()->notifyModelsChanged();
    });
}

void ModelManagerWrap::onPullLine(const std::string& line)
{
    const auto p = ollama::parseProgressLine(line);
    if (!p) return;
    if (!p->error.empty()) {
        pullError_ = QString::fromStdString(p->error);
        return;
    }
    if (p->success()) {
        setInstall(installingName_, 1.0, QStringLiteral("Done"));
        return;
    }
    pullTotals_.add(*p);
    QString text = QString::fromStdString(p->status);
    if (p->total > 0) {
        text = QStringLiteral("%1 / %2 · %3%")
                   .arg(gigabytes(pullTotals_.completed()), gigabytes(pullTotals_.total()))
                   .arg(int(pullTotals_.fraction() * 100.0));
    }
    setInstall(installingName_, pullTotals_.fraction(), text);
}

void ModelManagerWrap::cancelInstall()
{
    if (pullReply_) pullReply_->abort();
}

void ModelManagerWrap::use(const QString& name)
{
    AppSessionWrap::instance()->setCurrentModel(name);
    setMessage(QStringLiteral("Using %1.").arg(name));
}

void ModelManagerWrap::remove(const QString& name)
{
    client_->send("DELETE", QStringLiteral("/api/delete"), QByteArray::fromStdString(ollama::modelRequest(name.toStdString())),
                  [this, name](const OllamaClientWrap::Result& r) {
        if (!r.ok()) {
            setMessage(QStringLiteral("Could not remove %1: %2").arg(name, OllamaClientWrap::describe(r)));
            return;
        }
        setMessage(QStringLiteral("Removed %1.").arg(name));
        AppSessionWrap::instance()->notifyModelsChanged();
    });
}

// ---------------------------------------------------------------- .gguf import

QString ModelManagerWrap::importFileText() const
{
    if (importPath_.isEmpty()) return QStringLiteral("No file chosen");
    return QDir::toNativeSeparators(importPath_) + QStringLiteral(" · ") + gigabytes(importSize_);
}

void ModelManagerWrap::setImport(bool running, double progress, const QString& text)
{
    importing_ = running;
    importProgress_ = progress;
    importText_ = text;
    emit importChanged();
}

void ModelManagerWrap::chooseImportFile(const QUrl& file)
{
    if (importing_) return;
    const QString path = file.isLocalFile() ? file.toLocalFile() : file.toString();
    const QFileInfo info(path);
    if (!info.isFile() || info.suffix().toLower() != QStringLiteral("gguf")) {
        setMessage(QStringLiteral("Not a .gguf file: %1").arg(info.fileName()));
        return;
    }
    importPath_ = info.absoluteFilePath();
    importSize_ = info.size();
    setMessage(QString());
    emit importChanged();
}

void ModelManagerWrap::importGguf(const QString& name)
{
    const QString model = name.trimmed();
    if (importing_) return;
    if (importPath_.isEmpty()) {
        setMessage(QStringLiteral("Choose a .gguf file first."));
        return;
    }
    if (model.isEmpty()) {
        setMessage(QStringLiteral("Give the model a name."));
        return;
    }
    setMessage(QString());
    importError_.clear();
    setImport(true, 0.0, QStringLiteral("Hashing · 0%"));

    hashCancel_ = std::make_shared<std::atomic<bool>>(false);
    const auto cancel = hashCancel_;
    const QString path = importPath_;
    hashing_ = QtConcurrent::run([this, cancel, path, model] {
        QFile file(path);
        QString digest;
        QString error;
        if (!file.open(QIODevice::ReadOnly)) {
            error = QStringLiteral("Could not read %1").arg(path);
        } else {
            Sha256 hash;
            const qint64 size = file.size();
            qint64 done = 0;
            int lastPercent = -1;
            QByteArray chunk(4 * 1024 * 1024, Qt::Uninitialized);
            while (!cancel->load()) {
                const qint64 n = file.read(chunk.data(), chunk.size());
                if (n <= 0) break;
                hash.update(chunk.constData(), std::size_t(n));
                done += n;
                const int percent = size > 0 ? int(done * 100 / size) : 100;
                if (percent != lastPercent) {
                    lastPercent = percent;
                    QMetaObject::invokeMethod(this, [this, percent] {
                        if (importing_) setImport(true, percent / 100.0, QStringLiteral("Hashing · %1%").arg(percent));
                    }, Qt::QueuedConnection);
                }
            }
            if (!cancel->load()) {
                if (done != size) error = QStringLiteral("Could not read all of %1").arg(path);
                else digest = QString::fromStdString(hash.finishHex());
            }
        }
        QMetaObject::invokeMethod(this, [this, cancel, digest, error, model] {
            if (cancel->load()) {
                setImport(false, 0.0, QString());
                setMessage(QStringLiteral("Import canceled."));
            } else if (!error.isEmpty()) {
                setImport(false, 0.0, QString());
                setMessage(error);
            } else {
                uploadBlob(model, digest);
            }
        }, Qt::QueuedConnection);
    });
}

void ModelManagerWrap::uploadBlob(const QString& name, const QString& digest)
{
    const QString blobPath = QStringLiteral("/api/blobs/sha256:") + digest;
    setImport(true, 0.0, QStringLiteral("Checking Ollama…"));
    importReply_ = client_->send("HEAD", blobPath, QByteArray(), [this, name, digest, blobPath](const OllamaClientWrap::Result& r) {
        if (r.canceled()) {
            setImport(false, 0.0, QString());
            setMessage(QStringLiteral("Import canceled."));
            return;
        }
        if (r.ok()) {                          // Ollama has this file already
            createModel(name, digest);
            return;
        }
        if (r.status != 404) {
            setImport(false, 0.0, QString());
            setMessage(QStringLiteral("Import failed: %1").arg(OllamaClientWrap::describe(r)));
            return;
        }
        auto* file = new QFile(importPath_);
        if (!file->open(QIODevice::ReadOnly)) {
            delete file;
            setImport(false, 0.0, QString());
            setMessage(QStringLiteral("Could not read %1").arg(importPath_));
            return;
        }
        setImport(true, 0.0, QStringLiteral("Uploading · 0%"));
        importReply_ = client_->upload(blobPath, file, file->size(), [this, name, digest](const OllamaClientWrap::Result& up) {
            if (up.canceled()) {
                setImport(false, 0.0, QString());
                setMessage(QStringLiteral("Import canceled."));
            } else if (!up.ok()) {
                setImport(false, 0.0, QString());
                setMessage(QStringLiteral("Upload failed: %1").arg(OllamaClientWrap::describe(up)));
            } else {
                createModel(name, digest);
            }
        });
        file->setParent(importReply_);
        connect(importReply_, &QNetworkReply::uploadProgress, this, [this](qint64 sent, qint64 total) {
            if (total <= 0) return;
            const int percent = int(sent * 100 / total);
            setImport(true, double(sent) / double(total), QStringLiteral("Uploading · %1%").arg(percent));
        });
    });
}

void ModelManagerWrap::createModel(const QString& name, const QString& digest)
{
    setImport(true, 1.0, QStringLiteral("Creating %1…").arg(name));
    importError_.clear();
    const std::string fileName = QFileInfo(importPath_).fileName().toStdString();
    importReply_ = client_->stream(
        QStringLiteral("/api/create"),
        QByteArray::fromStdString(ollama::createRequest(name.toStdString(), fileName, digest.toStdString())),
        [this](const std::string& line) {
            const auto p = ollama::parseProgressLine(line);
            if (!p) return;
            if (!p->error.empty()) importError_ = QString::fromStdString(p->error);
            else if (!p->status.empty()) setImport(true, 1.0, QString::fromStdString(p->status));
        },
        [this, name](const OllamaClientWrap::Result& r) {
            const bool success = !r.canceled() && r.ok() && importError_.isEmpty() && importText_ == QStringLiteral("success");
            QString error = importError_;
            if (error.isEmpty() && !r.ok() && !r.canceled()) error = OllamaClientWrap::describe(r);
            setImport(false, 0.0, QString());
            if (r.canceled()) setMessage(QStringLiteral("Import canceled."));
            else if (!error.isEmpty()) setMessage(QStringLiteral("Could not create %1: %2").arg(name, error));
            else if (!success) setMessage(QStringLiteral("Ollama stopped before %1 was created.").arg(name));
            else {
                setMessage(QStringLiteral("Imported as %1.").arg(name));
                AppSessionWrap::instance()->notifyModelsChanged();
            }
        });
}

void ModelManagerWrap::cancelImport()
{
    if (hashCancel_) hashCancel_->store(true);
    if (importReply_) importReply_->abort();
}
