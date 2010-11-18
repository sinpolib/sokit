#include <QTcpSocket>
#include "toolkit.h"
#include "clientskt.h"

#define MAXBUFFER 1024*1024

ClientSkt::ClientSkt(QObject *parent)
: QObject(parent),m_port(0),m_recv(0),m_send(0)
{
}

ClientSkt::~ClientSkt()
{
}

bool ClientSkt::plug(const QHostAddress& ip, quint16 port)
{
	m_ip   = ip;
	m_port = port;
	m_recv = 0;
	m_send = 0;

	m_error.clear();

	return open();
}

void ClientSkt::unplug()
{
	m_recv = m_send = 0;
	emit countRecv(m_recv);
	emit countSend(m_send);

	close();

	emit unpluged();
}

void ClientSkt::setError(const QString& err)
{
	m_error = err;
}

void ClientSkt::recordRecv(quint32 bytes)
{
	m_recv += bytes;
	emit countRecv(m_recv);
}

void ClientSkt::recordSend(quint32 bytes)
{
	m_send += bytes;
	emit countSend(m_send);
}

void ClientSkt::send(const QString& data)
{
	QString err;
	QByteArray bin;

	if (!TK::ascii2bin(data, bin, err))
	{
		show("bad data format to send: "+err);
		return;
	}

	send(bin);
}

void ClientSkt::dump(const char* buf, quint32 len, bool isSend)
{
	emit dumpbin(QString("DAT %1").arg(isSend?"<<<<":">>>>"), buf, len);
}

void ClientSkt::show(const QString& msg)
{
	emit message(msg);
}

ClientSktTcp::ClientSktTcp(QObject *parent)
:ClientSkt(parent)
{
}

ClientSktTcp::~ClientSktTcp()
{
}

bool ClientSktTcp::open()
{
	connect(&m_socket, SIGNAL(readyRead()), this, SLOT(newData()));
	connect(&m_socket, SIGNAL(disconnected()), this, SLOT(closed()));
	connect(&m_socket, SIGNAL(connected()), this, SLOT(asynConn()));
	connect(&m_socket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(error()));

	m_socket.connectToHost(addr(), port());

	return true;
}

void ClientSktTcp::close()
{
	m_socket.close();
	m_socket.disconnect(this);
}

void ClientSktTcp::error()
{
	QTcpSocket* s = qobject_cast<QTcpSocket*>(sender());

	show(QString("TCP socket error %1, %2").arg(s->error()).arg(s->errorString()));

	unplug();
}

void ClientSktTcp::asynConn()
{
	show(QString("TCP connection to %1:%2 opened!")
		.arg(addr().toString()).arg(port()));
}

void ClientSktTcp::closed()
{
	show(QString("TCP connection closed!"));
}

void ClientSktTcp::newData()
{
	QTcpSocket* s = qobject_cast<QTcpSocket*>(sender());
	if (!s) return;

	qint64 bufLen = s->bytesAvailable();
	char* buf = TK::createBuffer(bufLen, MAXBUFFER);
	if (!buf) return;

	qint64 readLen = 0;
	qint64 ioLen = s->read(buf, bufLen);

	while (ioLen > 0)
	{
		readLen += ioLen;
		ioLen = s->read(buf+readLen, bufLen-readLen);
	}

	if (ioLen >= 0)
	{
		recordRecv((quint32)readLen);
		dump(buf, (quint32)readLen, false);
	}

	TK::releaseBuffer(buf);
}

void ClientSktTcp::send(const QByteArray& bin)
{
	const char *  src = bin.constData(); 
	qint64 srcLen = bin.length();

	qint64 writeLen = 0;
	qint64 ioLen = m_socket.write(src, srcLen);

	while (ioLen > 0)
	{
		writeLen += ioLen;
		ioLen = m_socket.write(src+writeLen, srcLen-writeLen);
	}

	if (writeLen != srcLen)
	{
		show(QString("failed to send data to %1:%2 [%3]")
			.arg(addr().toString()).arg(port()).arg(writeLen));
		return;
	}

	recordSend((quint32)writeLen);
	dump(src, (quint32)srcLen, true);
}

ClientSktUdp::ClientSktUdp(QObject *parent)
:ClientSkt(parent)
{
}

ClientSktUdp::~ClientSktUdp()
{
}

void ClientSktUdp::asynConn()
{
	show(QString("UDP channel to %1:%2 opened!")
		.arg(addr().toString()).arg(port()));
}

void ClientSktUdp::closed()
{
	show(QString("UDP channel closed!"));
}

void ClientSktUdp::close()
{
	m_socket.close();
	m_socket.disconnect(this);
}

void ClientSktUdp::error()
{
	QUdpSocket* s = qobject_cast<QUdpSocket*>(sender());

	show(QString("UDP socket error %1, %2").arg(s->error()).arg(s->errorString()));

	unplug();
}

bool ClientSktUdp::open()
{
	connect(&m_socket, SIGNAL(readyRead()), this, SLOT(newData()));
	connect(&m_socket, SIGNAL(disconnected()), this, SLOT(closed()));
	connect(&m_socket, SIGNAL(connected()), this, SLOT(asynConn()));
	connect(&m_socket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(error()));

	m_socket.connectToHost(addr(), port());

	return true;
}

void ClientSktUdp::newData()
{
	QUdpSocket* s = qobject_cast<QUdpSocket*>(sender());
	if (!s) return;

	qint64 bufLen = s->bytesAvailable();
	char* buf = TK::createBuffer(bufLen, MAXBUFFER);
	if (!buf) return;

	qint64 readLen = 0;
	qint64 ioLen = s->read(buf, bufLen);

	while (ioLen > 0)
	{
		readLen += ioLen;
		ioLen = s->read(buf+readLen, bufLen-readLen);
	}

	if (ioLen >= 0)
	{
		recordRecv((quint32)readLen);
		dump(buf, (quint32)readLen, false);
	}

	TK::releaseBuffer(buf);
}

void ClientSktUdp::send(const QByteArray& bin)
{
	const char *  src = bin.constData(); 
	qint64 srcLen = bin.length();

	qint64 writeLen = 0;
	qint64 ioLen = m_socket.write(src, srcLen);

	while (ioLen > 0)
	{
		writeLen += ioLen;
		ioLen = (writeLen >= srcLen) ? 0 :
				m_socket.write(src+writeLen, srcLen-writeLen);
	}

	if (writeLen != srcLen)
	{
		show(QString("failed to send data to %1:%2 [%3]")
			.arg(addr().toString()).arg(port()).arg(writeLen));
		return;
	}

	recordSend((quint32)writeLen);
	dump(src, (quint32)srcLen, true);
}

