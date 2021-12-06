#include "HyperionProperties.h"
#include "ui_HyperionProperties.h"

#include <obs-frontend-api.h>
#include <util/config-file.h>

#define CONFIG_SECTION "HyperionOutput"

// Constants
namespace {
const char OBS_CONFIG_AUTOSTART[] = "AutoStart";
const char OBS_CONFIG_ADDRESS[] = "Address";
const char OBS_CONFIG_PORT[] = "Port";
} //End of constants


HyperionProperties::HyperionProperties(QWidget *parent)
	: QDialog(parent)
	, ui(new Ui::HyperionProperties)
{
	ui->setupUi(this);

	connect(ui->ButtonStart, &QPushButton::clicked, this, &HyperionProperties::saveSettings);
	connect(ui->AutoStart, &QCheckBox::stateChanged, this, &HyperionProperties::saveSettings);

	connect(ui->ButtonStart, &QPushButton::clicked, this, &HyperionProperties::onStart);
	connect(ui->ButtonStop, &QPushButton::clicked, this, &HyperionProperties::onStop);

	config_t* config = obs_frontend_get_global_config();
	const bool autostart = config_get_bool(config, CONFIG_SECTION, OBS_CONFIG_AUTOSTART);
	const char* address = config_get_string(config, CONFIG_SECTION, OBS_CONFIG_ADDRESS);
	const int port = config_get_int(config, CONFIG_SECTION, OBS_CONFIG_PORT);

	ui->AutoStart->setChecked(autostart);
	ui->Address->setText(address);
	ui->Port->setValue(port);
	
	enableStart(true);

	if(autostart)
	{
		onStart();
	}
}

HyperionProperties::~HyperionProperties()
{
	saveSettings();
	hyperion_release();
	delete ui;
}

void HyperionProperties::enableStart(bool enable)
{
	ui->ButtonStart->setEnabled(enable);
	ui->ButtonStop->setEnabled(!enable);
}

void HyperionProperties::setWarningText(const char *msg)
{
	ui->WarningText->setText(msg);
}

void HyperionProperties::saveSettings()
{
	bool autostart = ui->AutoStart->isChecked();
	QString address = ui->Address->text();
	int port = ui->Port->value();

	config_t* config = obs_frontend_get_global_config();
	if(config != nullptr)
	{
		config_set_bool(config, CONFIG_SECTION, OBS_CONFIG_AUTOSTART, autostart);
		config_set_string(config, CONFIG_SECTION, OBS_CONFIG_ADDRESS, address.toLocal8Bit().constData());
		config_set_int(config, CONFIG_SECTION, OBS_CONFIG_PORT, port);
	}
}

void HyperionProperties::onStart()
{
	QString address = ui->Address->text();
	int port = ui->Port->value();
	signal_handler_t *handler = hyperion_get_signal_handler();
	signal_handler_connect(handler, "stop", output_stopped , this);
	enableStart(false);
	setWarningText("");
	hyperion_start_streaming(address, port);
}

void HyperionProperties::onStop()
{
	hyperion_stop_streaming();
}

static void output_stopped(void *data, calldata_t *cd)
{
	auto *page = static_cast<HyperionProperties*>(data);
	auto *output = static_cast<obs_output_t*>(calldata_ptr(cd, "output"));
	bool running = calldata_bool(cd, "running");
	const char* msg = calldata_string(cd, "msg");

	if (running)
	{
		page->setWarningText(msg);
	}
		
	signal_handler_t *handler = obs_output_get_signal_handler(output);
	page->enableStart(true);
	signal_handler_disconnect(handler, "stop", output_stopped , page);
}
