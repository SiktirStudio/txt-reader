#pragma once

#include <QToolButton>
#include <QWidget>

class QCheckBox;
class QLineEdit;
class QPushButton;
class QLabel;

// Floating find / replace panel (VS Code style card docked above the bottom
// of the editor area).
class FindPanel : public QWidget
{
    Q_OBJECT
public:
    explicit FindPanel(QWidget *parent = nullptr);

    void openFind(const QString &seed);
    void openReplace(const QString &seed);
    void closePanel();

    bool isReplaceVisible() const { return m_replaceVisible; }
    QString findText() const;
    QString replaceText() const;
    bool matchCase() const { return m_btnCase->isChecked(); }
    bool wholeWord() const { return m_btnWord->isChecked(); }
    bool useRegex() const { return m_btnRegex->isChecked(); }

    void setStatusText(const QString &text);
    void setIconColors(const QColor &color);

signals:
    void findNextRequested();
    void findPreviousRequested();
    void replaceRequested();
    void replaceAllRequested();
    void criteriaChanged();
    void panelClosed();

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;
    void showEvent(QShowEvent *event) override;

private:
    void setReplaceVisible(bool visible);

    QLineEdit *m_findEdit = nullptr;
    QLineEdit *m_replaceEdit = nullptr;
    QToolButton *m_btnCase = nullptr;
    QToolButton *m_btnWord = nullptr;
    QToolButton *m_btnRegex = nullptr;
    QToolButton *m_btnPrev = nullptr;
    QToolButton *m_btnNext = nullptr;
    QPushButton *m_btnReplace = nullptr;
    QPushButton *m_btnReplaceAll = nullptr;
    QToolButton *m_btnClose = nullptr;
    QLabel *m_countLabel = nullptr;
    bool m_replaceVisible = false;
};
