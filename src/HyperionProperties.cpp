#include "HyperionProperties.h"
#include "ui_HyperionProperties.h"

#include <obs-frontend-api.h>
#include <util/config-file.h>

#define CONFIG_SECTION "HyperionOutput"

HyperionProperties::HyperionProperties(QWidget *parent)
	: QDialog(parent)
	, ui(new Ui::HyperionProperties)
{
	ui->setupUi(this);

	config_t* config = obs_frontend_get_global_config();
	if (config_has_user_value(config, CONFIG_SECTION, OBS_SETTINGS_AUTOSTART))
	{
		bool autostart = config_get_bool(config, CONFIG_SECTION, OBS_SETTINGS_AUTOSTART);
		ui->AutoStart->setChecked(autostart);
	}

	if (config_has_user_value(config, CONFIG_SECTION, OBS_SETTINGS_ADDRESS))
	{
		QString address = config_get_string(config, CONFIG_SECTION, OBS_SETTINGS_ADDRESS);
		ui->Address->setText(address);
	}

	if (config_has_user_value(config, CONFIG_SECTION, OBS_SETTINGS_PORT))
	{
		int port = static_cast<int>(config_get_int(config, CONFIG_SECTION, OBS_SETTINGS_PORT));
		ui->Port->setValue(port);
	}

	if (config_has_user_value(config, CONFIG_SECTION, OBS_SETTINGS_SIZEDECIMATION))
	{
		int sizeDecimation = static_cast<int>(config_get_int(config, CONFIG_SECTION, OBS_SETTINGS_SIZEDECIMATION));
		ui->SizeDecimation->setValue(sizeDecimation);
	}
	
	enableStart(true);

	connect(ui->ButtonStart, &QPushButton::clicked, this, &HyperionProperties::saveSettings);
	connect(ui->AutoStart, &QCheckBox::stateChanged, this, &HyperionProperties::saveSettings);

	connect(ui->ButtonStart, &QPushButton::clicked, this, &HyperionProperties::onStart);
	connect(ui->ButtonStop, &QPushButton::clicked, this, &HyperionProperties::onStop);

	if(ui->AutoStart->isChecked())
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
	ui->WarningText->appendPlainText(msg);
}

void HyperionProperties::saveSettings()
{
	config_t* config = obs_frontend_get_global_config();
	if(config != nullptr)
	{
		bool autostart = ui->AutoStart->isChecked();
		QString address = ui->Address->text();
		int port = ui->Port->value();
		int sizeDecimation = ui->SizeDecimation->value();

		config_set_bool(config, CONFIG_SECTION, OBS_SETTINGS_AUTOSTART, autostart);
		config_set_string(config, CONFIG_SECTION, OBS_SETTINGS_ADDRESS, address.toLocal8Bit().constData());
		config_set_int(config, CONFIG_SECTION, OBS_SETTINGS_PORT, port);
		config_set_int(config, CONFIG_SECTION, OBS_SETTINGS_SIZEDECIMATION, sizeDecimation);
	}
}

void HyperionProperties::onStart()
{
	QString address = ui->Address->text();
	int port = ui->Port->value();
	int sizeDecimation = ui->SizeDecimation->value();

	signal_handler_t *handler = hyperion_get_signal_handler();
	signal_handler_connect(handler, "stop", output_stopped , this);
	signal_handler_connect(handler, "log", logger_message, this);
	
	enableStart(false);
	setWarningText("");
	hyperion_start_streaming(address, port, sizeDecimation);
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
	signal_handler_disconnect(handler, "log", logger_message, page);
}

static void logger_message(void *data, calldata_t *cd)
{
	auto *page = static_cast<HyperionProperties*>(data);
	auto *output = static_cast<obs_output_t*>(calldata_ptr(cd, "output"));
	const char* msg = calldata_string(cd, "msg");
	page->setWarningText(msg);
}
