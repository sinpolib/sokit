#ifndef __LOGGER_H__
#define __LOGGER_H__

#include <QFile>
#include <QTreeView>
#include <QCheckBox>
#include <QMenu>

class QTreeWidget;
class QTreeWidgetItem;
class QPlainTextEdit;

class Logger : public QObject
{
	Q_OBJECT

public:
	Logger(QObject* parent = nullptr);
	~Logger() override;

	void init(QTreeWidget* o, QCheckBox* w, QPlainTextEdit* d);

	void clear();
	void output(const QString& title, const QString& info);
	void output(const char* buf, quint32 len);

signals:
	void clearLog();

public slots:
	void output(const QString& info);
	void output(const QString& title, const char* buf, quint32 len);

private slots:
	void ctxmenu(const QPoint& pos);
	void copy();
	void syncOutput();

private:
	const QString getLogFileName();
	void writeLogFile(const QString& info);
	void pack();
	QTreeWidgetItem* appendLogEntry(QTreeWidgetItem* p, const QString& t);

private:
	QString m_dir;
	QFile m_file;

	QMenu m_cmlog, m_cmtxt;
	QCheckBox* m_chkWrite;
	QTreeWidget* m_treeOut;
	QPlainTextEdit* m_textOut;
};

#endif // __LOGGER_H__
