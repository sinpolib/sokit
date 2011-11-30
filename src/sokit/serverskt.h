#ifndef __SERVERSKT_H__
#define __SERVERSKT_H__

#include <QHash>
#include <QTcpServer>
#include <QUdpSocket>
#include <QDateTime>
#include <QTimer>

class ServerSkt : public QObject
{
	typedef QHash<QString, void*> OBJMAP;

	Q_OBJECT

public:
	ServerSkt(QObject *parent=0);
	virtual ~ServerSkt();

	virtual QString name() const { return "General"; };

	bool start(const QHostAddress& ip, quint16 port);
	void kill(const QString& key);
	void stop();

	void send(const QString& key, const QString& data);

	const QHostAddress& addr() const { return m_ip; };
	quint16 port() const { return m_port; };

signals:
	void connOpen(const QString& key);
	void connClose(const QString& key);
	void message(const QString& msg);
	void dumpbin(const QString& title, const char* data, quint32 len);

	void countRecv(qint32 bytes);
	void countSend(qint32 bytes);

protected:
	void dump(const char* buf, qint32 len, bool isSend, const QString& key);
	void show(const QString& msg);

	void setError(const QString& err);

	void recordRecv(qint32 bytes);
	void recordSend(qint32 bytes);

	void getKeys(QStringList& res);
	void setCookie(const QString& k, void* v);
	void unsetCookie(const QString& k);
	void* getCookie(const QString& k);
	

	virtual bool open() =0;
	virtual bool close(void* cookie) =0;
	virtual void send(void* cookie, const QByteArray& bin) =0;
	virtual void close() =0;

private:
	bool m_started;
	QHostAddress m_ip;
	quint16 m_port;

	OBJMAP m_conns;
	QString m_error;
};

class ServerSktTcp : public ServerSkt
{
	typedef struct _Conn
	{
		QTcpSocket* client;
		QString key;
	} Conn;

	Q_OBJECT

public:
	ServerSktTcp(QObject *parent=0);
	virtual ~ServerSktTcp();

	virtual QString name() const { return "TCP"; };

protected:
	virtual bool open();
	virtual bool close(void* cookie);
	virtual void send(void* cookie, const QByteArray& bin);
	virtual void close();

private slots:
	void newConnection();
	void newData();
	void error();
	void close(QObject* obj);

private:
	QTcpServer m_server;
};

class ServerSktUdp : public ServerSkt
{
	typedef struct _Conn
	{
		QHostAddress addr;
		quint16 port;
		QDateTime stamp;
		QString key;
	} Conn;

	Q_OBJECT

public:
	ServerSktUdp(QObject *parent=0);
	virtual ~ServerSktUdp();

	virtual QString name() const { return "UDP"; };

protected:
	virtual bool open();
	virtual bool close(void* cookie);
	virtual void send(void* cookie, const QByteArray& bin);
	virtual void close();

private slots:
	void newData();
	void error();
	void check();

private:
	QUdpSocket m_server;
	QTimer m_timer;
};

#endif // __SERVERSKT_H__

