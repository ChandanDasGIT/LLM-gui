#include <QCoreApplication>
#include <QGuiApplication>
#include <QIcon>
#include <QQmlApplicationEngine>

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    // Settings go to ~/.config/LLM-gui/LLM-gui.conf.
    QCoreApplication::setOrganizationName("LLM-gui");
    QCoreApplication::setApplicationName("LLM-gui");
    QCoreApplication::setApplicationVersion(QStringLiteral(APP_VERSION));
    // The desktop matches the window to its launcher entry and icon by this id.
    QGuiApplication::setDesktopFileName("io.github.chandandasgit.LLMgui");
    QGuiApplication::setWindowIcon(QIcon(QStringLiteral(":/icons/icon.svg")));

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
