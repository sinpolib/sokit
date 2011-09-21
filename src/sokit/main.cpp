#include <QTabWidget>
#include <QShortcut>
#include <QFontDatabase>

#include "toolkit.h"
#include "setting.h"
#include "clientform.h"
#include "serverform.h"
#include "transferform.h"
#include "notepadform.h"
#include "helpform.h"
#include "main.h"

#define SET_KEY_FTNM "/font/name"
#define SET_KEY_FTSZ "/font/size"

#define SET_KEY_LANG "/lang"
#define SET_VAL_LANG "sokit"
#define SET_VAL_LANX ".lan"

Sokit::Sokit(int& argc, char** argv)
:QApplication(argc,argv)
{
}

Sokit::~Sokit()
{
}

void Sokit::show()
{
	m_wnd.show();
}

void Sokit::close()
{
	m_wnd.close();
}

void Sokit::initDefaultActionsName()
{
	translate("QLineEdit", "&Undo");
	translate("QLineEdit", "&Redo");
	translate("QLineEdit", "Cu&t");
	translate("QLineEdit", "&Copy");
	translate("QLineEdit", "&Paste");
	translate("QLineEdit", "Delete");
	translate("QLineEdit", "Select All");

	translate("QTextControl", "&Undo");
	translate("QTextControl", "&Redo");
	translate("QTextControl", "Cu&t");
	translate("QTextControl", "&Copy");
	translate("QTextControl", "&Paste");
	translate("QTextControl", "Delete");
	translate("QTextControl", "Select All");
}

bool Sokit::initTranslator()
{
	QString file = Setting::get(SET_SEC_CFG, SET_KEY_LANG, SET_VAL_LANG);

	QStringList paths;
	paths << "."
        << "../share/" SET_APP_NAME
        << "../share/apps/" SET_APP_NAME
		<< Setting::path();

	foreach (QString p, paths)
	{
		if (m_trans.load(file, p, "", SET_VAL_LANX))
		{
			installTranslator(&m_trans);
			Setting::set(SET_SEC_CFG, SET_KEY_LANG, file);
			break;
		}
	}

	return true;
}

void Sokit::initFont()
{
	QFontDatabase db;
	QStringList fs = db.families();

	QFont font;

	int match = 0;

	QString family = Setting::get(SET_SEC_CFG, SET_KEY_FTNM, "").trimmed();
	QString size    = Setting::get(SET_SEC_CFG, SET_KEY_FTSZ, "").trimmed();

	if (family.isEmpty() || fs.filter(family).isEmpty())
	{
		QStringList defs = translate("Sokit", "font").split(";", QString::SkipEmptyParts);
		foreach (QString d, defs)
		{
			family = d.section(',', 0, 0).trimmed();
			size    = d.section(',', 1, 1).trimmed();

			if (!family.isEmpty() && !fs.filter(family).isEmpty())
			{
				match = 2;
				break;
			}
		}
	}
	else
	{
		match = 1;
	}

	if (match > 0)
	{
		font.setFamily(family);

		if (db.isSmoothlyScalable(family))
			font.setStyleStrategy((QFont::StyleStrategy)(QFont::PreferAntialias|QFont::PreferOutline|QFont::PreferQuality));

		int nsize = size.toInt();
		if (nsize > 0 && nsize < 20)
			font.setPointSize(nsize);

		setFont(font);

		if (match > 1)
		{
			Setting::set(SET_SEC_CFG, SET_KEY_FTNM, family);
			Setting::set(SET_SEC_CFG, SET_KEY_FTSZ, size);
		}
	}
}

bool Sokit::initUI()
{
	initTranslator();
	initFont();

	HelpForm* h = new HelpForm(&m_wnd, Qt::WindowCloseButtonHint);

	QShortcut* k = new QShortcut(QKeySequence(Qt::Key_F1), &m_wnd);
	QShortcut* t = new QShortcut(QKeySequence(Qt::Key_F10), &m_wnd);
    connect(k, SIGNAL(activated()), h, SLOT(exec()));
	connect(t, SIGNAL(activated()), this, SLOT(ontop()));

	m_wnd.setWindowTitle(translate("Sokit", "sokit -- F1 for help"));
	m_wnd.setWindowIcon(QIcon(":/sokit.png"));

	QWidget* pnl = new QWidget(&m_wnd);
	m_wnd.setCentralWidget(pnl);

	BaseForm* server = new ServerForm();
	BaseForm* transf = new TransferForm();
	BaseForm* client = new ClientForm();
	NotepadForm* npd = new NotepadForm();

	QTabWidget* tab = new QTabWidget(pnl);
	tab->addTab(server, server->windowTitle());
	tab->addTab(transf, transf->windowTitle());
	tab->addTab(client, client->windowTitle());
	tab->addTab(npd, npd->windowTitle());
	tab->setCurrentIndex(0);

	QLayout* lay = new QVBoxLayout(pnl);
	lay->setSpacing(5);
	lay->setContentsMargins(5, 5, 5, 5);
	lay->addWidget(tab);

	return server->init() && transf->init() &&
		client->init() && npd->init();
}

void Sokit::ontop()
{
	Qt::WindowFlags f = m_wnd.windowFlags();
	if (f & Qt::WindowStaysOnTopHint)
		f &= ~Qt::WindowStaysOnTopHint;
	else
		f |= Qt::WindowStaysOnTopHint;

	m_wnd.setWindowFlags(f);
	m_wnd.show();
}

int main(int argc, char *argv[])
{
	Sokit a(argc, argv);

	if (a.initUI())
		a.show();
	else
		a.close();
	
	return a.exec();
}
