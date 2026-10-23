#include "atmosphere.h"
#include "paletteextractor.h"
#include <QColor>
#include <QDir>
#include <QFileInfo>
#include <QGuiApplication>
#include <QImage>
#include <QPalette>
#include <QSettings>
#include <QStandardPaths>
#include <QtDBus>
#include <QtQuick>
#include <cutiestore.h>

namespace {

QString verifyPath(const QString &name)
{
	QString base = QDir::cleanPath(
		QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) +
		"/atmospheres");
	QString path = QDir::cleanPath(base + '/' + name);
	if (name == "Current" || name.contains('/') ||
	    name.contains(QChar::Null) || !path.startsWith(base + '/'))
		return QString();
	return path;
}

QVariant portalAppearanceSetting(const QString &key)
{
	QDBusMessage call = QDBusMessage::createMethodCall(
		"org.freedesktop.portal.Desktop",
		"/org/freedesktop/portal/desktop",
		"org.freedesktop.portal.Settings", "Read");
	call << "org.freedesktop.appearance" << key;
	QDBusMessage reply =
		QDBusConnection::sessionBus().call(call, QDBus::Block, 1000);
	if (reply.type() != QDBusMessage::ReplyMessage ||
	    reply.arguments().isEmpty())
		return {};

	QVariant value = reply.arguments().first();
	while (value.metaType() == QMetaType::fromType<QDBusVariant>())
		value = value.value<QDBusVariant>().variant();
	return value;
}

bool isKdeOrGnome()
{
	const QStringList desktops =
		qEnvironmentVariable("XDG_CURRENT_DESKTOP")
			.split(':', Qt::SkipEmptyParts);
	const bool detectedKdeOrGnome = desktops.contains("KDE", Qt::CaseInsensitive) ||
	      	  						desktops.contains("GNOME", Qt::CaseInsensitive);
	if (detectedKdeOrGnome)
		qDebug() << "libatmosphere: non-cutie shell detected";
	
	return detectedKdeOrGnome;
}

QColor portalAccentColor()
{
	const QVariant accent = portalAppearanceSetting("accent-color");
	if (accent.metaType() != QMetaType::fromType<QDBusArgument>())
		return {};

	const QDBusArgument rgb = accent.value<QDBusArgument>();
	if (rgb.currentType() != QDBusArgument::StructureType)
		return {};

	double red = -1, green = -1, blue = -1;
	rgb.beginStructure();
	rgb >> red >> green >> blue;
	rgb.endStructure();
	if (red < 0 || red > 1 || green < 0 || green > 1 || blue < 0 ||
	    blue > 1)
		return {};

	return QColor::fromRgbF(red, green, blue);
}

} // namespace

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

QString AtmosphereModel::particleEffects()
{
	return p_particleEffects;
}

QVariantList AtmosphereModel::atmosphereList()
{
	return p_atmosphereList;
}

QVariantMap AtmosphereModel::extractPalette(QUrl wallpaperUrl)
{
	return PaletteExtractor::extract(wallpaperUrl);
}

bool AtmosphereModel::saveAtmosphere(QString name, QUrl wallpaperUrl,
				      QVariantMap colors)
{
	QString dir = verifyPath(name);
	QImage wallpaper(wallpaperUrl.toLocalFile());
	if (dir.isEmpty() || wallpaper.isNull() || QFileInfo::exists(dir) ||
	    !QDir().mkpath(dir)) {
		qWarning() << "AtmosphereModel: cannot create atmosphere" << name;
		return false;
	}
	if (!wallpaper.save(dir + "/wallpaper.jpg", "JPG")) {
		qWarning() << "AtmosphereModel: failed to save wallpaper for" << name;
		QDir(dir).removeRecursively();
		return false;
	}

	QSettings settings(dir + "/settings.ini", QSettings::IniFormat);
	for (const QString &key : QStringList{
		     "variant", "primaryColor", "primaryAlphaColor",
		     "secondaryColor", "secondaryAlphaColor", "accentColor",
		     "textColor"})
		settings.setValue(key, colors.value(key).toString().remove('#'));
	settings.setValue("editable", true);
	settings.sync();

	onAtmosphereDataChanged(m_atmosphereStore->data());
	return true;
}

