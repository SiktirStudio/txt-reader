#include "ai.h"

#include <QHostAddress>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QSslConfiguration>
#include <QSslSocket>
#include <QTimer>

namespace {
bool isLoopback(const QUrl &url)
{
    return url.host().compare(QStringLiteral("localhost"), Qt::CaseInsensitive) == 0
        || QHostAddress(url.host()).isLoopback();
}

void setError(QString *error, const QString &message)
{
    if (error)
        *error = message;
}
}

QUrl Ai::endpoint(Provider provider, const QString &baseUrl, QString *error)
{
    setError(error, QString());
    QUrl url(baseUrl.trimmed(), QUrl::StrictMode);
    if (!url.isValid() || url.isRelative() || url.host().isEmpty()
        || !url.userInfo().isEmpty() || url.hasQuery() || url.hasFragment()
        || (url.scheme() != QStringLiteral("https") && url.scheme() != QStringLiteral("http"))) {
        setError(error, QObject::tr("Use an absolute HTTPS base URL without credentials, query or fragment."));
        return {};
    }
    if (url.scheme() == QStringLiteral("http") && !isLoopback(url)) {
        setError(error, QObject::tr("HTTP is allowed only for localhost or a literal loopback address."));
        return {};
    }
    QString path = url.path();
    while (path.endsWith(QLatin1Char('/')))
        path.chop(1);
    const QString suffix = provider == Provider::Anthropic
        ? QStringLiteral("/messages") : QStringLiteral("/chat/completions");
    if (!path.endsWith(suffix))
        path += suffix;
    url.setPath(path);
    return url;
}

Ai::Request Ai::prepare(Provider provider, const QString &baseUrl, const QString &model,
                        const QString &apiKey, const QString &prompt)
{
    Request result;
    const QUrl url = endpoint(provider, baseUrl, &result.error);
    if (!result.error.isEmpty())
        return result;
    if (model.trimmed().isEmpty() || model.size() > 256) {
        result.error = QObject::tr("Enter a model ID (up to 256 characters). Choose one available to your account.");
        return result;
    }
    // Reject header injection, non-ASCII keys and accidental whitespace.
    for (QChar ch : apiKey) {
        if (ch.unicode() < 0x21 || ch.unicode() > 0x7e) {
            result.error = QObject::tr("The API key contains whitespace or unsupported characters.");
            return result;
        }
    }
    if (apiKey.size() > 4096 || (apiKey.isEmpty() && !isLoopback(url))) {
        result.error = QObject::tr("Enter an API key. Only loopback servers may use an empty key.");
        return result;
    }
    if (prompt.trimmed().isEmpty() || prompt.toUtf8().size() > MaxPromptBytes) {
        result.error = QObject::tr("Enter a prompt of at most 1 MiB (UTF-8).");
        return result;
    }

    result.request = QNetworkRequest(url);
    result.request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    result.request.setRawHeader("Accept", "application/json");
    result.request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::ManualRedirectPolicy);
    result.request.setAttribute(QNetworkRequest::CookieLoadControlAttribute, QNetworkRequest::Manual);
    result.request.setAttribute(QNetworkRequest::CookieSaveControlAttribute, QNetworkRequest::Manual);
    result.request.setAttribute(QNetworkRequest::AuthenticationReuseAttribute, QNetworkRequest::Manual);
    result.request.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::AlwaysNetwork);
    result.request.setAttribute(QNetworkRequest::CacheSaveControlAttribute, false);
    result.request.setTransferTimeout(60000);
    QSslConfiguration tls = QSslConfiguration::defaultConfiguration();
    tls.setPeerVerifyMode(QSslSocket::VerifyPeer);
    tls.setProtocol(QSsl::TlsV1_2OrLater);
    result.request.setSslConfiguration(tls);

    const QJsonArray messages { QJsonObject { { QStringLiteral("role"), QStringLiteral("user") },
                                              { QStringLiteral("content"), prompt } } };
    QJsonObject body { { QStringLiteral("model"), model.trimmed() },
                       { QStringLiteral("messages"), messages },
                       { QStringLiteral("stream"), false } };
    if (provider == Provider::Anthropic) {
        if (!apiKey.isEmpty())
            result.request.setRawHeader("x-api-key", apiKey.toLatin1());
        result.request.setRawHeader("anthropic-version", "2023-06-01");
        body.insert(QStringLiteral("max_tokens"), 4096);
    } else if (!apiKey.isEmpty()) {
        result.request.setRawHeader("Authorization", QByteArrayLiteral("Bearer ") + apiKey.toLatin1());
    }
    result.body = QJsonDocument(body).toJson(QJsonDocument::Compact);
    return result;
}

