#include <QApplication>
#include <QDir>
#include <QFontDatabase>
#include <QIcon>
#include <QSettings>
#include <QStyleFactory>

#include "mainwindow.h"

#ifndef NOVAPAD_VERSION
#define NOVAPAD_VERSION "1.0.0"
#endif

static void loadEmbeddedFonts()
{
    const QList<QString> fonts = {
        QStringLiteral(":/fonts/FiraSans-Regular.ttf"),
        QStringLiteral(":/fonts/FiraSans-Italic.ttf"),
        QStringLiteral(":/fonts/FiraSans-Medium.ttf"),
        QStringLiteral(":/fonts/FiraSans-SemiBold.ttf"),
        QStringLiteral(":/fonts/FiraSans-Bold.ttf"),
        QStringLiteral(":/fonts/JetBrainsMono-Regular.ttf"),
        QStringLiteral(":/fonts/JetBrainsMono-Italic.ttf"),
        QStringLiteral(":/fonts/JetBrainsMono-Bold.ttf"),
        QStringLiteral(":/fonts/JetBrainsMono-BoldItalic.ttf"),
    };
    for (const QString &path : fonts)
        QFontDatabase::addApplicationFont(path);
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("NovaPad"));
    QApplication::setOrganizationName(QStringLiteral("SiktirStudio"));
    QApplication::setApplicationVersion(QString::fromLatin1(NOVAPAD_VERSION));
    QApplication::setWindowIcon(QIcon(QStringLiteral(":/appicon.png")));
    QApplication::setStyle(QStyleFactory::create(QStringLiteral("Fusion")));

    loadEmbeddedFonts();

    QFont uiFont(QStringLiteral("Fira Sans"));
    uiFont.setPointSize(10);
    QApplication::setFont(uiFont);

    QStringList args = app.arguments();

    // Developer / CI helpers -------------------------------------------------
    if (args.removeOne(QStringLiteral("--selftest")))
        return MainWindow::runSelfTest();
    if (args.removeOne(QStringLiteral("--stress"))) {
        MainWindow stressWin(nullptr);
        return stressWin.stressTest() ? 0 : 1;
    }

    QString shotDir;
    const int shotIdx = args.indexOf(QStringLiteral("--screenshot"));
    if (shotIdx >= 0 && shotIdx + 1 < args.size()) {
        shotDir = args.value(shotIdx + 1);
        args.removeAt(shotIdx);
        args.removeAt(shotIdx);
    }

    const QStringList files = args.mid(1);

    MainWindow win(nullptr);
    if (!shotDir.isEmpty()) {
        win.show();
        return win.takeScreenshots(shotDir, QStringLiteral("dark")) ? 0 : 1;
    }

    win.show();
    if (!files.isEmpty())
        win.openPaths(files);
    return app.exec();
}
