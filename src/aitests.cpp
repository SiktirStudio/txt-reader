#include "aitests.h"

#include "ai.h"
#include "aipanel.h"

#include <QComboBox>
#include <QEventLoop>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLineEdit>
#include <QNetworkReply>
#include <QPlainTextEdit>
#include <QRegularExpression>
#include <QSettings>
#include <QSslConfiguration>
#include <QSslSocket>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTimer>

namespace {
struct Exchange {
    QString text;
    QString error;
    QByteArray request;
    bool finished = false;
    int requests = 0;
};

Exchange exchange(Ai::Provider provider, const QByteArray &body, int status = 200, bool cancel = false)
{
    Exchange result;
    QTcpServer server;
    if (!server.listen(QHostAddress::LocalHost, 0)) {
        result.error = QStringLiteral("mock server could not listen");
        return result;
    }
    AiClient client;
    QEventLoop loop;
    QTimer deadline;
    deadline.setSingleShot(true);
    QObject::connect(&deadline, &QTimer::timeout, &loop, &QEventLoop::quit);
    QObject::connect(&client, &AiClient::completed, &loop, [&](const QString &text) {
        result.text = text;
        result.finished = true;
        loop.quit();
    });
    QObject::connect(&client, &AiClient::failed, &loop, [&](const QString &error) {
        result.error = error;
        result.finished = true;
        loop.quit();
    });
    QObject::connect(&server, &QTcpServer::newConnection, &loop, [&] {
        while (QTcpSocket *socket = server.nextPendingConnection()) {
            ++result.requests;
            QObject::connect(socket, &QTcpSocket::readyRead, socket, [&, socket] {
                result.request += socket->readAll();
                if (cancel || socket->property("answered").toBool())
                    return;
                const int split = result.request.indexOf("\r\n\r\n");
                if (split < 0)
                    return;
                const auto match = QRegularExpression(QStringLiteral("content-length: (\\d+)\\r\\n"),
                    QRegularExpression::CaseInsensitiveOption).match(QString::fromLatin1(result.request));
                if (!match.hasMatch() || result.request.size() - split - 4 < match.captured(1).toInt())
                    return;
                socket->setProperty("answered", true);
                QByteArray headers = QByteArrayLiteral("HTTP/1.1 ") + QByteArray::number(status)
                    + QByteArrayLiteral(" Test\r\nContent-Type: application/json\r\nConnection: close\r\n");
                if (status == 302)
                    headers += QByteArrayLiteral("Location: http://127.0.0.1:")
                        + QByteArray::number(server.serverPort()) + QByteArrayLiteral("/redirect-target\r\n");
                headers += QByteArrayLiteral("Content-Length: ") + QByteArray::number(body.size())
                    + QByteArrayLiteral("\r\n\r\n");
                socket->write(headers + body);
                socket->disconnectFromHost();
            });
        }
    });
    const QString base = QStringLiteral("http://127.0.0.1:%1/v1").arg(server.serverPort());
    client.send(provider, base, QStringLiteral("test-model"), QStringLiteral("test-key-not-real"),
                QStringLiteral("Test prompt"));
    if (cancel)
        QTimer::singleShot(50, &client, &AiClient::cancel);
    deadline.start(10000);
    loop.exec();
    return result;
}
}