QString Ai::responseText(Provider provider, const QByteArray &json, QString *error)
{
    setError(error, QString());
    if (json.size() > MaxResponseBytes) {
        setError(error, QObject::tr("The response exceeds the 4 MiB limit."));
        return {};
    }
    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(json, &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()
        || doc.object().contains(QStringLiteral("error"))) {
        // Do not echo server errors: they may contain credentials or prompt text.
        setError(error, QObject::tr("The server did not return a valid assistant response."));
        return {};
    }
    QJsonValue content;
    if (provider == Provider::Anthropic) {
        content = doc.object().value(QStringLiteral("content"));
    } else {
        const QJsonArray choices = doc.object().value(QStringLiteral("choices")).toArray();
        if (!choices.isEmpty())
            content = choices.first().toObject().value(QStringLiteral("message"))
                          .toObject().value(QStringLiteral("content"));
    }
    QString text;
    if (content.isString()) {
        text = content.toString();
    } else if (content.isArray()) {
        for (const QJsonValue &block : content.toArray()) {
            const QJsonObject object = block.toObject();
            if (object.value(QStringLiteral("type")).toString() == QStringLiteral("text"))
                text += object.value(QStringLiteral("text")).toString();
        }
    }
    if (text.isEmpty())
        setError(error, QObject::tr("The server returned no assistant text. Check the provider and model."));
    return text;
}

AiClient::AiClient(QObject *parent)
    : QObject(parent), m_network(new QNetworkAccessManager(this))
{
}

AiClient::~AiClient()
{
    if (m_reply) {
        m_reply->disconnect(this);
        m_reply->abort();
    }
}

bool AiClient::send(Ai::Provider provider, const QString &baseUrl, const QString &model,
                    const QString &apiKey, const QString &prompt)
{
    if (isBusy())
        return false;
    const Ai::Request prepared = Ai::prepare(provider, baseUrl, model, apiKey, prompt);
    if (!prepared.error.isEmpty()) {
        emit failed(prepared.error);
        return false;
    }
    if (prepared.request.url().scheme() == QStringLiteral("https") && !QSslSocket::supportsSsl()) {
        emit failed(tr("No TLS backend is available. Reinstall the complete NovaPad package."));
        return false;
    }
    m_response.clear();
    m_failure.clear();
    QNetworkReply *reply = m_network->post(prepared.request, prepared.body);
    m_reply = reply;
    reply->setReadBufferSize(Ai::MaxResponseBytes + 1);
    connect(reply, &QNetworkReply::readyRead, this, [this, reply] { collectResponse(reply); });
    connect(reply, &QNetworkReply::finished, this, [this, reply, provider] {
        if (reply->isOpen())
            collectResponse(reply);
        const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        QString error = m_failure;
        QString text;
        if (error.isEmpty()) {
            if (status >= 300 && status < 400)
                error = tr("Redirect refused. Configure the final trusted base URL explicitly.");
            else if (status < 200 || status >= 300 || reply->error() != QNetworkReply::NoError)
                error = tr("Request failed (HTTP %1). Check connectivity, TLS, credentials and model.").arg(status);
            else
                text = Ai::responseText(provider, m_response, &error);
        }
        m_reply.clear();
        m_response.clear();
        reply->deleteLater();
        emit busyChanged(false);
        if (error.isEmpty())
            emit completed(text);
        else
            emit failed(error);
    });
    QTimer::singleShot(60000, reply, [this, reply] {
        m_failure = tr("Request timed out after 60 seconds.");
        reply->abort();
    });
    emit busyChanged(true);
    return true;
}

void AiClient::collectResponse(QNetworkReply *reply)
{
    if (!m_failure.isEmpty())
        return;
    m_response += reply->read(Ai::MaxResponseBytes - m_response.size() + 1);
    if (m_response.size() > Ai::MaxResponseBytes) {
        m_failure = tr("The response exceeds the 4 MiB limit.");
        reply->abort();
    }
}

void AiClient::cancel()
{
    if (m_reply) {
        m_failure = tr("Request cancelled.");
        m_reply->abort();
    }
}
