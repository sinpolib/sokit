#include <QTcpSocket>
#include <QStringList>
#include <QMutexLocker>
#include "toolkit.h"
#include "transferskt.h"

#define PROP_CONN  "CONN"

#define MAXBUFFER 1024*1024

TransferSkt::TransferSkt(QObject* parent)
	: QObject(parent), m_spt(0), m_dpt(0)
{
}

TransferSkt::~TransferSkt()
{
}

bool TransferSkt::start(const QHostAddress& sip, quint16 spt, const QHostAddress& dip, quint16 dpt)
{
	m_sip = sip;
	m_dip = dip;
	m_spt = spt;
	m_dpt = dpt;

	m_conns.clear();
	m_error.clear();

	bool res = open();

	QString msg("start %1 transfer server %2!");
	if (!res)
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

	return res;
}

void TransferSkt::stop()
{
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

	show(QString("stop %1 transfer server!").arg(name()));
}

void TransferSkt::setError(const QString& err)
{
	m_error = err;
}

void TransferSkt::recordRecv(qint32 bytes)
{
	emit countRecv(bytes);
}

void TransferSkt::recordSend(qint32 bytes)
{
	emit countSend(bytes);
}

void TransferSkt::getKeys(QStringList& res)
{
	res = m_conns.keys();
}

void TransferSkt::setCookie(const QString& k, void* v)
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

void TransferSkt::unsetCookie(const QString& k)
{
	m_conns.remove(k);
	emit connClose(k);
}

void* TransferSkt::getCookie(const QString& k)
{
	return m_conns.value(k);
}

void TransferSkt::kill(const QString& key)
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

void TransferSkt::send(const QString& key, bool s2d, const QString& data)
{
	void* v = m_conns.value(key);
	if (v)
	{
		QString err;
		QByteArray bin;

		if (!TK::ascii2bin(data, bin, err))
			show("bad data format to send: " + err);
		else
			send(v, s2d, bin);
	}
}

void TransferSkt::dump(const char* buf, qint32 len, DIR dir, const QString& key)
{
	QString title("TRN");
	switch (dir)
	{
	case TS2D: title += " -->> ";
		break;
	case TD2S: title += " <<-- ";
		break;
	case SS2D: title += " --+> ";
		break;
	case SD2S: title += " <+-- ";
		break;
	default: title += " ???? ";
		break;
	}
	title += key;

	emit dumpbin(title, buf, static_cast<quint32>(len));
}

void TransferSkt::show(const QString& msg)
{
	emit message(msg);
}

TransferSktTcp::TransferSktTcp(QObject* parent)
	: TransferSkt(parent)
{
}

TransferSktTcp::~TransferSktTcp()
{
	m_server.disconnect(this);
}

bool TransferSktTcp::open()
{
	if (m_server.listen(srcAddr(), srcPort()))
	{
		connect(&m_server, SIGNAL(newConnection()), this, SLOT(newConnection()));

		return true;
	}
	setError(QString("%1, %2").arg(m_server.serverError()).arg(m_server.errorString()));

	return false;
}

bool TransferSktTcp::close(void* cookie)
{
	auto conn = static_cast<Conn*>(cookie);

	if (conn->src)
		conn->src->disconnect(this);

	if (conn->dst)
		conn->dst->disconnect(this);

	delete conn->src;
	delete conn->dst;
	delete conn;

	return true;
}

void TransferSktTcp::close(QObject* obj)
{
	QMutexLocker locker(&m_door);

	auto conn = static_cast<Conn*>(obj->property(PROP_CONN).value<void*>());
	if (!conn) return;

	if (conn->src)
	{
		conn->src->disconnect(this);

		if (obj == conn->dst)
			conn->src->deleteLater();
	}

	if (conn->dst)
	{
		conn->dst->disconnect(this);

		if (obj == conn->src)
			conn->dst->deleteLater();
	}

	unsetCookie(conn->key);
	delete conn;
}

void TransferSktTcp::error()
{
	auto s = qobject_cast<QTcpSocket*>(sender());

	show(QString("TCP socket error %1, %2").arg(s->error()).arg(s->errorString()));

	s->deleteLater();
}

void TransferSktTcp::close()
{
	m_server.close();
	m_server.disconnect(this);
}

