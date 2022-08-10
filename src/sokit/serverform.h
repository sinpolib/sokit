#ifndef __SERVERFORM_H__
#define __SERVERFORM_H__

#include "ui_serverform.h"
#include "baseform.h"
#include "serverskt.h"

class ServerForm : public BaseForm
{
	Q_OBJECT

public:
	ServerForm(QWidget* p = nullptr, Qt::WindowFlags f = Qt::WindowFlags(0));
	~ServerForm() override;

protected:
	bool initForm() override;
	bool initHotkeys() override;
	void initConfig() override;
	void saveConfig() override;
	void send(const QString& data, const QString& dir) override;
	void kill(QStringList& list) override;

private slots:
	void trigger(bool start);

private:
	ServerSktTcp m_tcp;
	ServerSktUdp m_udp;
	Ui::ServerForm m_ui;
};

#endif // __SERVERFORM_H__
