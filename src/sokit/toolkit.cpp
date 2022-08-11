#include <QNetworkInterface>
#include <QComboBox>
#include <QAbstractSocket>
#include <QToolButton>
#include <QCoreApplication>
#include <QAction>

#include "toolkit.h"

const char* TK::socketTypeName(bool tcp)
{
	return tcp ? "TCP" : "UDP";
}

const char* TK::socketTypeName(QAbstractSocket* skt)
{
	return (skt->socketType() == QAbstractSocket::TcpSocket) ? "TCP" : "UDP";
}

void TK::initNetworkInterfaces(QComboBox* box, bool testDef)
{
	QString def;

	if (testDef)
	{
		def = box->currentText();
		box->clearEditText();
	}

	QList<QHostAddress> lst = QNetworkInterface::allAddresses();
	foreach(QHostAddress a, lst)
	{
		if (QAbstractSocket::IPv4Protocol == a.protocol())
			pushComboBox(box, a.toString());
	}

	if (!def.isEmpty() && (-1 != box->findText(def)))
		box->setEditText(def);
}

void TK::pushComboBox(QComboBox* box, const QString& item)
{
	if (-1 == box->findText(item))
		box->insertItem(0, item);
}

void TK::resetPushBox(QToolButton* btn)
{
	if (btn->isChecked())
		btn->click();
}

QString TK::ipstr(const QHostAddress& addr, quint16 port)
{
	return addr.toString() + ':' + QString::number(port);
}

QString TK::ipstr(const QHostAddress& addr, quint16 port, bool tcp)
{
	return QString("%1 %2").arg((tcp ? "[TCP]" : "[UDP]"), ipstr(addr, port));
}

bool TK::popIPAddr(QComboBox* ip, QComboBox* port, IPAddr& addr)
{
	QString sip = ip->currentText().trimmed();
	QString spt = port->currentText().trimmed();

	bool res = false;
	addr.port = spt.toUShort(&res, 10);
	return (res && addr.ip.setAddress(sip));
}

void TK::pushIPAddr(QComboBox* ip, QComboBox* port)
{
	if (ip)
		pushComboBox(ip, ip->currentText().trimmed());

	if (port)
		pushComboBox(port, port->currentText().trimmed());
}

const char* TK::hextab = "0123456789ABCDEF";

#define TOHEXSTR(v, c, tab) \
		if (v) { \
			*c++ = tab[(v>>4)&0xF]; \
			*c++ = tab[v & 0xF]; \
		}

QString TK::ascii2hex(const QString& src, QVector<uint>& posmap, uint& count)
{
	posmap = src.toUcs4();
	count = 0;

	uint val;
	char ch[16];

	QString res;

	PARSE_STA status = OUT;

	int bound = 0;
	int hexpos = 0;

	int len = posmap.count();
	for (int i = 0; i < len; ++i)
	{
		char* c = ch;
		unsigned char t;

		uint& ov = posmap[i];

		val = ov;
		ov = hexpos;

		if ('[' == val)
		{
			switch (status)
			{
			case OUT: status = OUT2IN;
				break;
			case OUT2IN: status = OUT;
				break;
			case IN: status = ERR;
				break;
			case ERR: break;
			}

			if (OUT2IN == status)
				continue;
		}
		else if (']' == val)
		{
			if (OUT != status)
			{
				if (bound & 1)
				{
					status = ERR;
				}
				else
				{
					status = OUT;
					continue;
				}
			}
		}
		else if (OUT != status)
		{
			if ((val >= '0' && val <= '9') ||
				(val >= 'A' && val <= 'F'))
			{
				status = IN;
			}
			else if (val >= 'a' && val <= 'f')
			{
				val = 'A' + (val - 'a');
				status = IN;
			}
			else if (val == ' ')
			{
				status = IN;
				continue;
			}
			else
			{
				status = ERR;
			}
		}

		if (OUT == status)
		{
			if (bound & 1)
			{
				status = ERR;
			}
			else
			{
				t = val >> 24 & 0xFF;
				TOHEXSTR(t, c, hextab)

				t = val >> 16 & 0xFF;
				TOHEXSTR(t, c, hextab)

				t = val >> 8 & 0xFF;
				TOHEXSTR(t, c, hextab)

				t = val & 0xFF;
				TOHEXSTR(t, c, hextab)
			}
			bound = 0;
		}
		else if (IN == status)
		{
			bound += 1;
			*c++ = static_cast<char>(val);
		}

		if (ERR == status)
		{
			res += QString("ERROR[%1]").arg(i);

			hexpos = 0;
			while (i < len)
				posmap[i++] = 0;

			break;
		}

		count += (c - ch);

		if (0 == (bound & 1))
			*c++ = ' ';

		*c = 0;

		res += ch;
		hexpos = res.length();
	}

	posmap.append(hexpos);
	count /= 2;
	return res;
}

