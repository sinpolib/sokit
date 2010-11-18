#ifndef __CLIENTFORM_H__
#define __CLIENTFORM_H__

#include "ui_clientform.h"
#include "baseform.h"

class ClientSkt;
class ClientForm : public BaseForm
{
	Q_OBJECT

public:
	ClientForm(QWidget* p=0, Qt::WFlags f=0);
	virtual ~ClientForm();

protected:
	virtual bool initForm();
	virtual bool initHotkeys();
	virtual void initConfig();
	virtual void saveConfig();
	virtual void send(const QString& data, const QString& dir);

private:
	bool plug(bool istcp);
	void unplug();

private slots:
	void trigger(bool checked);
	void unpluged();

private:
	ClientSkt* m_client;
	Ui::ClientForm m_ui;
};

#endif // __CLIENTFORM_H__


