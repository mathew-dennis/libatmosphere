#pragma once
#include <QDebug>
#include <QtQml/qqml.h>
#include <QtQml/QQmlExtensionPlugin>

#define DEFAULT_ATMOSPHERE_PATH "/usr/share/atmospheres/city"

class CutieStore;

class AtmosphereModel : public QObject {
	Q_OBJECT
	Q_PROPERTY(QString path READ path WRITE setPath NOTIFY pathChanged);
	Q_PROPERTY(QString variant READ variant NOTIFY variantChanged);

	Q_PROPERTY(QString primaryColor READ primaryColor NOTIFY
			   primaryColorChanged);
	Q_PROPERTY(QString primaryAlphaColor READ primaryAlphaColor NOTIFY
			   primaryAlphaColorChanged);
	Q_PROPERTY(QString secondaryColor READ secondaryColor NOTIFY
			   secondaryColorChanged);
	Q_PROPERTY(QString secondaryAlphaColor READ secondaryAlphaColor NOTIFY
			   secondaryAlphaColorChanged);
	Q_PROPERTY(
		QString accentColor READ accentColor NOTIFY accentColorChanged);
	Q_PROPERTY(QString textColor READ textColor NOTIFY textColorChanged);

	Q_PROPERTY(QVariantList atmosphereList READ atmosphereList NOTIFY
			   atmosphereListChanged);

	CutieStore *m_atmosphereStore;

	QString p_path;
	QString p_variant;

	QString p_primaryColor;
	QString p_primaryAlphaColor;
	QString p_secondaryColor;
	QString p_secondaryAlphaColor;
	QString p_accentColor;
	QString p_textColor;

	QVariantList p_atmosphereList;

    public:
	AtmosphereModel(QObject *parent = 0);

	~AtmosphereModel();

	Q_INVOKABLE QString path();
	Q_INVOKABLE void setPath(QString path);

	Q_INVOKABLE QString variant();
	Q_INVOKABLE QString primaryColor();
	Q_INVOKABLE QString primaryAlphaColor();
	Q_INVOKABLE QString secondaryColor();
	Q_INVOKABLE QString secondaryAlphaColor();
	Q_INVOKABLE QString accentColor();
	Q_INVOKABLE QString textColor();

	Q_INVOKABLE QVariantList atmosphereList();

	Q_INVOKABLE QUrl themeSound(QString name);

	static QObject *provider(QQmlEngine *engine, QJSEngine *scriptEngine);

    public Q_SLOTS:
	void onAtmosphereDataChanged(QVariantMap data);

    Q_SIGNALS:
	void pathChanged();
	void variantChanged();

	void primaryColorChanged();
	void primaryAlphaColorChanged();
	void secondaryColorChanged();
	void secondaryAlphaColorChanged();
	void accentColorChanged();
	void textColorChanged();

	void atmosphereListChanged();
};