bool AtmosphereModel::deleteAtmosphere(QString name)
{
	QString dir = verifyPath(name);
	bool safeToDelete = !dir.isEmpty() && !QFileInfo(dir).isSymLink() &&
		  QSettings(dir + "/settings.ini", QSettings::IniFormat)
			  .value("editable", false).toBool();
	if (!safeToDelete || !QDir(dir).removeRecursively()) {
		qWarning() << "AtmosphereModel: cannot delete atmosphere" << name;
		return false;
	}

	if (QDir::cleanPath(p_path) == dir)
		setPath(QString(DEFAULT_ATMOSPHERE_PATH));
	else
		onAtmosphereDataChanged(m_atmosphereStore->data());
	return true;
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
	const bool useSystemAppearance = isKdeOrGnome();

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
					myMap.insert("editable",
						     settings.value("editable",
								    false)
							     .toBool());
					myList << myMap;
				}
			}
		}
	}
	p_atmosphereList = myList;
	atmosphereListChanged();

	QSettings settings(p_path + "/settings.ini", QSettings::IniFormat);
	p_primaryColor = "#" + settings.value("primaryColor").toString();
	p_primaryAlphaColor =
		"#" + settings.value("primaryAlphaColor").toString();
	p_secondaryColor = "#" + settings.value("secondaryColor").toString();
	p_secondaryAlphaColor =
		"#" + settings.value("secondaryAlphaColor").toString();
	p_accentColor = "#" + settings.value("accentColor").toString();
	p_textColor = "#" + settings.value("textColor").toString();
	p_variant = settings.value("variant").toString();
	p_particleEffects =
		settings.value("particle-effects", "none").toString();
	if (useSystemAppearance) {
		const QColor accent = portalAccentColor();
		qDebug() << "libatmosphere: portal accent" << accent;
		if (accent.isValid()) {
			const QPalette pal = QGuiApplication::palette();
			// Primary is the accent itself. Secondary keeps its hue but
			// has less saturation and more lightness, so it reads as a
			// softer, clearly different shade. Alpha variants are derived
			// from these two.
			const QColor primary = accent;
			const float hue =
				accent.hslHueF() < 0 ? 0.0f : accent.hslHueF();
			const float sat = accent.hslSaturationF();
			const float light = accent.lightnessF();
			const QColor secondary = QColor::fromHslF(
				hue, sat * 0.7f, light + (1.0f - light) * 0.35f);
			QColor primaryAlpha = primary;
			primaryAlpha.setAlpha(0x80);
			QColor secondaryAlpha = secondary;
			secondaryAlpha.setAlpha(0x65);

			p_primaryColor = primary.name();
			p_primaryAlphaColor = primaryAlpha.name(QColor::HexArgb);
			p_secondaryColor = secondary.name();
			p_secondaryAlphaColor = secondaryAlpha.name(QColor::HexArgb);
			p_accentColor = accent.name();
			p_textColor = pal.color(QPalette::WindowText).name();
			// color-scheme: 0 = no preference, 1 = dark, 2 = light
			const uint scheme =
				portalAppearanceSetting("color-scheme").toUInt();
			if (scheme)
				p_variant = scheme == 1 ? "dark" : "light";
		}
	}
	primaryColorChanged();
	primaryAlphaColorChanged();
	secondaryColorChanged();
	secondaryAlphaColorChanged();
	accentColorChanged();
	textColorChanged();
	variantChanged();
	particleEffectsChanged();
}

QObject *AtmosphereModel::provider(QQmlEngine *engine, QJSEngine *scriptEngine)
{
	Q_UNUSED(scriptEngine);
	return new AtmosphereModel(engine);
}