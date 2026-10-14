#include <QApplication>
#include <QDir>
#include <QFontDatabase>
#include <QIcon>
#include <QSettings>
#include <QSslSocket>
#include <QTemporaryDir>
#include <memory>
#include <QStyleFactory>
#include <cstdio>

#include "mainwindow.h"

#ifndef NOVAPAD_VERSION
#define NOVAPAD_VERSION "1.1.0"
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
    for (int i = 1; i < argc; ++i) {
        const QString arg = QString::fromLocal8Bit(argv[i]);
        if (arg == QStringLiteral("--version") || arg == QStringLiteral("-v")) {
            std::printf("NovaPad %s (Qt %s)\n", NOVAPAD_VERSION, qVersion());
            return 0;
        }
    }

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

    // Test preferences and sessions must never touch the user's real settings.
    std::unique_ptr<QTemporaryDir> testSettings;
    if (args.contains(QStringLiteral("--selftest")) || args.contains(QStringLiteral("--stress"))) {
        testSettings = std::make_unique<QTemporaryDir>();
        if (!testSettings->isValid())
            return 1;
        QSettings::setDefaultFormat(QSettings::IniFormat);
        QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, testSettings->path());
        QSettings::setPath(QSettings::IniFormat, QSettings::SystemScope, testSettings->path());
    }

    // Developer / CI helpers -------------------------------------------------
    if (args.removeOne(QStringLiteral("--tls-check"))) {
        const bool available = QSslSocket::supportsSsl();
        qInfo("TLS %s (backend: %s)", available ? "OK" : "UNAVAILABLE",
              qPrintable(QSslSocket::activeBackend()));
        return available ? 0 : 1;
    }
    if (args.removeOne(QStringLiteral("--selftest")))
        return MainWindow::runSelfTest();
    if (args.removeOne(QStringLiteral("--stress"))) {
        MainWindow stressWindow(nullptr);
        return stressWindow.stressTest() ? 0 : 1;
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

    win.showMaximized();
    if (!files.isEmpty())
        win.openPaths(files);
    return app.exec();
}
