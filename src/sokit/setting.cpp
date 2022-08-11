#include <QComboBox>
#include <QSettings>
#include <QFileInfo>
#include <QDir>

#include "setting.h"

Setting::Setting()
{
	QString path(QDir::currentPath());
	if (!QFileInfo(path).isWritable() ||
		path == QDir::homePath())
	{
		QDir dir(QDir::home());
		dir.mkdir("." SET_APP_NAME);
		if (dir.cd("." SET_APP_NAME))
			path = dir.absolutePath();
	}

	QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, path);
}

QSettings& Setting::storage()
{
	static Setting dummy; // to ensure call QSettings::setPath before create the g_settings
	static QSettings g_settings(QSettings::IniFormat, QSettings::UserScope, SET_APP_NAME);
	return g_settings;
}

QString Setting::path()
{
	return QFileInfo(storage().fileName()).dir().absolutePath();
}

void Setting::flush()
{
	storage().sync();
}

void Setting::set(const QString& section, const QString& key, const QString& val)
{
	storage().setValue(section + key, val);
}

QString Setting::get(const QString& section, const QString& key, const QString& def)
{
	return storage().value(section + key, def).toString();
}

void Setting::save(const QString& section, const QString& prefix, const QComboBox& cmb, bool all)
{
	QSettings& store = storage();

	store.beginGroup(section);

	QString tkey = prefix + SET_PFX_CMBTXT;

	QString tval = cmb.currentText().trimmed();
	if (!tval.isEmpty())
		store.setValue(tkey, tval);

	if (all)
	{
		QStringList keys, vals;

		keys = store.childKeys();
		qint32 n = keys.size();
		if (n > 0)
		{
			keys.sort();

			while (n--)
			{
				QString k = keys[n];
				if ((k != tkey) && k.startsWith(prefix))
				{
					QString v = store.value(k).toString().trimmed();
					if (!v.isEmpty() && (-1 == cmb.findText(v)))
						vals.prepend(v);

					store.remove(k);
				}
			}
		}

		n = cmb.count();
		if (n > SET_MAX_CMBITM)
			n = SET_MAX_CMBITM;

		qint32 i = 0;
		for (i = 0; i < n; ++i)
			store.setValue(prefix + QString::number(i), cmb.itemText(i));

		n = (vals.count() > SET_MAX_CMBITM) ? SET_MAX_CMBITM : vals.count();
		for (qint32 j = 0; i < n; ++i, ++j)
			store.setValue(prefix + QString::number(i), vals[j]);
	}

	store.endGroup();
}

void Setting::lord(const QString& section, const QString& prefix, QComboBox& cmb, bool all)
{
	cmb.clear();

	QSettings& store = storage();

	QStringList keys, vals;

	store.beginGroup(section);

	keys = store.childKeys();
	qint32 n = keys.size();
	if (n > 0)
	{
		QString tval;
		QString tkey = prefix + SET_PFX_CMBTXT;

		keys.sort();

		while (n--)
		{
			QString k = keys[n];
			if (k.startsWith(prefix))
			{
				QString v = store.value(k).toString().trimmed();
				if (k == tkey)
					tval = v;
				else if (all && !v.isEmpty())
					vals.append(v);
			}
		}

		vals.removeDuplicates();

		n = vals.count();
		if (n > 0)
		{
			if (n > SET_MAX_CMBITM)
				n = SET_MAX_CMBITM;

			while (n--)
				cmb.addItem(vals[n]);
		}

		if (!tval.isEmpty())
			cmb.setEditText(tval);
	}

	store.endGroup();
}
