#include <QDate>
#include <QDir>
#include <QTextStream>
#include <QKeySequence>
#include <QApplication>
#include <QClipboard>
#include <QTreeWidget>
#include <QPlainTextEdit>
#include "toolkit.h"
#include "setting.h"
#include "logger.h"

#define SET_MAX_LOGITM  100
#define SET_MAX_LOGTRM  30

Logger::Logger(QObject* parent)
	: QObject(parent), m_chkWrite(nullptr), m_treeOut(nullptr), m_textOut(nullptr)
{
}

Logger::~Logger()
{
	m_file.close();
}

void Logger::init(QTreeWidget* o, QCheckBox* w, QPlainTextEdit* d)
{
	m_cmlog.clear();
	m_cmtxt.clear();

	if (m_treeOut)
		m_treeOut->disconnect(this);

	if (m_textOut)
		m_textOut->disconnect(this);

	if (m_chkWrite)
		m_chkWrite->disconnect(this);

	m_treeOut = o;
	m_textOut = d;
	m_chkWrite = w;

	if (m_treeOut && m_textOut && m_chkWrite)
	{
		QList<QKeySequence> ks;
		ks << QKeySequence(Qt::CTRL | Qt::Key_D);

		auto copy = new QAction(tr("Copy"), this);
		copy->setShortcuts(QKeySequence::Copy);
		connect(copy, SIGNAL(triggered()), this, SLOT(copy()));

		auto clear = new QAction(tr("Clear"), this);
		clear->setShortcuts(ks);
		connect(clear, SIGNAL(triggered()), this, SIGNAL(clearLog()));

		auto all = new QAction(tr("Select All"), this);
		all->setShortcuts(QKeySequence::SelectAll);
		connect(all, SIGNAL(triggered()), m_textOut, SLOT(selectAll()));

		m_cmlog.addAction(copy);
		m_cmlog.addSeparator();
		m_cmlog.addAction(clear);

		m_cmtxt.addAction(copy);
		m_cmtxt.addSeparator();
		m_cmtxt.addAction(all);

		QPalette pal = m_textOut->palette();
		pal.setBrush(QPalette::Base, m_treeOut->palette().brush(QPalette::Window));
		m_textOut->setPalette(pal);

		m_treeOut->setContextMenuPolicy(Qt::CustomContextMenu);
		connect(m_treeOut, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(ctxmenu(const QPoint&)));
		connect(m_treeOut, SIGNAL(itemSelectionChanged()), this, SLOT(syncOutput()));

		m_textOut->setContextMenuPolicy(Qt::CustomContextMenu);
		connect(m_textOut, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(ctxmenu(const QPoint&)));
	}
}

void Logger::syncOutput()
{
	QList<QTreeWidgetItem*> list = m_treeOut->selectedItems();
	if (!list.isEmpty())
		m_textOut->setPlainText(list.first()->text(0));
	else
		m_textOut->clear();
}

void Logger::ctxmenu(const QPoint& pos)
{
	if (sender() == static_cast<QObject*>(m_treeOut))
		m_cmlog.exec(m_treeOut->mapToGlobal(pos));
	else
		m_cmtxt.exec(m_textOut->mapToGlobal(pos));
}

void Logger::copy()
{
	if (sender() == static_cast<QObject*>(m_treeOut))
	{
		QList<QTreeWidgetItem*> list = m_treeOut->selectedItems();
		if (!list.isEmpty())
			QApplication::clipboard()->setText(list.first()->text(0));
	}
	else
	{
		m_textOut->copy();
	}
}

const QString Logger::getLogFileName()
{
	int i = 0;
	while (2 > i++)
	{
		if (!m_dir.isEmpty())
		{
			QDir d;
			if (d.exists(m_dir) || d.mkpath(m_dir))
			{
				i = 0;
				break;
			}
		}

		m_dir = Setting::path() + "/" + property(SET_SEC_DIR).toString();
	}

	return (i == 2)
		       ? QString()
		       : m_dir + QDir::separator() +
		       QDate::currentDate().toString("yyyyMMdd.log");
}

void Logger::writeLogFile(const QString& info)
{
	if (!m_chkWrite->isChecked())
		return;

	m_file.close();
	m_file.setFileName(getLogFileName());

	if (m_file.open(QIODevice::Append |
		QIODevice::WriteOnly |
		QIODevice::Text))
	{
		QByteArray a(info.toUtf8());

		const char* d = a.data();
		for (int n = a.size(); n > 0;)
		{
			int w = m_file.write(d, n);

			d += w;
			n -= w;
		}

		m_file.close();
	}
}

void Logger::clear()
{
	m_treeOut->clear();
	m_textOut->clear();
}

void Logger::output(const QString& info)
{
	output("MSG", info);
}

void Logger::output(const char* buf, uint len)
{
	output("DAT", buf, len);
}

void Logger::pack()
{
	if (m_treeOut->topLevelItemCount() > SET_MAX_LOGITM)
		m_treeOut->model()->removeRows(0, SET_MAX_LOGTRM);

	m_treeOut->scrollToBottom();
}

QTreeWidgetItem* Logger::appendLogEntry(QTreeWidgetItem* p, const QString& t)
{
	auto res = new QTreeWidgetItem(p);
	if (res)
	{
		res->setText(0, t);

		if (p)
		{
			p->addChild(res);
		}
		else
		{
			m_treeOut->addTopLevelItem(res);
			m_textOut->setPlainText(t);
		}
	}
	return res;
}

void Logger::output(const QString& title, const QString& info)
{
	auto it = new QTreeWidgetItem(0);
	if (!it) return;

	QString lab(QTime::currentTime().toString("HH:mm:ss "));

	lab += title;
	lab += ' ';
	lab += info;

	appendLogEntry(nullptr, lab);

	pack();

	lab += '\n';
	lab += '\n';

	writeLogFile(lab);
}

void Logger::output(const QString& title, const char* buf, quint32 len)
{
	QString lab(QTime::currentTime().toString("HH:mm:ss "));

	QTextStream out(&lab);

	out << title
		<< " <" << len << "> "
		<< TK::bin2ascii(buf, len);

	QString hex = TK::bin2hex(buf, len);

	QTreeWidgetItem* it = appendLogEntry(nullptr, lab);
	if (it)
	{
		appendLogEntry(it, hex);

		pack();
	}

	out << '\n' << hex << '\n' << '\n';

	writeLogFile(lab);
}
