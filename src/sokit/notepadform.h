#ifndef __NOTEPADFORM_H__
#define __NOTEPADFORM_H__

#include <QPlainTextEdit>

class NotepadForm : public QWidget
{
	Q_OBJECT

public:
	NotepadForm(QWidget* p=0, Qt::WFlags f=0);
	virtual ~NotepadForm();

public:
	bool init();

private slots:
	void jumptab();

private:
	void setupUi();
	void uninit();

private:
    QPlainTextEdit* m_board;
};

#endif // __NOTEPADFORM_H__