#undef TOHEXSTR

QString TK::bin2hex(const char* buf, uint len)
{
	auto tmp = new char[len * 3 + 3];

	const char* s = buf;
	const char* e = buf + len;
	char* d = tmp;

	if (s && d)
	{
		*d++ = '[';

		while (s < e)
		{
			char c = *s++;
			*d++ = hextab[(c >> 4) & 0xF];
			*d++ = hextab[c & 0xF];
			*d++ = ' ';
		}

		*d++ = ']';
		*d = 0;
	}

	QString res(tmp);

	delete tmp;

	return res;
}

QString TK::bin2ascii(const char* buf, uint len)
{
	auto tmp = new char[len + 1];

	const char* s = buf;
	const char* e = buf + len;
	char* d = tmp;

	if (s && d)
	{
		while (s < e)
		{
			char c = *s++;
			*d++ = isprint(static_cast<uchar>(c)) ? c : '.';
		}

		*d = 0;
	}

	QString res(tmp);

	delete tmp;

	return res;
}

bool TK::ascii2bin(const QString& src, QByteArray& dst, QString& err)
{
	dst.clear();
	err.clear();

	QDataStream os(&dst, QIODevice::WriteOnly);


	QVector<uint> lst = src.toUcs4();

	PARSE_STA status = OUT;

	int bound = 0;
	quint8 hexval = 0;

	int len = lst.count();
	for (int i = 0; i < len; ++i)
	{
		uint val = lst.at(i);

		if ('[' == val)
		{
			switch (status)
			{
			case OUT: status = OUT2IN;
				break;
			case OUT2IN: status = OUT;
				break;
			case IN: status = ERR;
				break;
			case ERR: break;
			}

			if (OUT2IN == status)
				continue;
		}
		else if (']' == val)
		{
			if (OUT != status)
			{
				if (bound & 1)
				{
					status = ERR;
				}
				else
				{
					status = OUT;
					continue;
				}
			}
		}
		else if (OUT != status)
		{
			if (val >= '0' && val <= '9')
			{
				val -= '0';
				status = IN;
			}
			else if (val >= 'A' && val <= 'F')
			{
				val -= 'A';
				val += 10;
				status = IN;
			}
			else if (val >= 'a' && val <= 'f')
			{
				val -= 'a';
				val += 10;
				status = IN;
			}
			else if (val == ' ')
			{
				status = IN;
				continue;
			}
			else
			{
				status = ERR;
			}
		}

		if (OUT == status)
		{
			if (bound & 1)
			{
				status = ERR;
			}
			else
			{
				if (val >> 16)
					os << val;
				else if (val >> 8)
					os << static_cast<quint16>(val);
				else
					os << static_cast<quint8>(val);
			}

			bound = 0;
			hexval = 0;
		}
		else if (IN == status)
		{
			if (bound & 1)
				os << static_cast<quint8>(static_cast<quint8>(val & 0xF) | (hexval << 4));
			else
				hexval = static_cast<quint8>(val & 0xF);

			++bound;
		}

		if (ERR == status)
		{
			dst.clear();
			err.append(QString("ERROR[%1]").arg(i));
			break;
		}
	}

	return (ERR != status);
}

char* TK::createBuffer(qint64& cap, qint64 limit)
{
	if (cap < 0 || cap > limit)
		return nullptr;

	if (0 == cap)
		cap = 1;

	return new char[cap];
}

void TK::releaseBuffer(char*& buf)
{
	delete buf;
	buf = nullptr;
}