void TransferSktTcp::newConnection()
{
	auto svr = qobject_cast<QTcpServer*>(sender());
	if (!svr) return;

	QTcpSocket* src = svr->nextPendingConnection();
	while (src)
	{
		auto conn = new Conn;
		if (!conn)
		{
			src->deleteLater();
		}
		else
		{
			auto dst = new QTcpSocket();
			if (!dst)
			{
				delete conn;
				src->deleteLater();
			}
			else
			{
				src->setProperty(PROP_CONN, QVariant::fromValue(static_cast<void*>(conn)));
				dst->setProperty(PROP_CONN, QVariant::fromValue(static_cast<void*>(conn)));

				conn->src = src;
				conn->dst = dst;
				conn->key = TK::ipstr(src->peerAddress(), src->peerPort());

				connect(src, SIGNAL(readyRead()), this, SLOT(newData()));
				connect(src, SIGNAL(destroyed(QObject*)), this, SLOT(close(QObject*)));
				connect(src, SIGNAL(disconnected()), src, SLOT(deleteLater()));
				connect(src, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(error()));

				connect(dst, SIGNAL(readyRead()), this, SLOT(newData()));
				connect(dst, SIGNAL(destroyed(QObject*)), this, SLOT(close(QObject*)));
				connect(dst, SIGNAL(disconnected()), dst, SLOT(deleteLater()));
				connect(dst, SIGNAL(connected()), this, SLOT(asynConnection()));
				connect(dst, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(error()));

				dst->connectToHost(dstAddr(), dstPort());

				setCookie(conn->key, conn);
			}
		}
		src = svr->nextPendingConnection();
	}
}

void TransferSktTcp::asynConnection()
{
	auto s = qobject_cast<QTcpSocket*>(sender());
	if (!s) return;

	auto conn = static_cast<Conn*>(s->property(PROP_CONN).value<void*>());
	if (!conn) return;

	show(QString("connection %1 to %2:%3 opened!")
	     .arg(conn->key, s->peerName()).arg(s->peerPort()));
}

void TransferSktTcp::newData()
{
	QMutexLocker locker(&m_door);

	auto s = qobject_cast<QTcpSocket*>(sender());
	if (!s) return;

	auto conn = static_cast<Conn*>(s->property(PROP_CONN).value<void*>());
	if (!conn) return;

	QTcpSocket* d = (s == conn->src) ? conn->dst : conn->src;

	qint64 bufLen = s->bytesAvailable();
	char* buf = TK::createBuffer(bufLen, MAXBUFFER);
	if (!buf) return;

	qint64 readLen, writeLen, ioLen;

	readLen = 0;
	ioLen = s->read(buf, bufLen);
	while (ioLen > 0)
	{
		readLen += ioLen;
		ioLen = s->read(buf + readLen, bufLen - readLen);
	}

	if (ioLen >= 0)
	{
		recordRecv(readLen);

		writeLen = 0;
		ioLen = d->write(buf, readLen);
		while (ioLen > 0)
		{
			writeLen += ioLen;
			ioLen = d->write(buf + writeLen, readLen - writeLen);
		}

		if (ioLen >= 0)
		{
			recordSend(writeLen);
			dump(buf, readLen, ((s == conn->src) ? TS2D : TD2S), conn->key);
		}
	}

	TK::releaseBuffer(buf);
}

void TransferSktTcp::send(void* cookie, bool s2d, const QByteArray& bin)
{
	auto conn = static_cast<Conn*>(cookie);

	QTcpSocket* d = s2d ? conn->dst : conn->src;
	QHostAddress a = s2d ? dstAddr() : conn->src->peerAddress();
	quint16 p = s2d ? dstPort() : conn->src->peerPort();

	const char* src = bin.constData();
	qint64 srcLen = bin.length();

	qint64 writeLen = 0;
	qint64 ioLen = d->write(src, srcLen);

	while (ioLen > 0)
	{
		writeLen += ioLen;
		ioLen = d->write(src + writeLen, srcLen - writeLen);
	}

	if (writeLen != srcLen)
	{
		show(QString("failed to send data to %1:%2 [%3]")
		     .arg(a.toString()).arg(p).arg(writeLen));
		return;
	}

	recordSend(writeLen);
	dump(src, srcLen, (s2d ? SS2D : SD2S), conn->key);
}

TransferSktUdp::TransferSktUdp(QObject* parent)
	: TransferSkt(parent)
{
}

TransferSktUdp::~TransferSktUdp()
{
	m_server.disconnect(this);
}

