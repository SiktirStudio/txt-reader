#include "aipanel.h"

#include <QComboBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSettings>
#include <QSignalBlocker>
#include <QSplitter>
#include <QVBoxLayout>

AiPanel::AiPanel(QWidget *parent)
    : QWidget(parent), m_client(new AiClient(this))
{
    setMinimumWidth(320);
    auto *layout = new QVBoxLayout(this);
    auto *form = new QFormLayout;
    form->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    m_provider = new QComboBox(this);
    m_provider->setObjectName(QStringLiteral("aiProvider"));
    m_provider->addItems({tr("OpenAI-compatible"), tr("Anthropic")});
    m_baseUrl = new QLineEdit(this);
    m_baseUrl->setObjectName(QStringLiteral("aiBaseUrl"));
    m_baseUrl->setMaxLength(2048);
    m_baseUrl->setToolTip(tr("Trusted API base URL, including /v1 if required. HTTPS except for loopback."));
    m_model = new QComboBox(this);
    m_model->setObjectName(QStringLiteral("aiModel"));
    m_model->setEditable(true);
    m_model->setInsertPolicy(QComboBox::NoInsert);
    m_model->lineEdit()->setMaxLength(256);
    m_key = new QLineEdit(this);
    m_key->setObjectName(QStringLiteral("aiApiKey"));
    m_key->setEchoMode(QLineEdit::Password);
    m_key->setInputMethodHints(Qt::ImhSensitiveData | Qt::ImhNoPredictiveText | Qt::ImhNoAutoUppercase);
    m_key->setMaxLength(4096);
    m_key->setPlaceholderText(tr("Memory only; never saved"));
    m_clearKey = new QPushButton(tr("Clear key"), this);
    auto *keyRow = new QHBoxLayout;
    keyRow->addWidget(m_key, 1);
    keyRow->addWidget(m_clearKey);
    form->addRow(tr("Provider"), m_provider);
    form->addRow(tr("Base URL"), m_baseUrl);
    form->addRow(tr("Model"), m_model);
    form->addRow(tr("API key"), keyRow);
    layout->addLayout(form);

    auto *privacy = new QLabel(tr("Only the prompt is sent, and only when you click Send. "
        "The key, prompt and response stay in memory, not in settings. Use only trusted endpoints."), this);
    privacy->setWordWrap(true);
    layout->addWidget(privacy);
    auto *splitter = new QSplitter(Qt::Vertical, this);
    auto *promptSection = new QWidget(splitter);
    auto *promptLayout = new QVBoxLayout(promptSection);
    promptLayout->setContentsMargins(0, 0, 0, 0);
    promptLayout->addWidget(new QLabel(tr("Prompt"), promptSection));
    m_prompt = new QPlainTextEdit(promptSection);
    m_prompt->setObjectName(QStringLiteral("aiPrompt"));
    m_prompt->setPlaceholderText(tr("Ask a question; optionally add the selected code below."));
    promptLayout->addWidget(m_prompt);
    auto *responseSection = new QWidget(splitter);
    auto *responseLayout = new QVBoxLayout(responseSection);
    responseLayout->setContentsMargins(0, 0, 0, 0);
    responseLayout->addWidget(new QLabel(tr("Response (plain text)"), responseSection));
    m_response = new QPlainTextEdit(responseSection);
    m_response->setObjectName(QStringLiteral("aiResponse"));
    m_response->setReadOnly(true);
    responseLayout->addWidget(m_response);
    splitter->addWidget(promptSection);
    splitter->addWidget(responseSection);
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 2);
    layout->addWidget(splitter, 1);

    auto *contextButtons = new QHBoxLayout;
    m_addSelection = new QPushButton(tr("Add selected code"), this);
    m_addSelection->setObjectName(QStringLiteral("aiAddSelection"));
    m_clear = new QPushButton(tr("Clear text"), this);
    contextButtons->addWidget(m_addSelection);
    contextButtons->addWidget(m_clear);
    layout->addLayout(contextButtons);
    auto *sendButtons = new QHBoxLayout;
    m_send = new QPushButton(tr("Send"), this);
    m_send->setObjectName(QStringLiteral("aiSend"));
    m_cancel = new QPushButton(tr("Cancel"), this);
    sendButtons->addWidget(m_send);
    sendButtons->addWidget(m_cancel);
    layout->addLayout(sendButtons);
    m_status = new QLabel(this);
    m_status->setObjectName(QStringLiteral("aiStatus"));
    m_status->setTextFormat(Qt::PlainText);
    m_status->setWordWrap(true);
    layout->addWidget(m_status);

    QSettings settings;
    m_providerIndex = settings.value(QStringLiteral("ai/provider")).toString() == QStringLiteral("anthropic") ? 1 : 0;
    m_provider->setCurrentIndex(m_providerIndex);
    restoreProvider();
    connect(m_provider, &QComboBox::currentIndexChanged, this, [this](int index) {
        savePreferences();
        m_providerIndex = index;
        m_key->clear();
        restoreProvider();
        savePreferences();
    });
    // Never reuse a credential silently after an endpoint change.
    connect(m_baseUrl, &QLineEdit::textChanged, m_key, &QLineEdit::clear);
    connect(m_baseUrl, &QLineEdit::editingFinished, this, &AiPanel::savePreferences);
    connect(m_model, &QComboBox::currentTextChanged, this, &AiPanel::savePreferences);
    connect(m_clearKey, &QPushButton::clicked, m_key, &QLineEdit::clear);
    connect(m_addSelection, &QPushButton::clicked, this, &AiPanel::selectionRequested);
    connect(m_clear, &QPushButton::clicked, this, [this] {
        m_prompt->clear();
        m_response->clear();
        m_status->clear();
    });
    connect(m_send, &QPushButton::clicked, this, [this] {
        savePreferences();
        m_response->clear();
        m_status->setText(tr("Sending…"));
        m_client->send(provider(), m_baseUrl->text(), m_model->currentText(),
                       m_key->text(), m_prompt->toPlainText());
    });
    connect(m_cancel, &QPushButton::clicked, m_client, &AiClient::cancel);
    connect(m_client, &AiClient::busyChanged, this, &AiPanel::setBusy);
    connect(m_client, &AiClient::failed, m_status, &QLabel::setText);
    connect(m_client, &AiClient::completed, this, [this](const QString &text) {
        m_response->setPlainText(text);
        m_status->setText(tr("Done. Review AI output before using it."));
    });
    setBusy(false);
}

