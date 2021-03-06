/*
    This file is part of the KDE libraries
    SPDX-FileCopyrightText: 2006 Olivier Goffart <ogoffart at kde.org>

    SPDX-License-Identifier: LGPL-2.0-only
*/

#include "kassistantdialog.h"

#include <QDialogButtonBox>
#include <QIcon>
#include <QPushButton>
#include <QApplication>

#include <QHash>

class Q_DECL_HIDDEN KAssistantDialog::Private
{
public:
    Private(KAssistantDialog *q)
        : q(q)
    {
    }

    KAssistantDialog *const q;
    QHash<KPageWidgetItem *, bool> valid;
    QHash<KPageWidgetItem *, bool> appropriate;
    KPageWidgetModel *pageModel = nullptr;
    QPushButton *backButton = nullptr;
    QPushButton *nextButton = nullptr;
    QPushButton *finishButton = nullptr;

    void init();
    void _k_slotUpdateButtons();

    QModelIndex getNext(QModelIndex nextIndex)
    {
        QModelIndex currentIndex;
        do {
            currentIndex = nextIndex;
            nextIndex = pageModel->index(0, 0, currentIndex);
            if (!nextIndex.isValid()) {
                nextIndex = currentIndex.sibling(currentIndex.row() + 1, 0);
            }
        } while (nextIndex.isValid() && !appropriate.value(pageModel->item(nextIndex), true));
        return nextIndex;
    }

    QModelIndex getPrevious(QModelIndex nextIndex)
    {
        QModelIndex currentIndex;
        do {
            currentIndex = nextIndex;
            nextIndex = currentIndex.sibling(currentIndex.row() - 1, 0);
            if (!nextIndex.isValid()) {
                nextIndex = currentIndex.parent();
            }
        } while (nextIndex.isValid() && !appropriate.value(pageModel->item(nextIndex), true));
        return nextIndex;
    }
};

KAssistantDialog::KAssistantDialog(QWidget *parent, Qt::WindowFlags flags)
    : KPageDialog(parent, flags), d(new Private(this))
{
    d->init();
    //workaround to get the page model
    KPageWidget *pagewidget = findChild<KPageWidget *>();
    Q_ASSERT(pagewidget);
    d->pageModel = static_cast<KPageWidgetModel *>(pagewidget->model());
}

KAssistantDialog::KAssistantDialog(KPageWidget *widget, QWidget *parent, Qt::WindowFlags flags)
    : KPageDialog(widget, parent, flags), d(new Private(this))
{
    d->init();
    d->pageModel = static_cast<KPageWidgetModel *>(widget->model());
}

KAssistantDialog::~KAssistantDialog()
{
    delete d;
}

void KAssistantDialog::Private::init()
{
    QDialogButtonBox *buttonBox = q->buttonBox();

    buttonBox->setStandardButtons(QDialogButtonBox::Cancel | QDialogButtonBox::Help);
    backButton = new QPushButton;

    const QString iconBack = QApplication::isRightToLeft() ? QStringLiteral("go-next") : QStringLiteral("go-previous");
    const QString iconNext = QApplication::isRightToLeft() ? QStringLiteral("go-previous") : QStringLiteral("go-next");
    backButton->setText(tr("&Back", "@action:button go back"));
    backButton->setIcon(QIcon::fromTheme(iconBack));
    backButton->setToolTip(tr("Go back one step", "@info:tooltip"));
    q->connect(backButton, &QAbstractButton::clicked, q, &KAssistantDialog::back);
    buttonBox->addButton(backButton, QDialogButtonBox::ActionRole);

    nextButton = new QPushButton;
    nextButton->setText(tr("Next", "@action:button Opposite to Back"));
    nextButton->setIcon(QIcon::fromTheme(iconNext));
    nextButton->setDefault(true);
    q->connect(nextButton, &QAbstractButton::clicked, q, &KAssistantDialog::next);
    buttonBox->addButton(nextButton, QDialogButtonBox::ActionRole);

    finishButton = new QPushButton;
    finishButton->setText(tr("Finish", "@action:button"));
    finishButton->setIcon(QIcon::fromTheme(QStringLiteral("dialog-ok-apply")));
    buttonBox->addButton(finishButton, QDialogButtonBox::AcceptRole);

    q->setFaceType(KPageDialog::Plain);

    q->connect(q, SIGNAL(currentPageChanged(KPageWidgetItem*,KPageWidgetItem*)), q, SLOT(_k_slotUpdateButtons()));
}

void KAssistantDialog::back()
{
    QModelIndex nextIndex = d->getPrevious(d->pageModel->index(currentPage()));
    if (nextIndex.isValid()) {
        setCurrentPage(d->pageModel->item(nextIndex));
    }
}

void KAssistantDialog::next()
{
    QModelIndex nextIndex = d->getNext(d->pageModel->index(currentPage()));
    if (nextIndex.isValid()) {
        setCurrentPage(d->pageModel->item(nextIndex));
    } else if (isValid(currentPage())) {
        accept();
    }
}

void KAssistantDialog::setValid(KPageWidgetItem *page, bool enable)
{
    d->valid[page] = enable;
    if (page == currentPage()) {
        d->_k_slotUpdateButtons();
    }
}

bool KAssistantDialog::isValid(KPageWidgetItem *page) const
{
    return d->valid.value(page, true);
}

void KAssistantDialog::Private::_k_slotUpdateButtons()
{
    QModelIndex currentIndex = pageModel->index(q->currentPage());
    //change the caption of the next/finish button
    QModelIndex nextIndex = getNext(currentIndex);
    finishButton->setEnabled(!nextIndex.isValid() && q->isValid(q->currentPage()));
    nextButton->setEnabled(nextIndex.isValid() && q->isValid(q->currentPage()));
    finishButton->setDefault(!nextIndex.isValid());
    nextButton->setDefault(nextIndex.isValid());
    //enable or disable the back button;
    nextIndex = getPrevious(currentIndex);
    backButton->setEnabled(nextIndex.isValid());
}

void KAssistantDialog::showEvent(QShowEvent *event)
{
    d->_k_slotUpdateButtons(); //called because last time that function was called is when the first page was added, so the next button show "finish"
    KPageDialog::showEvent(event);
}

void KAssistantDialog::setAppropriate(KPageWidgetItem *page, bool appropriate)
{
    d->appropriate[page] = appropriate;
    d->_k_slotUpdateButtons();
}

bool KAssistantDialog::isAppropriate(KPageWidgetItem *page) const
{
    return d->appropriate.value(page, true);
}

QPushButton* KAssistantDialog::backButton() const
{
    return d->backButton;
}

QPushButton* KAssistantDialog::nextButton() const
{
    return d->nextButton;
}

QPushButton* KAssistantDialog::finishButton() const
{
    return d->finishButton;
}

#include "moc_kassistantdialog.cpp"
