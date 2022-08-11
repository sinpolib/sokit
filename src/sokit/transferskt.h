#ifndef __TRANSFERSKT_H__
#define __TRANSFERSKT_H__

#include <QHash>
#include <QTcpServer>
#include <QUdpSocket>
#include <QDateTime>
#include <QTimer>
#include <QMutex>

class TransferSkt : public QObject
{
	using OBJMAP = QHash<QString, void*>;

	Q_OBJECT

public:
	TransferSkt(QObject* parent = nullptr);
	~TransferSkt() override;

	virtual QString name() const { return "General"; };

	bool start(const QHostAddress& sip, quint16 spt, const QHostAddress& dip, quint16 dpt);
	void kill(const QString& key);
	void stop();

	void send(const QString& key, bool s2d, const QString& data);

	const QHostAddress& srcAddr() const { return m_sip; };
	const QHostAddress& dstAddr() const { return m_dip; };

	quint16 srcPort() const { return m_spt; };
	quint16 dstPort() const { return m_dpt; };

signals:
	void connOpen(const QString& key);
	void connClose(const QString& key);
	void message(const QString& msg);
	void dumpbin(const QString& title, const char* data, quint32 len);
	void stopped();

	void countRecv(qint32 bytes);
	void countSend(qint32 bytes);

protected:
	enum DIR { TS2D, TD2S, SS2D, SD2S, };

	void dump(const char* buf, qint32 len, DIR dir, const QString& key);
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
	virtual void send(void* cookie, bool s2d, const QByteArray& bin) =0;
	virtual void close() =0;

private:
	QHostAddress m_sip;
	QHostAddress m_dip;

	quint16 m_spt;
	quint16 m_dpt;

	OBJMAP m_conns;
	QString m_error;
};

class TransferSktTcp : public TransferSkt
{
	using Conn = struct _Conn
	{
		QTcpSocket* src;
		QTcpSocket* dst;
		QString key;
	};

	Q_OBJECT

public:
	TransferSktTcp(QObject* parent = nullptr);
	~TransferSktTcp() override;

	QString name() const override { return "TCP"; };

protected:
	bool open() override;
	bool close(void* cookie) override;
	void send(void* cookie, bool s2d, const QByteArray& bin) override;
	void close() override;

private slots:
	void newConnection();
	void asynConnection();
	void newData();
	void error();
	void close(QObject* obj);

private:
	QTcpServer m_server;
	QMutex m_door;
};

class TransferSktUdp : public TransferSkt
{
	using Conn = struct _Conn
	{
		QUdpSocket* dst;
		QHostAddress addr;
		quint16 port;
		QDateTime stamp;
		QString key;
	};

	Q_OBJECT

public:
	TransferSktUdp(QObject* parent = nullptr);
	~TransferSktUdp() override;

	QString name() const override { return "UDP"; };

protected:
	bool open() override;
	bool close(void* cookie) override;
	void send(void* cookie, bool s2d, const QByteArray& bin) override;
	void close() override;

private slots:
	void newData();
	void error();
	void check();
	void close(QObject* obj);

private:
	QUdpSocket m_server;
	QTimer m_timer;
};

#endif // __TRANSFERSKT_H__
