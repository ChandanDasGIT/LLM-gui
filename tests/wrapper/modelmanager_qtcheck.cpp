#include "testkit.h"
#include "fakeollama.h"

#include "wrapper/common/appSession/appSessionWrap.h"
#include "wrapper/modelManager/modelManagerWrap.h"

#include "common/json.h"
#include "modelManager/sha256.h"

#include <QFile>
#include <QSignalSpy>
#include <QtTest>

namespace {

const QByteArray tags = R"({"models":[{"name":"qwen3:8b","size":5200000000,"details":{"family":"qwen3","parameter_size":"8.2B","quantization_level":"Q4_K_M"}}]})";

QVariant role(ModelListModel* m, int row, const char* name)
{
    return m->data(m->index(row), m->roleNames().key(name));
}

int rowNamed(ModelListModel* m, const QString& name)
{
    for (int i = 0; i < m->rowCount(); ++i)
        if (role(m, i, "name") == name) return i;
    return -1;
}

} // namespace

TEST(qtmodelmanager, hardware_recommended_and_installed)
{
    FakeSession s;
    s.ollama->onJson("GET", "/api/tags", tags);
    AppSessionWrap::instance()->setCurrentModel("qwen3:8b");
    ModelManagerWrap manager;
    CHECK(manager.categories().size() == 6 && manager.category() == "General");
    CHECK(manager.hardwareText().contains("GB RAM"));
    CHECK(manager.snapshotText().contains("2026-11-24"));
    CHECK(QTest::qWaitFor([&] { return manager.installed()->rowCount() == 1; }, 3000));
    CHECK(manager.installedSummary() == "1 model · 5.2 GB on disk");
    CHECK(role(manager.installed(), 0, "details") == "5.2 GB · 8.2B · Q4_K_M" && role(manager.installed(), 0, "current").toBool());

    const int qwen = rowNamed(manager.recommended(), "qwen3:8b");
    CHECK(qwen >= 0 && role(manager.recommended(), qwen, "installed").toBool());
    CHECK(!role(manager.recommended(), qwen, "badge").toString().isEmpty());
    const int general = manager.recommended()->rowCount();
    manager.setCategory("Vision");
    CHECK(manager.recommended()->rowCount() > 0 && manager.recommended()->rowCount() != general);
    const int vision = manager.recommended()->rowCount();
    manager.setShowAll(true);
    CHECK(manager.recommended()->rowCount() >= vision);
}

TEST(qtmodelmanager, install_reports_progress_and_success)
{
    FakeSession s;
    s.ollama->onJson("GET", "/api/tags", tags);
    FakeOllama::Response pull;
    pull.lines = {
        R"({"status":"pulling manifest"})",
        R"({"status":"pulling aa","digest":"sha256:aa","total":4000000000,"completed":1000000000})",
        R"({"status":"pulling aa","digest":"sha256:aa","total":4000000000,"completed":4000000000})",
        R"({"status":"verifying sha256 digest"})",
        R"({"status":"success"})",
    };
    s.ollama->on("POST", "/api/pull", pull);
    ModelManagerWrap manager;
    QStringList texts;
    QObject::connect(&manager, &ModelManagerWrap::installChanged, [&] { texts << manager.installText(); });
    QSignalSpy models(AppSessionWrap::instance(), &AppSessionWrap::modelsChanged);
    manager.install("  gemma3:4b ");
    CHECK(manager.installingName() == "gemma3:4b");
    CHECK(QTest::qWaitFor([&] { return manager.installingName().isEmpty(); }, 3000));
    CHECK(texts.contains("1.0 GB / 4.0 GB · 25%") && texts.contains("pulling manifest"));
    CHECK(manager.message() == "Installed gemma3:4b." && models.count() == 1);
    const auto body = json::parse(s.ollama->requestsTo("/api/pull").last().body.toStdString());
    CHECK(body && (*body)["model"].asString() == "gemma3:4b");
}

