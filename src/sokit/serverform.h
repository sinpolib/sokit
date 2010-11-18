#ifndef __SERVERFORM_H__
#define __SERVERFORM_H__

#include "ui_serverform.h"
#include "baseform.h"
#include "serverskt.h"

class ServerForm : public BaseForm
{
	Q_OBJECT

public:
	ServerForm(QWidget* p=0, Qt::WFlags f=0);
	virtual ~ServerForm();

protected:
	virtual bool initForm();
	virtual bool initHotkeys();
	virtual void initConfig();
	virtual void saveConfig();
	virtual void send(const QString& data, const QString& dir);
	virtual void kill(QStringList& list);

private slots:
	void trigger(bool start);

private:
	ServerSktTcp m_tcp;
	ServerSktUdp m_udp;
	Ui::ServerForm m_ui;
};

#endif // __SERVERFORM_H__



