#include <QTcpSocket>
#include <QStringList>
#include "toolkit.h"
#include "serverskt.h"

#define PROP_CONN  "CONN"

#define MAXBUFFER 1024*1024

ServerSkt::ServerSkt(QObject* parent)
	: QObject(parent), m_port(0)
{
	m_started = false;
}

ServerSkt::~ServerSkt()
{
}

bool ServerSkt::start(const QHostAddress& ip, quint16 port)
{
	m_ip = ip;
	m_port = port;

	m_conns.clear();
	m_error.clear();

	m_started = open();

	QString msg("start %1 server %2!");
	if (!m_started)
	{
		msg = msg.arg(name(), "failed");
		if (!m_error.isEmpty())
		{
			msg += " error:[";
			msg += m_error;
			msg += "].";
		}
	}
	else
	{
		msg = msg.arg(name(), "successfully");
	}

	show(msg);

	return m_started;
}

void ServerSkt::stop()
{
	if (!m_started)
		return;

	OBJMAP::const_iterator i;
	for (i = m_conns.constBegin(); i != m_conns.constEnd(); ++i)
	{
		QString k = i.key();
		void* v = i.value();

		if (close(v))
			emit connClose(k);
	}

	m_conns.clear();

	close();

	show(QString("stop %1 server!").arg(name()));

	m_started = false;
}

void ServerSkt::setError(const QString& err)
{
	m_error = err;
}

void ServerSkt::recordRecv(qint32 bytes)
{
	emit countRecv(bytes);
}

void ServerSkt::recordSend(qint32 bytes)
{
	emit countSend(bytes);
}

void ServerSkt::getKeys(QStringList& res)
{
	res = m_conns.keys();
}

void ServerSkt::setCookie(const QString& k, void* v)
{
	void* o = m_conns.value(k);
	if (o)
	{
		if (close(o))
			emit connClose(k);
	}

	m_conns.insert(k, v);
	emit connOpen(k);
}

void ServerSkt::unsetCookie(const QString& k)
{
	m_conns.remove(k);
	emit connClose(k);
}

void* ServerSkt::getCookie(const QString& k)
{
	return m_conns.value(k);
}

void ServerSkt::kill(const QString& key)
{
	void* v = m_conns.value(key);
	if (v)
	{
		if (close(v))
			unsetCookie(key);
	}
	else
	{
		unsetCookie(key);
	}
}

void ServerSkt::send(const QString& key, const QString& data)
{
	void* v = m_conns.value(key);
	if (v)
	{
		QString err;
		QByteArray bin;

		if (!TK::ascii2bin(data, bin, err))
			show("bad data format to send: " + err);
		else
			send(v, bin);
	}
}

void ServerSkt::dump(const char* buf, qint32 len, bool isSend, const QString& key)
{
	emit dumpbin(QString("DAT %1 %2").arg(isSend ? "<---" : "--->", key), buf, static_cast<quint32>(len));
}

void ServerSkt::show(const QString& msg)
{
	emit message(msg);
}

ServerSktTcp::ServerSktTcp(QObject* parent)
	: ServerSkt(parent)
{
}

ServerSktTcp::~ServerSktTcp()
{
	m_server.disconnect(this);
	stop();
}

bool ServerSktTcp::open()
{
	if (m_server.listen(addr(), port()))
	{
		connect(&m_server, SIGNAL(newConnection()), this, SLOT(newConnection()));
		return true;
	}
	setError(QString("%1, %2").arg(m_server.serverError()).arg(m_server.errorString()));

	return false;
}

bool ServerSktTcp::close(void* cookie)
{
	auto conn = static_cast<Conn*>(cookie);

	if (conn->client)
		conn->client->disconnect(this);

	delete conn->client;
	delete conn;

	return true;
}

void ServerSktTcp::close(QObject* obj)
{
	auto conn = static_cast<Conn*>(obj->property(PROP_CONN).value<void*>());
	if (!conn) return;

	unsetCookie(conn->key);
	delete conn;
}

void ServerSktTcp::error()
{
	auto s = qobject_cast<QTcpSocket*>(sender());

	show(QString("TCP socket error %1, %2").arg(s->error()).arg(s->errorString()));

	s->deleteLater();
}

void ServerSktTcp::close()
{
	m_server.close();
	m_server.disconnect(this);
}

void ServerSktTcp::newConnection()
{
	auto server = qobject_cast<QTcpServer*>(sender());
	if (!server) return;

	QTcpSocket* client = server->nextPendingConnection();
	while (client)
	{
		auto conn = new Conn;
		if (!conn)
		{
			client->deleteLater();
		}
		else
		{
			client->setProperty(PROP_CONN, QVariant::fromValue(static_cast<void*>(conn)));

			conn->client = client;
			conn->key = TK::ipstr(client->peerAddress(), client->peerPort(), true);

			connect(client, SIGNAL(readyRead()), this, SLOT(newData()));
			connect(client, SIGNAL(destroyed(QObject*)), this, SLOT(close(QObject*)));
			connect(client, SIGNAL(disconnected()), client, SLOT(deleteLater()));
			connect(client, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(error()));

			setCookie(conn->key, conn);
		}
		client = server->nextPendingConnection();
	}
}

