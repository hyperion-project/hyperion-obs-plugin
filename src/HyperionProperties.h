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
const char OBS_SETTINGS_SIZEDECIMATION[] = "SizeDecimation";

const bool OBS_SETTINGS_DEFAULT_AUTOSTART = false;
const char OBS_SETTINGS_DEFAULT_ADDRESS[] = "localhost";
const int OBS_SETTINGS_DEFAULT_PORT = 19400;
const int OBS_SETTINGS_DEFAULT_SIZEDECIMATION = DEFAULT_SIZEDECIMATION;

class HyperionProperties : public QDialog
{
	Q_OBJECT

public:
	explicit HyperionProperties(QWidget *parent = nullptr);
	~HyperionProperties() override;
	void enableStart(bool enable);
	void setWarningText(const char *msg);
	void saveSettings();

private Q_SLOTS:
	void onStart();
	void onStop();

private:
	Ui::HyperionProperties *ui;
};

static void output_started(void *data, calldata_t *cd);
static void output_stopped(void *data, calldata_t *cd);
static void logger_message(void *data, calldata_t *cd);

#endif // HYPERIONPROPERTIES_H
