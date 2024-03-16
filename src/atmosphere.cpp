#include "atmosphere.h"
#include <QDir>
#include <QStandardPaths>
#include <QtQuick>
#include <cutiestore.h>

AtmosphereModel::AtmosphereModel(QObject *parent)
	: QObject(parent)
{
	m_atmosphereStore = new CutieStore();
	m_atmosphereStore->setAppName("qml-module-cutie");
	m_atmosphereStore->setStoreName("atmosphere");
	connect(m_atmosphereStore, SIGNAL(dataChanged(QVariantMap)), this,
		SLOT(onAtmosphereDataChanged(QVariantMap)));
	onAtmosphereDataChanged(m_atmosphereStore->data());
}

AtmosphereModel::~AtmosphereModel()
{
	delete m_atmosphereStore;
}

QString AtmosphereModel::path()
{
	return p_path;
}

void AtmosphereModel::setPath(QString path)
{
	QVariantMap storeData = m_atmosphereStore->data();
	storeData["path"] = QVariant(path);
	m_atmosphereStore->setData(storeData);
	onAtmosphereDataChanged(storeData);
}

QString AtmosphereModel::variant()
{
	return p_variant;
}

QString AtmosphereModel::primaryColor()
{
	return p_primaryColor;
}

QString AtmosphereModel::primaryAlphaColor()
{
	return p_primaryAlphaColor;
}

QString AtmosphereModel::secondaryColor()
{
	return p_secondaryColor;
}

QString AtmosphereModel::secondaryAlphaColor()
{
	return p_secondaryAlphaColor;
}

QString AtmosphereModel::accentColor()
{
	return p_accentColor;
}

QString AtmosphereModel::textColor()
{
	return p_textColor;
}

QVariantList AtmosphereModel::atmosphereList()
{
	return p_atmosphereList;
}

QUrl AtmosphereModel::themeSound(QString name)
{
	QString theme = qEnvironmentVariable("CUTIE_SOUND_THEME", "cutie");
	QStringList dataDirList = QStandardPaths::standardLocations(
		QStandardPaths::GenericDataLocation);
	for (QString dir : dataDirList) {
		QDir dataDir(QDir(dir).filePath("sounds"));
		if (!dataDir.exists(theme))
			continue;
		QStringList filters;
		filters << (name + ".*");
		dataDir = QDir(dataDir.filePath(theme));
		for (QString subdir : dataDir.entryList(QDir::Dirs)) {
			QDir subDataDir(dataDir.filePath(subdir));
			QStringList candidates =
				subDataDir.entryList(filters, QDir::Files);
			if (candidates.count() > 0)
				return QUrl::fromLocalFile(subDataDir.filePath(
					candidates.first()));
		}
	}
	return QUrl();
}

void AtmosphereModel::onAtmosphereDataChanged(QVariantMap data)
{
	QString old_path = p_path;
	p_path = data["path"].toString();
	if (p_path.isEmpty())
		p_path = QString(DEFAULT_ATMOSPHERE_PATH);
	if (old_path != p_path)
		pathChanged();

	QStringList dirs = QStandardPaths::standardLocations(
		QStandardPaths::GenericDataLocation);
	QVariantList myList;
	foreach(const QString &str, dirs) {
		QDir checkDir = str + "/atmospheres";
		if (checkDir.exists()) {
			foreach(const QString &atmosphere_dir,
				checkDir.entryList(QDir::AllDirs |
						   QDir::NoDotAndDotDot)) {
				QVariantMap myMap;
				if (atmosphere_dir != "Current") {
					QSettings settings(
						checkDir.absolutePath() + "/" +
							atmosphere_dir +
							"/settings.ini",
						QSettings::IniFormat);
					myMap.insert("name", atmosphere_dir);
					myMap.insert("path",
						     checkDir.absolutePath() +
							     "/" +
							     atmosphere_dir);
					myMap.insert("variant",
						     settings.value("variant")
							     .toString());
					myList << myMap;
				}
			}
		}
	}
	p_atmosphereList = myList;
	atmosphereListChanged();

	QSettings settings(p_path + "/settings.ini", QSettings::IniFormat);
	p_primaryColor = "#" + settings.value("primaryColor").toString();
	primaryColorChanged();
	p_primaryAlphaColor =
		"#" + settings.value("primaryAlphaColor").toString();
	primaryAlphaColorChanged();
	p_secondaryColor = "#" + settings.value("secondaryColor").toString();
	secondaryColorChanged();
	p_secondaryAlphaColor =
		"#" + settings.value("secondaryAlphaColor").toString();
	secondaryAlphaColorChanged();
	p_accentColor = "#" + settings.value("accentColor").toString();
	accentColorChanged();
	p_textColor = "#" + settings.value("textColor").toString();
	textColorChanged();
	p_variant = "#" + settings.value("variant").toString();
	variantChanged();
}

QObject *AtmosphereModel::provider(QQmlEngine *engine, QJSEngine *scriptEngine)
{
	Q_UNUSED(scriptEngine);
	return new AtmosphereModel(engine);
}