void ServerSktTcp::newData()
{
	auto client = qobject_cast<QTcpSocket*>(sender());
	if (!client) return;

	auto conn = static_cast<Conn*>(client->property(PROP_CONN).value<void*>());
	if (!conn) return;

	qint64 bufLen = client->bytesAvailable();
	char* buf = TK::createBuffer(bufLen, MAXBUFFER);
	if (!buf) return;

	qint64 readLen = 0;
	qint64 ioLen = client->read(buf, bufLen);

	while (ioLen > 0)
	{
		readLen += ioLen;
		ioLen = client->read(buf + readLen, bufLen - readLen);
	}

	if (ioLen >= 0)
	{
		recordRecv(readLen);
		dump(buf, readLen, false, conn->key);
	}

	TK::releaseBuffer(buf);
}

void ServerSktTcp::send(void* cookie, const QByteArray& bin)
{
	auto conn = static_cast<Conn*>(cookie);

	const char* src = bin.constData();
	qint64 srcLen = bin.length();

	qint64 writeLen = 0;
	qint64 ioLen = conn->client->write(src, srcLen);

	while (ioLen > 0)
	{
		writeLen += ioLen;
		ioLen = conn->client->write(src + writeLen, srcLen - writeLen);
	}

	if (writeLen != srcLen)
	{
		show(QString("failed to send data to %1:%2 [%3]")
		     .arg(addr().toString()).arg(port()).arg(writeLen));
		return;
	}

	recordSend(writeLen);
	dump(src, srcLen, true, conn->key);
}

ServerSktUdp::ServerSktUdp(QObject* parent)
	: ServerSkt(parent)
{
}

ServerSktUdp::~ServerSktUdp()
{
	m_server.disconnect(this);
	stop();
}

void ServerSktUdp::error()
{
	auto s = qobject_cast<QUdpSocket*>(sender());

	show(QString("UDP socket error %1, %2").arg(s->error()).arg(s->errorString()));
}

bool ServerSktUdp::open()
{
	if (m_server.bind(addr(), port(), QUdpSocket::ShareAddress))
	{
		connect(&m_server, SIGNAL(readyRead()), this, SLOT(newData()));
		connect(&m_server, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(error()));
		connect(&m_timer, SIGNAL(timeout()), this, SLOT(check()));

		m_timer.start(2000);
		return true;
	}
	setError(QString("%1, %2").arg(m_server.error()).arg(m_server.errorString()));

	return false;
}

bool ServerSktUdp::close(void* cookie)
{
	delete static_cast<Conn*>(cookie);
	return true;
}

void ServerSktUdp::close()
{
	m_timer.disconnect(this);
	m_timer.stop();
	m_server.close();
	m_server.disconnect(this);
}

void ServerSktUdp::newData()
{
	auto s = qobject_cast<QUdpSocket*>(sender());
	if (!s) return;

	qint64 bufLen = s->pendingDatagramSize();
	char* buf = TK::createBuffer(bufLen, MAXBUFFER);
	if (!buf) return;

	QHostAddress addr;
	quint16 port(0);

	qint64 readLen = 0;
	qint64 ioLen = s->readDatagram(buf, bufLen, &addr, &port);

	//while (ioLen > 0)
	//{
	readLen += ioLen;
	//	ioLen = s->readDatagram(buf+readLen, bufLen-readLen, &addr, &port);
	//}

	if (ioLen >= 0)
	{
		auto conn = static_cast<Conn*>(getCookie(TK::ipstr(addr, port, false)));
		if (!conn)
		{
			conn = new Conn;
			if (conn)
			{
				conn->key = TK::ipstr(addr, port, false);
				conn->addr = addr;
				conn->port = port;
				setCookie(conn->key, conn);
			}
		}

		if (conn)
		{
			recordRecv(readLen);

			conn->stamp = QDateTime::currentDateTime();
			dump(buf, readLen, false, conn->key);
		}
	}

	TK::releaseBuffer(buf);
}

void ServerSktUdp::send(void* cookie, const QByteArray& bin)
{
	auto conn = static_cast<Conn*>(cookie);

	const char* src = bin.constData();
	qint64 srcLen = bin.length();

	qint64 writeLen = 0;
	qint64 ioLen = m_server.writeDatagram(src, srcLen, conn->addr, conn->port);

	while (ioLen > 0)
	{
		writeLen += ioLen;

		ioLen = (writeLen >= srcLen)
			        ? 0
			        : m_server.writeDatagram(src + writeLen, srcLen - writeLen, conn->addr, conn->port);
	}

	if (writeLen != srcLen)
	{
		show(QString("failed to send data to %1:%2 [%3]")
		     .arg(conn->addr.toString()).arg(conn->port).arg(writeLen));
		return;
	}

	recordSend(writeLen);
	dump(src, srcLen, true, conn->key);
}

void ServerSktUdp::check()
{
	QStringList list;
	getKeys(list);

	while (!list.isEmpty())
	{
		QString k = list.takeFirst();

		void* c = getCookie(k);

		if (c && (static_cast<Conn*>(c)->stamp.addSecs(120) < QDateTime::currentDateTime()))
		{
			close(c);
			unsetCookie(k);
		}
	}
}