void runAiSelfTests(const std::function<void(bool, const char *)> &check)
{
    using Ai::Provider;
    QString error;
    check(Ai::endpoint(Provider::OpenAICompatible, QStringLiteral("https://api.example.com/v1/"), &error)
              == QUrl(QStringLiteral("https://api.example.com/v1/chat/completions")) && error.isEmpty(),
          "AI: OpenAI endpoint");
    check(Ai::endpoint(Provider::Anthropic, QStringLiteral("https://api.example.com/v1"), &error)
              == QUrl(QStringLiteral("https://api.example.com/v1/messages")), "AI: Anthropic endpoint");
    check(Ai::endpoint(Provider::Anthropic, QStringLiteral("https://api.example.com/v1/messages"), &error)
              .path() == QStringLiteral("/v1/messages"), "AI: full endpoint is not duplicated");
    const QStringList rejected {
        QStringLiteral("http://api.example.com/v1"), QStringLiteral("http://192.168.1.2/v1"),
        QStringLiteral("http://127.0.0.1.example.com/v1"), QStringLiteral("ftp://localhost/v1"),
        QStringLiteral("/v1"), QStringLiteral("https://user:secret@example.com/v1"),
        QStringLiteral("https://example.com/v1?api_key=secret"), QStringLiteral("https://example.com/#secret"),
        QStringLiteral("http://localhost.example.com/v1"), QStringLiteral("https://")
    };
    for (const QString &url : rejected)
        check(Ai::endpoint(Provider::OpenAICompatible, url, &error).isEmpty() && !error.isEmpty(),
              "AI security: reject unsafe base URL");
    for (const QString &url : {QStringLiteral("http://localhost:8080/v1"),
                               QStringLiteral("http://127.0.0.1:8080/v1"),
                               QStringLiteral("http://[::1]:8080/v1")})
        check(!Ai::endpoint(Provider::OpenAICompatible, url, &error).isEmpty() && error.isEmpty(),
              "AI security: allow HTTP loopback");

    const auto openai = Ai::prepare(Provider::OpenAICompatible, QStringLiteral("https://example.com/v1"),
        QStringLiteral("test-model"), QStringLiteral("test-key-not-real"), QStringLiteral("Test prompt"));
    check(openai.error.isEmpty() && openai.request.rawHeader("Authorization") == "Bearer test-key-not-real"
              && !openai.request.hasRawHeader("x-api-key"), "AI: OpenAI authentication header");
    const QJsonObject openaiBody = QJsonDocument::fromJson(openai.body).object();
    check(openaiBody.value(QStringLiteral("model")).toString() == QStringLiteral("test-model")
              && openaiBody.value(QStringLiteral("messages")).toArray().first().toObject()
                     .value(QStringLiteral("content")).toString() == QStringLiteral("Test prompt")
              && !openai.body.contains("test-key-not-real"), "AI: request body excludes credential");
    check(openai.request.attribute(QNetworkRequest::RedirectPolicyAttribute).toInt() == QNetworkRequest::ManualRedirectPolicy,
          "AI security: redirects disabled");
    check(openai.request.sslConfiguration().peerVerifyMode() == QSslSocket::VerifyPeer
              && openai.request.sslConfiguration().protocol() == QSsl::TlsV1_2OrLater,
          "AI security: verified TLS 1.2 or newer");
    check(openai.request.transferTimeout() == 60000, "AI security: request timeout configured");
    check(openai.request.attribute(QNetworkRequest::CookieLoadControlAttribute).toInt() == QNetworkRequest::Manual
              && openai.request.attribute(QNetworkRequest::CookieSaveControlAttribute).toInt() == QNetworkRequest::Manual,
          "AI security: cookies disabled");
    const auto anthropic = Ai::prepare(Provider::Anthropic, QStringLiteral("https://example.com/v1"),
        QStringLiteral("test-model"), QStringLiteral("test-key-not-real"), QStringLiteral("Test prompt"));
    check(anthropic.request.rawHeader("x-api-key") == "test-key-not-real"
              && anthropic.request.rawHeader("anthropic-version") == "2023-06-01"
              && !anthropic.request.hasRawHeader("Authorization")
              && QJsonDocument::fromJson(anthropic.body).object().value(QStringLiteral("max_tokens")).toInt() == 4096,
          "AI: Anthropic headers and token budget");
    auto prepare = [](const QString &base, const QString &model, const QString &key, const QString &prompt) {
        return Ai::prepare(Provider::OpenAICompatible, base, model, key, prompt);
    };
    check(!prepare(QStringLiteral("https://example.com"), QStringLiteral("model"), QString(), QStringLiteral("hi")).error.isEmpty(),
          "AI security: remote API key required");
    check(prepare(QStringLiteral("http://127.0.0.1:8000"), QStringLiteral("model"), QString(), QStringLiteral("hi")).error.isEmpty(),
          "AI: local server may omit key");
    check(!prepare(QStringLiteral("https://example.com"), QStringLiteral("model"), QStringLiteral("key\r\nInjected: x"),
                   QStringLiteral("hi")).error.isEmpty(), "AI security: reject header injection");
    check(!prepare(QStringLiteral("https://example.com"), QString(), QStringLiteral("key"), QStringLiteral("hi")).error.isEmpty(),
          "AI: model required");
    check(!prepare(QStringLiteral("https://example.com"), QStringLiteral("model"), QStringLiteral("key"), QString()).error.isEmpty(),
          "AI: prompt required");
    check(!prepare(QStringLiteral("https://example.com"), QStringLiteral("model"), QStringLiteral("key"),
                   QString(Ai::MaxPromptBytes + 1, QLatin1Char('x'))).error.isEmpty(), "AI security: prompt size limit");

    const QByteArray openaiJson = R"({"choices":[{"message":{"content":"Hello <b>world</b>"}}]})";
    const QByteArray anthropicJson = R"({"content":[{"type":"text","text":"Hello "},{"type":"tool_use"},{"type":"text","text":"world"}]})";
    check(Ai::responseText(Provider::OpenAICompatible, openaiJson, &error) == QStringLiteral("Hello <b>world</b>")
              && error.isEmpty(), "AI: OpenAI plain text response");
    check(Ai::responseText(Provider::Anthropic, anthropicJson, &error) == QStringLiteral("Hello world")
              && error.isEmpty(), "AI: Anthropic text blocks");
    check(Ai::responseText(Provider::OpenAICompatible, QByteArrayLiteral("not json"), &error).isEmpty() && !error.isEmpty(),
          "AI: reject malformed JSON");
    check(Ai::responseText(Provider::Anthropic, QByteArrayLiteral("{}"), &error).isEmpty() && !error.isEmpty(),
          "AI: reject empty response");
    Ai::responseText(Provider::OpenAICompatible, QByteArrayLiteral("{\"error\":\"test-key-not-real\"}"), &error);
    check(!error.contains(QStringLiteral("test-key-not-real")), "AI security: server errors do not echo keys");
    Ai::responseText(Provider::OpenAICompatible, QByteArray(Ai::MaxResponseBytes + 1, 'x'), &error);
    check(!error.isEmpty(), "AI security: response parser size limit");

    const Exchange first = exchange(Provider::OpenAICompatible, openaiJson);
    check(first.finished && first.error.isEmpty() && first.text == QStringLiteral("Hello <b>world</b>"),
          "AI transport: loopback OpenAI request completes");
    check(first.request.startsWith("POST /v1/chat/completions HTTP/1.1")
              && first.request.contains("Bearer test-key-not-real"), "AI transport: correct OpenAI wire format");
    const Exchange second = exchange(Provider::Anthropic, anthropicJson);
    check(second.finished && second.error.isEmpty() && second.text == QStringLiteral("Hello world")
              && second.request.startsWith("POST /v1/messages HTTP/1.1"), "AI transport: loopback Anthropic request completes");
    const Exchange redirect = exchange(Provider::OpenAICompatible, QByteArrayLiteral("{}"), 302);
    check(redirect.finished && redirect.text.isEmpty() && redirect.error.contains(QStringLiteral("Redirect refused"))
              && redirect.requests == 1, "AI security: transport refuses redirects");
    const Exchange failure = exchange(Provider::OpenAICompatible, QByteArrayLiteral("{\"error\":\"test-key-not-real\"}"), 401);
    check(failure.finished && !failure.error.isEmpty() && !failure.error.contains(QStringLiteral("test-key-not-real")),
          "AI security: HTTP errors redact server body");
    const Exchange oversized = exchange(Provider::OpenAICompatible, QByteArray(Ai::MaxResponseBytes + 1, 'x'));
    check(oversized.finished && oversized.error.contains(QStringLiteral("4 MiB")), "AI security: transport bounds response size");
    const Exchange cancelled = exchange(Provider::OpenAICompatible, {}, 200, true);
    check(cancelled.finished && cancelled.error == QStringLiteral("Request cancelled."), "AI transport: cancellation completes");

    // Exercise the actual UI/settings path, not just the request-building helper.
    {
        AiPanel panel;
        auto *provider = panel.findChild<QComboBox *>(QStringLiteral("aiProvider"));
        auto *base = panel.findChild<QLineEdit *>(QStringLiteral("aiBaseUrl"));
        auto *model = panel.findChild<QComboBox *>(QStringLiteral("aiModel"));
        auto *key = panel.findChild<QLineEdit *>(QStringLiteral("aiApiKey"));
        auto *prompt = panel.findChild<QPlainTextEdit *>(QStringLiteral("aiPrompt"));
        provider->setCurrentIndex(0);
        base->setText(QStringLiteral("http://127.0.0.1:8000/v1"));
        key->setText(QStringLiteral("private-key-never-save"));
        prompt->setPlainText(QStringLiteral("private-prompt-never-save"));
        model->setCurrentText(QStringLiteral("custom-test-model"));
        QMetaObject::invokeMethod(base, "editingFinished", Qt::DirectConnection);
        panel.appendSelectedCode(QStringLiteral("int x;\u2029int y;"));
        check(prompt->toPlainText().contains(QStringLiteral("int x;\nint y;")), "AI panel: add selection without sending");
        check(key->echoMode() == QLineEdit::Password, "AI panel: API key is masked");
        QSettings settings;
        settings.sync();
        bool privateDataSaved = false;
        for (const QString &name : settings.allKeys()) {
            const QString value = settings.value(name).toString();
            privateDataSaved |= name.contains(QStringLiteral("apikey"), Qt::CaseInsensitive)
                || value.contains(QStringLiteral("private-key-never-save"))
                || value.contains(QStringLiteral("private-prompt-never-save"));
        }
        check(!privateDataSaved, "AI security: QSettings excludes key and prompt");
        base->setText(QStringLiteral("http://localhost:8001/v1"));
        check(key->text().isEmpty(), "AI security: endpoint change clears key");
        key->setText(QStringLiteral("private-key-never-save"));
        provider->setCurrentIndex(1);
        check(key->text().isEmpty(), "AI security: provider change clears key");
        provider->setCurrentIndex(0);
        check(model->currentText() == QStringLiteral("custom-test-model"), "AI panel: per-provider model restored");
    }
    {
        AiPanel panel;
        check(panel.findChild<QLineEdit *>(QStringLiteral("aiApiKey"))->text().isEmpty(),
              "AI security: new panel never restores a key");
        check(panel.findChild<QPlainTextEdit *>(QStringLiteral("aiPrompt"))->toPlainText().isEmpty(),
              "AI security: new panel never restores prompt/history");
    }
}
