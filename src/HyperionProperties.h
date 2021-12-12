#ifndef HYPERIONPROPERTIES_H
#define HYPERIONPROPERTIES_H

#include <QDialog>
#include "hyperion-obs.h"

namespace Ui {
	class HyperionProperties;
}

const char OBS_SETTINGS_AUTOSTART[] = "AutoStart";
const char OBS_SETTINGS_ADDRESS[] = "Address";
const char OBS_SETTINGS_PORT[] = "Port";
const char OBS_SETTINGS_PRIORITY[] = "Priority";
const char OBS_SETTINGS_SIZEDECIMATION[] = "SizeDecimation";

class HyperionProperties : public QDialog
{
	Q_OBJECT

public:
	explicit HyperionProperties(QWidget *parent = nullptr);
	~HyperionProperties() override;
	void enableStart(bool enable);
	void appendLogText(const char *msg);
	void clearLog();
	void saveSettings();

public Q_SLOTS:
	void onStart();
	void onStop();

private:
	Ui::HyperionProperties *ui;
};

#endif // HYPERIONPROPERTIES_H
