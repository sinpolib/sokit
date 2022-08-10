#ifndef __HELPFORM_H__
#define __HELPFORM_H__

#include "ui_helpform.h"

class HelpForm : public QDialog
{
	Q_OBJECT

public:
	HelpForm(QWidget* p = nullptr, Qt::WindowFlags f = Qt::WindowFlags(0));
	~HelpForm() override;

private:
	void init();

private:
	Ui::HelpForm m_ui;
};

#endif // __HELPFORM_H__
