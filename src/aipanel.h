#pragma once

#include <QWidget>

#include "ai.h"

class QComboBox;
class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QPushButton;

class AiPanel : public QWidget
{
    Q_OBJECT
public:
    explicit AiPanel(QWidget *parent = nullptr);
    void appendSelectedCode(const QString &code);

signals:
    void selectionRequested();

private:
    Ai::Provider provider() const;
    QString settingsPrefix() const;
    void restoreProvider();
    void savePreferences();
    void setBusy(bool busy);

    AiClient *m_client;
    QComboBox *m_provider;
    QLineEdit *m_baseUrl;
    QComboBox *m_model;
    QLineEdit *m_key;
    QPlainTextEdit *m_prompt;
    QPlainTextEdit *m_response;
    QLabel *m_status;
    QPushButton *m_send;
    QPushButton *m_cancel;
    QPushButton *m_addSelection;
    QPushButton *m_clear;
    QPushButton *m_clearKey;
    int m_providerIndex = 0;
};
