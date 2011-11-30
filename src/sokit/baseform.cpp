#include "toolkit.h"
#include "setting.h"
#include "baseform.h"

#include <QShortcut>
#include <QWidget>
#include <QLabel>
#include <QToolButton>
#include <QLineEdit>
#include <QTreeWidget>
#include <QComboBox>
#include <QListWidget>
#include <QPlainTextEdit>

#define PROP_EDIT "edit"
#define PROP_DIRT "dirt"
#define PROP_TARG "targ"

BaseForm::BaseForm(QWidget* p, Qt::WFlags f)
:QWidget(p, f),m_cntRecv(0),m_cntSend(0),m_labRecv(0),m_labSend(0),m_cnlist(0)
{
}

BaseForm::~BaseForm()
{
}

bool BaseForm::init()
{
	if (!initForm() || !initHotkeys())
		return false;

	initConfig();

	m_logger.setProperty(SET_SEC_DIR, property(SET_SEC_DIR).toString());

	return true;
}

void BaseForm::initCounter(QLabel* r, QLabel* s)
{
	m_labRecv = r;
	m_labSend = s;
}

void BaseForm::initLogger(QCheckBox* w, QToolButton* c, QTreeWidget* o, QPlainTextEdit* d)
{
	m_logger.init(o, w, d);

	connect(c, SIGNAL(released()), this, SLOT(clear()));
	connect(&m_logger, SIGNAL(clearLog()), this, SLOT(clear()));

	bindFocus(o, Qt::Key_F3);

	QShortcut* wr = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_W), this);
	QShortcut* cl = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_D), this);
	QShortcut* sl = new QShortcut(QKeySequence(Qt::Key_F4), this);

	sl->setProperty(PROP_TARG, qVariantFromValue((void*)d));

	connect(wr, SIGNAL(activated()), w, SLOT(click()));
	connect(sl, SIGNAL(activated()), this, SLOT(hotOutput()));
	connect(cl, SIGNAL(activated()), this, SLOT(clear()));

	connect(this, SIGNAL(output(const QString&)), &m_logger, SLOT(output(const QString&)));
	connect(this, SIGNAL(output(const QString&, const char*, quint32)), &m_logger, SLOT(output(const QString&, const char*, quint32)));
}

void BaseForm::initLister(QToolButton* a, QToolButton* k, QListWidget* l)
{
	m_cnlist = l;

	QShortcut* sk = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_K), this);
	QShortcut* sa = new QShortcut(QKeySequence(Qt::ALT  + Qt::Key_A), this);

	connect(sk, SIGNAL(activated()), this, SLOT(kill()));
	connect(sa, SIGNAL(activated()), m_cnlist, SLOT(selectAll()));

	connect(k, SIGNAL(released()), this, SLOT(kill()));
	connect(a, SIGNAL(released()), m_cnlist, SLOT(selectAll()));

	bindFocus(m_cnlist, Qt::Key_F2);
}

void BaseForm::bindBuffer(qint32 id, QLineEdit* e, QToolButton* s, QComboBox* d)
{
	s->setProperty(PROP_EDIT, qVariantFromValue((void*)e));
	s->setProperty(PROP_DIRT, qVariantFromValue((void*)d));

	connect(s, SIGNAL(released()), this, SLOT(send()));

	bindClick(s, Qt::Key_0 + id + Qt::CTRL);
	bindFocus(e, Qt::Key_0 + id + Qt::ALT);
	bindFocus(d, Qt::Key_0 + id + Qt::CTRL + Qt::SHIFT);
}

void BaseForm::bindFocus(QWidget* w, qint32 k)
{
	QShortcut* s = new QShortcut(QKeySequence(k), this);
	s->setProperty(PROP_TARG, qVariantFromValue((void*)w));
	connect(s, SIGNAL(activated()), this, SLOT(focus()));
}

void BaseForm::bindClick(QAbstractButton* b, qint32 k)
{
	QShortcut* s = new QShortcut(QKeySequence(k), this);
	connect(s, SIGNAL(activated()), b, SLOT(click()));
}

void BaseForm::bindSelect(QComboBox* b, qint32 i, qint32 k)
{
	QShortcut* s = new QShortcut(QKeySequence(k), this);
	s->setProperty(PROP_TARG, qVariantFromValue((void*)b));
	s->setObjectName(QString::number(i));

	connect(s, SIGNAL(activated()), this, SLOT(select()));
}

void BaseForm::focus()
{
	QWidget* w = (QWidget*)sender()->property(PROP_TARG).value<void*>();
	if (w) w->setFocus(Qt::TabFocusReason);
}

void BaseForm::hotOutput()
{
	QPlainTextEdit* t = (QPlainTextEdit*)sender()->property(PROP_TARG).value<void*>();
	if (t)
	{
		t->setFocus(Qt::TabFocusReason);
		t->selectAll();
	}
}

void BaseForm::select()
{
	QComboBox* b = (QComboBox*)sender()->property(PROP_TARG).value<void*>();
	if (b && b->isEnabled())
	{
		qint32 i = sender()->objectName().toInt();
		if (i < 0)
		{
			i = b->currentIndex() + 1;
			if (i >= b->count()) i = 0;
		}

		b->setCurrentIndex(i);
	}
}

void BaseForm::countRecv(qint32 bytes)
{
	if (bytes < 0)
		m_cntRecv = 0;
	else
		m_cntRecv += bytes;		

	m_labRecv->setText(QString::number(m_cntRecv));
}

void BaseForm::countSend(qint32 bytes)
{
	if (bytes < 0)
		m_cntSend = 0;
	else
		m_cntSend += bytes;

	m_labSend->setText(QString::number(m_cntSend));
}

void BaseForm::send()
{
	QLineEdit* e = (QLineEdit*)sender()->property(PROP_EDIT).value<void*>();
	QComboBox* d = (QComboBox*)sender()->property(PROP_DIRT).value<void*>();
	if (e)
		send(e->text(), (d?d->currentText():""));
}

void BaseForm::clear()
{
	m_logger.clear();

	lock();
	countRecv(-1);
	countSend(-1);
	unlock();
}

void BaseForm::kill()
{
	if (lock(1000))
	{
		QStringList list;

		listerSelected(list);
		kill(list);

		unlock();
	}
}

void BaseForm::listerSelected(QStringList& output)
{
	qint32 i = m_cnlist->count();
	while (i--)
	{
		QListWidgetItem* itm = m_cnlist->item(i);
		if (itm && itm->isSelected())
			output << itm->text();
	}
}

void BaseForm::listerAdd(const QString& caption)
{
	listerRemove(caption);
	m_cnlist->addItem(caption);
}

void BaseForm::listerRemove(const QString& caption)
{
	qint32 i = m_cnlist->count();
	while (i--)
	{
		QListWidgetItem* itm = m_cnlist->item(i);
		if (itm && itm->text()==caption)
			delete m_cnlist->takeItem(i);
	}
}