Ai::Provider AiPanel::provider() const
{
    return m_providerIndex == 1 ? Ai::Provider::Anthropic : Ai::Provider::OpenAICompatible;
}

QString AiPanel::settingsPrefix() const
{
    return m_providerIndex == 1 ? QStringLiteral("ai/anthropic/") : QStringLiteral("ai/openai/");
}

void AiPanel::restoreProvider()
{
    const QSignalBlocker blockModel(m_model);
    const QSignalBlocker blockUrl(m_baseUrl);
    QSettings settings;
    const bool anthropic = provider() == Ai::Provider::Anthropic;
    m_baseUrl->setText(settings.value(settingsPrefix() + QStringLiteral("baseUrl"),
        anthropic ? QStringLiteral("https://api.anthropic.com/v1")
                  : QStringLiteral("https://api.openai.com/v1")).toString());
    m_model->clear();
    m_model->addItems(anthropic ? QStringList { QStringLiteral("claude-sonnet-4-5"), QStringLiteral("claude-haiku-4-5") }
                               : QStringList { QStringLiteral("gpt-4.1-mini"), QStringLiteral("gpt-4.1"), QStringLiteral("gpt-4o-mini") });
    m_model->setCurrentText(settings.value(settingsPrefix() + QStringLiteral("model"), m_model->itemText(0)).toString());
}

void AiPanel::savePreferences()
{
    QSettings settings;
    settings.setValue(QStringLiteral("ai/provider"), m_providerIndex == 1 ? QStringLiteral("anthropic") : QStringLiteral("openai"));
    QString error;
    Ai::endpoint(provider(), m_baseUrl->text(), &error);
    // Reject URL credentials / queries instead of accidentally persisting them.
    if (error.isEmpty())
        settings.setValue(settingsPrefix() + QStringLiteral("baseUrl"), m_baseUrl->text().trimmed());
    settings.setValue(settingsPrefix() + QStringLiteral("model"), m_model->currentText().trimmed());
    // Deliberate allowlist: never persist the API key, prompt, response or history.
}

void AiPanel::setBusy(bool busy)
{
    m_send->setEnabled(!busy);
    m_cancel->setEnabled(busy);
    m_provider->setEnabled(!busy);
    m_baseUrl->setEnabled(!busy);
    m_model->setEnabled(!busy);
    m_key->setEnabled(!busy);
    m_clearKey->setEnabled(!busy);
    m_addSelection->setEnabled(!busy);
    m_clear->setEnabled(!busy);
    m_prompt->setReadOnly(busy);
}

void AiPanel::appendSelectedCode(const QString &code)
{
    if (m_client->isBusy())
        return;
    if (code.isEmpty()) {
        m_status->setText(tr("Select code in the editor first. Nothing was sent."));
        return;
    }
    QString normalized = code;
    normalized.replace(QChar::ParagraphSeparator, QLatin1Char('\n'));
    const QString text = m_prompt->toPlainText() + QStringLiteral("\n\nSelected code:\n") + normalized;
    if (text.toUtf8().size() > Ai::MaxPromptBytes) {
        m_status->setText(tr("Adding this selection would exceed the 1 MiB prompt limit."));
        return;
    }
    m_prompt->setPlainText(text);
    m_status->setText(tr("Selection added to the prompt. Click Send when ready."));
}
