#ifndef __MAIN_H__
#define __MAIN_H__

#include <QTranslator>
#include <QApplication>
#include <QMainWindow>

class Sokit : public QApplication
{
	Q_OBJECT

public:
	Sokit(int& argc, char** argv);
	~Sokit() override;

	bool initTranslator();
	bool initUI();
	void show();
	void close();

private slots:
	void ontop();

private:
	void initFont();
	void initDefaultActionsName();

private:
	QMainWindow m_wnd;
	QTranslator m_trans;
};

#endif //__MAIN_H__
