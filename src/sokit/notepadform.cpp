#include "toolkit.h"
#include "setting.h"
#include "notepadform.h"

#include <QFile>
#include <QTextStream>
#include <QVBoxLayout>
#include <QShortcut>
#include <QTabWidget>

#define SET_NP_FILE  "sokit.txt"

NotepadForm::NotepadForm(QWidget* p, Qt::WindowFlags f)
	: QWidget(p, f)
{
	setupUi();
}

NotepadForm::~NotepadForm()
{
	uninit();
}

void NotepadForm::setupUi()
{
	resize(680, 450);

	auto lay = new QVBoxLayout(this);
	lay->setSpacing(5);
	lay->setContentsMargins(5, 5, 5, 5);

	m_board = new QPlainTextEdit(this);
	m_board->setAcceptDrops(false);
	m_board->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	m_board->setTabChangesFocus(false);
	m_board->setUndoRedoEnabled(true);
	m_board->setTextInteractionFlags(Qt::TextEditorInteraction);

	lay->addWidget(m_board);

	setWindowTitle(tr("Notepad"));

	QMetaObject::connectSlotsByName(this);

	auto n = new QShortcut(QKeySequence(Qt::Key_Tab | Qt::CTRL), this);
	auto l = new QShortcut(QKeySequence(Qt::Key_Tab | Qt::CTRL | Qt::SHIFT), this);
	n->setObjectName("n");
	l->setObjectName("l");

	connect(n, SIGNAL(activated()), this, SLOT(jumptab()));
	connect(l, SIGNAL(activated()), this, SLOT(jumptab()));
}

bool NotepadForm::init()
{
	QFile file(Setting::path() + "/" + SET_NP_FILE);

	if (file.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		QTextStream str(&file);
		m_board->setPlainText(str.readAll());

		file.close();
	}

	return true;
}

void NotepadForm::uninit()
{
	QFile file(Setting::path() + "/" + SET_NP_FILE);

	if (file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate))
	{
		QTextStream str(&file);
		str << m_board->toPlainText();

		str.flush();
		file.close();
	}
}

void NotepadForm::jumptab()
{
	if (parent())
	{
		auto p = qobject_cast<QTabWidget*>(parent()->parent());
		if (p)
		{
			qint32 max = p->count();
			qint32 cur = p->indexOf(this);

			if (sender()->objectName().startsWith('n'))
			{
				if (++cur >= max) cur = 0;
			}
			else
			{
				if (--cur < 0) cur = (max > 0) ? (max - 1) : 0;
			}

			p->setCurrentIndex(cur);
		}
	}
}