TEST(qtmodelmanager, install_errors_and_cancel)
{
    FakeSession s;
    s.ollama->onJson("GET", "/api/tags", tags);
    FakeOllama::Response bad;
    bad.lines = {R"({"error":"pull model manifest: file does not exist"})"};
    s.ollama->on("POST", "/api/pull", bad);
    ModelManagerWrap manager;
    manager.install("nope:1b");
    CHECK(QTest::qWaitFor([&] { return manager.installingName().isEmpty(); }, 3000));
    CHECK(manager.message() == "Could not install nope:1b: pull model manifest: file does not exist");

    FakeOllama::Response held;
    held.lines = {R"({"status":"pulling aa","digest":"sha256:aa","total":100,"completed":10})"};
    held.holdOpen = true;
    s.ollama->on("POST", "/api/pull", held);
    manager.install("big:70b");
    CHECK(QTest::qWaitFor([&] { return manager.installProgress() > 0.0; }, 3000));
    manager.install("other:1b");                           // one at a time
    CHECK(manager.message() == "Wait for big:70b to finish, or cancel it.");
    manager.cancelInstall();
    CHECK(QTest::qWaitFor([&] { return manager.installingName().isEmpty(); }, 3000));
    CHECK(manager.message() == "Install of big:70b canceled.");
    CHECK(QTest::qWaitFor([&] { return s.ollama->closedByClient == 1; }, 3000));
}

TEST(qtmodelmanager, use_and_remove)
{
    FakeSession s;
    s.ollama->onJson("GET", "/api/tags", tags);
    s.ollama->onJson("DELETE", "/api/delete", "");
    ModelManagerWrap manager;
    manager.use("qwen3:8b");
    CHECK(AppSessionWrap::instance()->currentModel() == "qwen3:8b");
    QSignalSpy models(AppSessionWrap::instance(), &AppSessionWrap::modelsChanged);
    manager.remove("qwen3:8b");
    CHECK(QTest::qWaitFor([&] { return manager.message() == "Removed qwen3:8b."; }, 3000));
    CHECK(models.count() == 1);
    CHECK(s.ollama->requestsTo("/api/delete").last().body == R"({"model":"qwen3:8b"})");
    s.ollama->onJson("DELETE", "/api/delete", R"({"error":"model 'x' not found"})", 404);
    manager.remove("x");
    CHECK(QTest::qWaitFor([&] { return manager.message() == "Could not remove x: model 'x' not found"; }, 3000));
}

TEST(qtmodelmanager, gguf_import_hashes_uploads_and_creates)
{
    FakeSession s;
    s.ollama->onJson("GET", "/api/tags", tags);
    QByteArray bytes;
    for (int i = 0; i < 300000; ++i) bytes += char(i * 31);
    const QString path = s.folder->path() + "/tiny-model.gguf";
    { QFile f(path); CHECK(f.open(QIODevice::WriteOnly)); f.write(bytes); }
    Sha256 hash;
    hash.update(bytes.constData(), std::size_t(bytes.size()));
    const QByteArray digest = QByteArray::fromStdString(hash.finishHex());
    const QByteArray blob = "/api/blobs/sha256:" + digest;
    s.ollama->onJson("HEAD", blob, "", 404);
    s.ollama->onJson("POST", blob, "", 201);
    FakeOllama::Response create;
    create.lines = {R"({"status":"parsing GGUF"})", R"({"status":"writing manifest"})", R"({"status":"success"})"};
    s.ollama->on("POST", "/api/create", create);

    ModelManagerWrap manager;
    manager.chooseImportFile(QUrl::fromLocalFile(s.folder->path() + "/notes.txt"));
    CHECK(manager.message().startsWith("Not a .gguf file"));
    manager.importGguf("mine");
    CHECK(manager.message() == "Choose a .gguf file first.");
    manager.chooseImportFile(QUrl::fromLocalFile(path));
    CHECK(manager.importFileText().endsWith("tiny-model.gguf · 0.0 GB"));
    manager.importGguf("  ");
    CHECK(manager.message() == "Give the model a name.");

    QSignalSpy models(AppSessionWrap::instance(), &AppSessionWrap::modelsChanged);
    manager.importGguf("mine");
    CHECK(manager.importing());
    CHECK(QTest::qWaitFor([&] { return !manager.importing(); }, 5000));
    CHECK(manager.message() == "Imported as mine.");
    CHECK(models.count() == 1);
    CHECK(s.ollama->requestsTo(blob).size() == 2);
    CHECK(s.ollama->requestsTo(blob)[1].body == bytes);
    const auto body = json::parse(s.ollama->requestsTo("/api/create").last().body.toStdString());
    CHECK(body && (*body)["model"].asString() == "mine");
    CHECK((*body)["files"]["tiny-model.gguf"].asString() == "sha256:" + digest.toStdString());

    s.ollama->onJson("HEAD", blob, "", 200);               // Ollama has the file: no second upload
    manager.importGguf("mine2");
    CHECK(QTest::qWaitFor([&] { return !manager.importing(); }, 5000));
    CHECK(manager.message() == "Imported as mine2." && s.ollama->requestsTo(blob).size() == 3);
}
