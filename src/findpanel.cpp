#include "findpanel.h"

#include <QEvent>
#include <QGraphicsDropShadowEffect>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QToolButton>
#include <QVBoxLayout>

#include "icons.h"

FindPanel::FindPanel(QWidget *parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("FindPanel"));
    setAttribute(Qt::WA_StyledBackground, true);
    setAutoFillBackground(false);

    auto *shadow = new QGraphicsDropShadowEffect(this);
    shadow->setBlurRadius(28);
    shadow->setOffset(0, 6);
    shadow->setColor(QColor(0, 0, 0, 110));
    setGraphicsEffect(shadow);

    m_findEdit = new QLineEdit(this);
    m_findEdit->setPlaceholderText(tr("Find"));
    m_findEdit->setClearButtonEnabled(true);
    m_findEdit->setMinimumWidth(240);
    m_findEdit->installEventFilter(this);

    m_btnCase = new QToolButton(this);
    m_btnCase->setText(QStringLiteral("Aa"));
    m_btnCase->setToolTip(tr("Match case"));
    m_btnCase->setCheckable(true);
    m_btnCase->setAutoRaise(true);

    m_btnWord = new QToolButton(this);
    m_btnWord->setText(QStringLiteral("ab"));
    m_btnWord->setToolTip(tr("Match whole word"));
    m_btnWord->setCheckable(true);
    m_btnWord->setAutoRaise(true);

    m_btnRegex = new QToolButton(this);
    m_btnRegex->setText(QStringLiteral(".*"));
    m_btnRegex->setToolTip(tr("Regular expression"));
    m_btnRegex->setCheckable(true);
    m_btnRegex->setAutoRaise(true);

    m_btnPrev = new QToolButton(this);
    m_btnPrev->setToolTip(tr("Previous (Shift+Enter)"));
    m_btnPrev->setAutoRaise(true);
    m_btnPrev->setIcon(Icons::chevronUp(parentWidget() ? palette().color(QPalette::Text)
                                                       : QColor(Qt::black)));
    m_btnNext = new QToolButton(this);
    m_btnNext->setToolTip(tr("Next (Enter)"));
    m_btnNext->setAutoRaise(true);
    m_btnNext->setIcon(Icons::chevronDown(parentWidget() ? palette().color(QPalette::Text)
                                                         : QColor(Qt::black)));

    m_countLabel = new QLabel(this);
    m_countLabel->setObjectName(QStringLiteral("FindCount"));

    m_btnClose = new QToolButton(this);
    m_btnClose->setToolTip(tr("Close (Esc)"));
    m_btnClose->setAutoRaise(true);
    m_btnClose->setIcon(Icons::close(parentWidget() ? palette().color(QPalette::Text)
                                                    : QColor(Qt::black)));

    auto *findRow = new QHBoxLayout();
    findRow->setContentsMargins(12, 10, 10, 4);
    findRow->setSpacing(6);
    findRow->addWidget(m_findEdit, 1);
    findRow->addWidget(m_btnCase);
    findRow->addWidget(m_btnWord);
    findRow->addWidget(m_btnRegex);
    findRow->addWidget(m_btnPrev);
    findRow->addWidget(m_btnNext);
    findRow->addWidget(m_countLabel);
    findRow->addWidget(m_btnClose);

    m_replaceEdit = new QLineEdit(this);
    m_replaceEdit->setPlaceholderText(tr("Replace"));
    m_replaceEdit->setMinimumWidth(240);
    m_replaceEdit->installEventFilter(this);

    m_btnReplace = new QPushButton(tr("Replace"), this);
    m_btnReplaceAll = new QPushButton(tr("Replace All"), this);

    auto *replaceRow = new QHBoxLayout();
    replaceRow->setContentsMargins(12, 0, 10, 10);
    replaceRow->setSpacing(6);
    replaceRow->addWidget(m_replaceEdit, 1);
    replaceRow->addWidget(m_btnReplace);
    replaceRow->addWidget(m_btnReplaceAll);

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);
    root->addLayout(findRow);
    root->addLayout(replaceRow);

    setReplaceVisible(false);
    adjustSize();

    connect(m_findEdit, &QLineEdit::returnPressed, this, &FindPanel::findNextRequested);
    connect(m_btnNext, &QToolButton::clicked, this, &FindPanel::findNextRequested);
    connect(m_btnPrev, &QToolButton::clicked, this, &FindPanel::findPreviousRequested);
    connect(m_btnReplace, &QPushButton::clicked, this, &FindPanel::replaceRequested);
    connect(m_btnReplaceAll, &QPushButton::clicked, this, &FindPanel::replaceAllRequested);
    connect(m_btnClose, &QToolButton::clicked, this, &FindPanel::closePanel);
    connect(m_findEdit, &QLineEdit::textChanged, this, &FindPanel::criteriaChanged);

    auto optionChanged = static_cast<void (QToolButton::*)(bool)>(&QToolButton::toggled);
    connect(m_btnCase, optionChanged, this, &FindPanel::criteriaChanged);
    connect(m_btnWord, optionChanged, this, &FindPanel::criteriaChanged);
    connect(m_btnRegex, optionChanged, this, &FindPanel::criteriaChanged);
}

void FindPanel::openFind(const QString &seed)
{
    setReplaceVisible(false);
    if (!seed.isEmpty())
        m_findEdit->setText(seed);
    show();
    m_findEdit->setFocus();
    m_findEdit->selectAll();
}

void FindPanel::openReplace(const QString &seed)
{
    setReplaceVisible(true);
    if (!seed.isEmpty())
        m_findEdit->setText(seed);
    show();
    m_findEdit->setFocus();
    m_findEdit->selectAll();
}

void FindPanel::closePanel()
{
    hide();
    emit panelClosed();
}

void FindPanel::setReplaceVisible(bool visible)
{
    m_replaceVisible = visible;
    m_replaceEdit->setVisible(visible);
    m_btnReplace->setVisible(visible);
    m_btnReplaceAll->setVisible(visible);
    adjustSize();
    updateGeometry();
}

void FindPanel::setStatusText(const QString &text)
{
    m_countLabel->setText(text);
}

void FindPanel::setIconColors(const QColor &color)
{
    m_btnPrev->setIcon(Icons::chevronUp(color));
    m_btnNext->setIcon(Icons::chevronDown(color));
    m_btnClose->setIcon(Icons::close(color));
}

QString FindPanel::findText() const
{
    return m_findEdit->text();
}

QString FindPanel::replaceText() const
{
    return m_replaceEdit->text();
}

bool FindPanel::eventFilter(QObject *watched, QEvent *event)
{
    if ((watched == m_findEdit || watched == m_replaceEdit)
        && event->type() == QEvent::KeyPress) {
        auto *ke = static_cast<QKeyEvent *>(event);
        if (ke->key() == Qt::Key_Escape) {
            closePanel();
            return true;
        }
        if (watched == m_findEdit && ke->key() == Qt::Key_Return
            && (ke->modifiers() & Qt::ShiftModifier)) {
            emit findPreviousRequested();
            return true;
        }
        if (watched == m_replaceEdit && ke->key() == Qt::Key_Return) {
            emit replaceRequested();
            return true;
        }
    }
    return QWidget::eventFilter(watched, event);
}

void FindPanel::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    m_findEdit->setFocus();
    m_findEdit->selectAll();
}
