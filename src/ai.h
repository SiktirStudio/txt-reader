#pragma once

#include <QByteArray>
#include <QNetworkRequest>
#include <QObject>
#include <QPointer>
#include <QString>
#include <QUrl>

class QNetworkAccessManager;
class QNetworkReply;

namespace Ai {
enum class Provider { OpenAICompatible, Anthropic };
constexpr int MaxPromptBytes = 1024 * 1024;
constexpr int MaxResponseBytes = 4 * 1024 * 1024;

struct Request {
    QNetworkRequest request;
    QByteArray body;
    QString error;
};

// No DNS lookup: HTTP is restricted to literal loopback addresses / localhost.
QUrl endpoint(Provider provider, const QString &baseUrl, QString *error = nullptr);
Request prepare(Provider provider, const QString &baseUrl, const QString &model,
                const QString &apiKey, const QString &prompt);
QString responseText(Provider provider, const QByteArray &json, QString *error = nullptr);
}

// One explicit, non-streaming request at a time. No disk cache, cookies, history,
// credential persistence, automatic redirects, or TLS-error bypasses.
class AiClient : public QObject
{
    Q_OBJECT
public:
    explicit AiClient(QObject *parent = nullptr);
    ~AiClient() override;
    bool send(Ai::Provider provider, const QString &baseUrl, const QString &model,
              const QString &apiKey, const QString &prompt);
    bool isBusy() const { return !m_reply.isNull(); }
    void cancel();

signals:
    void busyChanged(bool busy);
    void completed(const QString &text);
    void failed(const QString &message);

private:
    void collectResponse(QNetworkReply *reply);
    QNetworkAccessManager *m_network;
    QPointer<QNetworkReply> m_reply;
    QByteArray m_response;
    QString m_failure;
};