void TransferSktUdp::error()
{
	auto s = qobject_cast<QUdpSocket*>(sender());

	show(QString("UDP socket error %1, %2").arg(s->error()).arg(s->errorString()));

	if (s != &m_server)
		s->deleteLater();
}

bool TransferSktUdp::open()
{
	if (m_server.bind(srcAddr(), srcPort(), QUdpSocket::ShareAddress))
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

bool TransferSktUdp::close(void* cookie)
{
	auto conn = static_cast<Conn*>(cookie);

	if (conn->dst)
		conn->dst->disconnect(this);

	delete conn->dst;
	delete conn;

	return true;
}

void TransferSktUdp::close(QObject* obj)
{
	auto conn = static_cast<Conn*>(obj->property(PROP_CONN).value<void*>());
	if (!conn) return;

	unsetCookie(conn->key);
	delete conn;
}

void TransferSktUdp::close()
{
	m_timer.disconnect(this);
	m_timer.stop();
	m_server.close();
	m_server.disconnect(this);
}

void TransferSktUdp::newData()
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
		Conn* conn = nullptr;
		if (s == &m_server)
		{
			conn = static_cast<Conn*>(getCookie(TK::ipstr(addr, port)));
			if (!conn)
			{
				conn = new Conn;
				if (conn)
				{
					auto dst = new QUdpSocket();
					if (!dst)
					{
						delete conn;
						conn = nullptr;
					}
					else
					{
						dst->setProperty(PROP_CONN, QVariant::fromValue(static_cast<void*>(conn)));

						conn->dst = dst;
						conn->key = TK::ipstr(addr, port);
						conn->addr = addr;
						conn->port = port;

						connect(dst, SIGNAL(readyRead()), this, SLOT(newData()));
						connect(dst, SIGNAL(destroyed(QObject*)), this, SLOT(close(QObject*)));
						connect(dst, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(error()));

						dst->connectToHost(dstAddr(), dstPort());

						setCookie(conn->key, conn);
					}
				}
			}
		}
		else
		{
			conn = static_cast<Conn*>(s->property(PROP_CONN).value<void*>());
			if (!conn)
			{
				s->disconnect(this);
				s->deleteLater();
			}
		}

		if (conn)
		{
			recordRecv(readLen);

			conn->stamp = QDateTime::currentDateTime();

			qint64 writeLen = 0;
			if (s == &m_server)
			{
				ioLen = conn->dst->write(buf, readLen);
				while (ioLen > 0)
				{
					writeLen += ioLen;
					ioLen = (writeLen >= readLen) ? 0 : conn->dst->write(buf + writeLen, readLen - writeLen);
				}

				dump(buf, readLen, TS2D, conn->key);
			}
			else
			{
				ioLen = m_server.writeDatagram(buf, readLen, conn->addr, conn->port);
				while (ioLen > 0)
				{
					writeLen += ioLen;
					ioLen = (writeLen >= readLen)
						        ? 0
						        : m_server.writeDatagram(buf + writeLen, readLen - writeLen, conn->addr, conn->port);
				}

				dump(buf, readLen, TD2S, conn->key);
			}

			recordSend(writeLen);
		}
	}

	TK::releaseBuffer(buf);
}

void TransferSktUdp::send(void* cookie, bool s2d, const QByteArray& bin)
{
	auto conn = static_cast<Conn*>(cookie);

	QHostAddress a = s2d ? dstAddr() : conn->addr;
	quint16 p = s2d ? dstPort() : conn->port;

	const char* src = bin.constData();
	qint64 srcLen = bin.length();

	qint64 writeLen = 0;
	qint64 ioLen = s2d ? conn->dst->write(src, srcLen) : m_server.writeDatagram(src, srcLen, a, p);

	while (ioLen > 0)
	{
		writeLen += ioLen;

		if (writeLen >= srcLen)
			break;

		ioLen = s2d
			        ? conn->dst->write(src + writeLen, srcLen - writeLen)
			        : m_server.writeDatagram(src + writeLen, srcLen - writeLen, a, p);
	}

	if (writeLen != srcLen)
	{
		show(QString("failed to send data to %1:%2 [%3]")
		     .arg(a.toString()).arg(p).arg(writeLen));
		return;
	}

	recordSend(writeLen);
	dump(src, srcLen, (s2d ? SS2D : SD2S), conn->key);
}

void TransferSktUdp::check()
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
