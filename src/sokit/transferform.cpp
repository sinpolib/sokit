#include "toolkit.h"
#include "setting.h"
#include "transferskt.h"
#include "transferform.h"

#include <QShortcut>

#define SET_SEC_TRANS  "transfer"
#define SET_KEY_TRANS  "/transfer"
#define SET_KEY_CMBSA  "/srcip"
#define SET_KEY_CMBDA  "/dstip"
#define SET_KEY_CMBSP  "/srcport"
#define SET_KEY_CMBDP  "/dstport"

#define SET_VAL_LGTAN  "log_transf"

#define PROP_EDIT "edit"
#define PROP_DIRT "dirt"

TransferForm::TransferForm(QWidget* parent, Qt::WindowFlags flags)
	: BaseForm(parent, flags), m_server(nullptr)
{
	m_ui.setupUi(this);
}

TransferForm::~TransferForm()
{
	if (m_server && lock(1000))
	{
		m_server->disconnect(this);
		delete m_server;
		m_server = nullptr;

		unlock();
	}

	saveConfig();
}

void TransferForm::initConfig()
{
	QString sst(SET_SEC_TRANS);
	Setting::lord(sst + SET_KEY_CMBSA, SET_PFX_CMBITM, *m_ui.cmbSrcAddr, false);
	Setting::lord(sst + SET_KEY_CMBDA, SET_PFX_CMBITM, *m_ui.cmbDstAddr);
	Setting::lord(sst + SET_KEY_CMBSP, SET_PFX_CMBITM, *m_ui.cmbSrcPort);
	Setting::lord(sst + SET_KEY_CMBDP, SET_PFX_CMBITM, *m_ui.cmbDstPort);

	QString skl(SET_SEC_DIR);
	skl += SET_KEY_LOG;
	skl = Setting::get(skl, SET_KEY_TRANS, SET_VAL_LGTAN);
	setProperty(SET_SEC_DIR, skl);

	TK::initNetworkInterfaces(m_ui.cmbSrcAddr, true);
	TK::initNetworkInterfaces(m_ui.cmbDstAddr);
}

void TransferForm::saveConfig()
{
	QString sst(SET_SEC_TRANS);
	Setting::save(sst + SET_KEY_CMBSA, SET_PFX_CMBITM, *m_ui.cmbSrcAddr, false);
	Setting::save(sst + SET_KEY_CMBDA, SET_PFX_CMBITM, *m_ui.cmbDstAddr);
	Setting::save(sst + SET_KEY_CMBSP, SET_PFX_CMBITM, *m_ui.cmbSrcPort);
	Setting::save(sst + SET_KEY_CMBDP, SET_PFX_CMBITM, *m_ui.cmbDstPort);

	QString skl(SET_SEC_DIR);
	skl += SET_KEY_LOG;
	Setting::set(skl, SET_KEY_TRANS, property(SET_SEC_DIR).toString());
}

bool TransferForm::initForm()
{
	initCounter(m_ui.labRecv, m_ui.labSend);
	initLogger(m_ui.chkLog, m_ui.btnClear, m_ui.treeOutput, m_ui.txtOutput);
	initLister(m_ui.btnConnAll, m_ui.btnConnDel, m_ui.lstConn);

	bindBuffer(1, m_ui.edtBuf1, m_ui.btnSend1, m_ui.cmbDir1);
	bindBuffer(2, m_ui.edtBuf2, m_ui.btnSend2, m_ui.cmbDir2);
	bindBuffer(3, m_ui.edtBuf3, m_ui.btnSend3, m_ui.cmbDir3);

	bindSelect(m_ui.cmbDir1, -1, Qt::Key_F5);
	bindSelect(m_ui.cmbDir2, -1, Qt::Key_F6);
	bindSelect(m_ui.cmbDir3, -1, Qt::Key_F7);

	connect(m_ui.btnTrigger, SIGNAL(clicked(bool)), this, SLOT(trigger(bool)));

	return true;
}

bool TransferForm::initHotkeys()
{
	bindFocus(m_ui.cmbSrcAddr, Qt::Key_Escape);
	bindClick(m_ui.btnTrigger, Qt::CTRL | Qt::Key_S);
	bindSelect(m_ui.cmbType, 0, Qt::CTRL | Qt::Key_T);
	bindSelect(m_ui.cmbType, 1, Qt::CTRL | Qt::Key_U);

	return true;
}

void TransferForm::kill(QStringList& list)
{
	if (m_server)
	{
		while (!list.isEmpty())
			m_server->kill(list.takeFirst());
	}
}

void TransferForm::trigger(bool start)
{
	if (lock(1000))
	{
		if (m_server)
		{
			m_server->stop();
			m_server->disconnect(this);
			delete m_server;
			m_server = nullptr;
		}

		IPAddr sa, da;
		if (start)
		{
			start = TK::popIPAddr(m_ui.cmbSrcAddr, m_ui.cmbSrcPort, sa) &&
				TK::popIPAddr(m_ui.cmbDstAddr, m_ui.cmbDstPort, da);
		}

		if (start)
		{
			QString type = m_ui.cmbType->currentText();
			if (type.contains(TK::socketTypeName(true)))
				m_server = new TransferSktTcp(this);
			else
				m_server = new TransferSktUdp(this);

			if (m_server)
			{
				connect(m_server, SIGNAL(connOpen(const QString&)), this, SLOT(listerAdd(const QString&)));
				connect(m_server, SIGNAL(connClose(const QString&)), this, SLOT(listerRemove(const QString&)));
				connect(m_server, SIGNAL(message(const QString&)), this, SIGNAL(output(const QString&)));
				connect(m_server, SIGNAL(dumpbin(const QString&,const char*,quint32)), this,
				        SIGNAL(output(const QString&,const char*,quint32)));
				connect(m_server, SIGNAL(countRecv(qint32)), this, SLOT(countRecv(qint32)));
				connect(m_server, SIGNAL(countSend(qint32)), this, SLOT(countSend(qint32)));
				connect(m_server, SIGNAL(stopped()), this, SLOT(stop()));

				start = m_server->start(sa.ip, sa.port, da.ip, da.port);
				if (!start)
				{
					m_server->disconnect(this);
					delete m_server;
					m_server = nullptr;
				}
			}
			else
			{
				start = false;
			}
		}

		unlock();
	}

	m_ui.cmbSrcAddr->setDisabled(start);
	m_ui.cmbSrcPort->setDisabled(start);
	m_ui.cmbDstAddr->setDisabled(start);
	m_ui.cmbDstPort->setDisabled(start);
	m_ui.cmbType->setDisabled(start);

	if (start)
	{
		TK::pushIPAddr(m_ui.cmbSrcPort, m_ui.cmbSrcAddr);
		TK::pushIPAddr(nullptr, m_ui.cmbDstPort);
	}
	else
	{
		TK::resetPushBox(m_ui.btnTrigger);
	}
}

void TransferForm::stop()
{
	trigger(false);
}

void TransferForm::send(const QString& data, const QString& dir)
{
	bool s2d = dir.startsWith('S');

	if (m_server && lock(1000))
	{
		QStringList list;
		listerSelected(list);

		while (!list.isEmpty())
			m_server->send(list.takeFirst(), s2d, data);

		unlock();
	}
}
