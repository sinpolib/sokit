#ifndef __TRANSFERFORM_H__
#define __TRANSFERFORM_H__

#include "ui_transferform.h"
#include "baseform.h"

class TransferSkt;

class TransferForm : public BaseForm
{
	Q_OBJECT

public:
	TransferForm(QWidget* p = nullptr, Qt::WindowFlags f = Qt::WindowFlags(0));
	~TransferForm() override;

protected:
	bool initForm() override;
	bool initHotkeys() override;
	void initConfig() override;
	void saveConfig() override;
	void send(const QString& data, const QString& dir) override;
	void kill(QStringList& list) override;

private slots:
	void trigger(bool start);
	void stop();

private:
	TransferSkt* m_server;
	Ui::TransferForm m_ui;
};

#endif // __TRANSFERFORM_H__
