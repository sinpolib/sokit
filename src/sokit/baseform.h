#ifndef __BASEFORM_H__
#define __BASEFORM_H__

#include <QMutex>
#include "logger.h"
#include <QToolButton>
#include <QComboBox>

class QLabel;
class QListWidget;

class BaseForm : public QWidget
{
	Q_OBJECT

public:
	BaseForm(QWidget* p = nullptr, Qt::WindowFlags f = Qt::WindowFlags(0));
	~BaseForm() override;

	bool init();

protected:
	bool lock()
	{
		m_door.lock();
		return true;
	};
	bool lock(qint32 w) { return m_door.tryLock(w); };
	void unlock() { m_door.unlock(); };

	void initCounter(QLabel* r, QLabel* s);
	void initLogger(QCheckBox* w, QToolButton* c, QTreeWidget* o, QPlainTextEdit* d);
	void initLister(QToolButton* a, QToolButton* k, QListWidget* l);
	void bindBuffer(qint32 id, QLineEdit* e, QToolButton* s, QComboBox* d);
	void bindFocus(QWidget* w, qint32 k);
	void bindClick(QAbstractButton* b, qint32 k);
	void bindSelect(QComboBox* b, qint32 i, qint32 k);

	void listerSelected(QStringList& output);

	virtual bool initForm() =0;
	virtual bool initHotkeys() =0;
	virtual void initConfig() =0;
	virtual void saveConfig() =0;

	virtual void kill(QStringList& /*list*/)
	{
	};
	virtual void send(const QString& data, const QString& dir) =0;

signals:
	void output(const QString& info);
	void output(const QString& title, const char* buf, quint32 len);

protected slots:
	void send();
	void kill();
	void clear();
	void focus();
	void select();
	void hotOutput();

	void countRecv(qint32 bytes);
	void countSend(qint32 bytes);

	void listerAdd(const QString& caption);
	void listerRemove(const QString& caption);

private:
	QMutex m_door;
	Logger m_logger;

	quint32 m_cntRecv;
	quint32 m_cntSend;

	QLabel* m_labRecv;
	QLabel* m_labSend;

	QListWidget* m_cnlist;
};

#endif // __BASEFORM_H__
