#include "toolkit.h"
#include "setting.h"
#include "serverform.h"

#include <QShortcut>

#define SET_SEC_SVR  "server"
#define SET_KEY_SVR  "/server"

#define SET_KEY_CMBTA  "/tip"
#define SET_KEY_CMBUA  "/uip"
#define SET_KEY_CMBTP  "/tport"
#define SET_KEY_CMBUP  "/uport"

#define SET_VAL_LGSVR  "log_server"

ServerForm::ServerForm(QWidget* parent, Qt::WindowFlags flags)
	: BaseForm(parent, flags)
{
	m_ui.setupUi(this);
}

ServerForm::~ServerForm()
{
	if (lock(1000))
	{
		m_tcp.disconnect(this);
		m_udp.disconnect(this);

		m_tcp.stop();
		m_udp.stop();

		unlock();
	}

	saveConfig();
}

void ServerForm::initConfig()
{
	QString sss(SET_SEC_SVR);
	Setting::lord(sss + SET_KEY_CMBTA, SET_PFX_CMBITM, *m_ui.cmbTcpAddr, false);
	Setting::lord(sss + SET_KEY_CMBUA, SET_PFX_CMBITM, *m_ui.cmbUdpAddr, false);
	Setting::lord(sss + SET_KEY_CMBTP, SET_PFX_CMBITM, *m_ui.cmbTcpPort);
	Setting::lord(sss + SET_KEY_CMBUP, SET_PFX_CMBITM, *m_ui.cmbUdpPort);

	QString skl(SET_SEC_DIR);
	skl += SET_KEY_LOG;
	skl = Setting::get(skl, SET_KEY_SVR, SET_VAL_LGSVR);
	setProperty(SET_SEC_DIR, skl);

	TK::initNetworkInterfaces(m_ui.cmbTcpAddr, true);
	TK::initNetworkInterfaces(m_ui.cmbUdpAddr, true);
}

void ServerForm::saveConfig()
{
	QString sss(SET_SEC_SVR);
	Setting::save(sss + SET_KEY_CMBTA, SET_PFX_CMBITM, *m_ui.cmbTcpAddr, false);
	Setting::save(sss + SET_KEY_CMBUA, SET_PFX_CMBITM, *m_ui.cmbUdpAddr, false);
	Setting::save(sss + SET_KEY_CMBTP, SET_PFX_CMBITM, *m_ui.cmbTcpPort);
	Setting::save(sss + SET_KEY_CMBUP, SET_PFX_CMBITM, *m_ui.cmbUdpPort);

	QString skl(SET_SEC_DIR);
	skl += SET_KEY_LOG;
	Setting::set(skl, SET_KEY_SVR, property(SET_SEC_DIR).toString());
}

bool ServerForm::initForm()
{
	initCounter(m_ui.labRecv, m_ui.labSend);
	initLogger(m_ui.chkLog, m_ui.btnClear, m_ui.treeOutput, m_ui.txtOutput);
	initLister(m_ui.btnConnAll, m_ui.btnConnDel, m_ui.lstConn);

	bindBuffer(1, m_ui.edtBuf1, m_ui.btnSend1, nullptr);
	bindBuffer(2, m_ui.edtBuf2, m_ui.btnSend2, nullptr);
	bindBuffer(3, m_ui.edtBuf3, m_ui.btnSend3, nullptr);

	connect(m_ui.btnTcp, SIGNAL(clicked(bool)), this, SLOT(trigger(bool)));
	connect(m_ui.btnUdp, SIGNAL(clicked(bool)), this, SLOT(trigger(bool)));

	connect(&m_tcp, SIGNAL(connOpen(const QString&)), this, SLOT(listerAdd(const QString&)));
	connect(&m_tcp, SIGNAL(connClose(const QString&)), this, SLOT(listerRemove(const QString&)));
	connect(&m_tcp, SIGNAL(message(const QString&)), this, SIGNAL(output(const QString&)));
	connect(&m_tcp, SIGNAL(dumpbin(const QString&,const char*,quint32)), this,
	        SIGNAL(output(const QString&,const char*,quint32)));
	connect(&m_tcp, SIGNAL(countRecv(qint32)), this, SLOT(countRecv(qint32)));
	connect(&m_tcp, SIGNAL(countSend(qint32)), this, SLOT(countSend(qint32)));

	connect(&m_udp, SIGNAL(connOpen(const QString&)), this, SLOT(listerAdd(const QString&)));
	connect(&m_udp, SIGNAL(connClose(const QString&)), this, SLOT(listerRemove(const QString&)));
	connect(&m_udp, SIGNAL(message(const QString&)), this, SIGNAL(output(const QString&)));
	connect(&m_udp, SIGNAL(dumpbin(const QString&,const char*,quint32)), this,
	        SIGNAL(output(const QString&,const char*,quint32)));
	connect(&m_udp, SIGNAL(countRecv(qint32)), this, SLOT(countRecv(qint32)));
	connect(&m_udp, SIGNAL(countSend(qint32)), this, SLOT(countSend(qint32)));

	return true;
}

bool ServerForm::initHotkeys()
{
	bindFocus(m_ui.cmbTcpAddr, Qt::Key_Escape);
	bindClick(m_ui.btnTcp, Qt::CTRL + Qt::Key_T);
	bindClick(m_ui.btnUdp, Qt::CTRL + Qt::Key_U);

	return true;
}

void ServerForm::kill(QStringList& list)
{
	QString tcpname(TK::socketTypeName(true));

	while (!list.isEmpty())
	{
		QString key = list.takeFirst();
		if (key.contains(tcpname))
			m_tcp.kill(key);
		else
			m_udp.kill(key);
	}
}

void ServerForm::trigger(bool start)
{
	auto btnTrigger = qobject_cast<QToolButton*>(sender());
	if (!btnTrigger) return;

	bool istcp = (btnTrigger == m_ui.btnTcp);
	QComboBox* cbAddr = istcp ? m_ui.cmbTcpAddr : m_ui.cmbUdpAddr;
	QComboBox* cbPort = istcp ? m_ui.cmbTcpPort : m_ui.cmbUdpPort;
	ServerSkt* server = istcp ? static_cast<ServerSkt*>(&m_tcp) : static_cast<ServerSkt*>(&m_udp);

	IPAddr addr;
	if (start)
		start = TK::popIPAddr(cbAddr, cbPort, addr);

	lock();

	if (start)
		start = server->start(addr.ip, addr.port);
	else
		server->stop();

	unlock();

	cbAddr->setDisabled(start);
	cbPort->setDisabled(start);

	if (start)
		TK::pushIPAddr(nullptr, cbPort);
	else
		TK::resetPushBox(btnTrigger);
}

void ServerForm::send(const QString& data, const QString&)
{
	QStringList list;
	if (lock(1000))
	{
		listerSelected(list);
		unlock();
	}

	QString tcpname(TK::socketTypeName(true));
	while (!list.isEmpty())
	{
		QString key = list.takeFirst();

		ServerSkt* server = key.contains(tcpname)
			                    ? static_cast<ServerSkt*>(&m_tcp)
			                    : static_cast<ServerSkt*>(&m_udp);

		server->send(key, data);
	}
}
