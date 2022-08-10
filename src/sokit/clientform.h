#ifndef __CLIENTFORM_H__
#define __CLIENTFORM_H__

#include "ui_clientform.h"
#include "baseform.h"

class ClientSkt;

class ClientForm : public BaseForm
{
	Q_OBJECT

public:
	ClientForm(QWidget* p = nullptr, Qt::WindowFlags f = Qt::WindowFlags(0));
	~ClientForm() override;

protected:
	bool initForm() override;
	bool initHotkeys() override;
	void initConfig() override;
	void saveConfig() override;
	void send(const QString& data, const QString& dir) override;

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
