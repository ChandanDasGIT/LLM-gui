#include <QCoreApplication>
#include <QGuiApplication>
#include <QQmlApplicationEngine>

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    // Settings go to ~/.config/LLM-gui/LLM-gui.conf.
    QCoreApplication::setOrganizationName("LLM-gui");
    QCoreApplication::setApplicationName("LLM-gui");

    QQmlApplicationEngine engine;
    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);
    engine.loadFromModule("LLMgui", "Main");

    return QGuiApplication::exec();
}
