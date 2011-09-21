#include "toolkit.h"
#include "setting.h"
#include "helpform.h"

#include <QShortcut>
#include <QTextStream>

HelpForm::HelpForm(QWidget* p, Qt::WFlags f):QDialog(p, f)
{
	m_ui.setupUi(this);
	init();
}

HelpForm::~HelpForm()
{
}

void HelpForm::init()
{
	QShortcut* k = new QShortcut(QKeySequence(Qt::Key_F1), this);
	connect(k, SIGNAL(activated()), this, SLOT(close()));

}



