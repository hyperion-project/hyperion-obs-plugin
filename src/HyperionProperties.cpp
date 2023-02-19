#include "HyperionProperties.h"
#include "ui_HyperionProperties.h"

#include <obs-frontend-api.h>
#include <util/config-file.h>

#define CONFIG_SECTION "HyperionOutput"

static void logger_message(void *data, calldata_t * cd)
{
	auto *page = static_cast<HyperionProperties*>(data);
	const char* msg = calldata_string(cd, "msg");
	page->appendLogText(msg);
}

HyperionProperties::HyperionProperties(QWidget *parent)
	: QDialog(parent)
	, ui(new Ui::HyperionProperties)
{
	ui->setupUi(this);
	this->setWindowFlags(this->windowFlags() & ~Qt::WindowContextHelpButtonHint);

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

	if (config_has_user_value(config, CONFIG_SECTION, OBS_SETTINGS_PRIORITY))
	{
		int priority = static_cast<int>(config_get_int(config, CONFIG_SECTION, OBS_SETTINGS_PRIORITY));
		ui->Priority->setValue(priority);
	}

	if (config_has_user_value(config, CONFIG_SECTION, OBS_SETTINGS_SIZEDECIMATION))
	{
		int sizeDecimation = static_cast<int>(config_get_int(config, CONFIG_SECTION, OBS_SETTINGS_SIZEDECIMATION));
		ui->SizeDecimation->setValue(sizeDecimation);
	}

	enableStart(true);

	signal_handler_t *handler = hyperion_get_signal_handler();
	signal_handler_connect(handler, "start", OnStartSignal , this);
	signal_handler_connect(handler, "stop", OnStopSignal , this);
	signal_handler_connect(handler, "log", logger_message, this);

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
	signal_handler_t *handler = hyperion_get_signal_handler();
	signal_handler_disconnect(handler, "start", OnStartSignal , this);
	signal_handler_disconnect(handler, "stop", OnStopSignal , this);
	signal_handler_disconnect(handler, "log", logger_message, this);

	saveSettings();
	hyperion_release();

	delete ui;
}

void HyperionProperties::enableStart(bool enable)
{
	ui->ButtonStart->setEnabled(enable);
	ui->ButtonStop->setEnabled(!enable);

	ui->Address->setEnabled(enable);
	ui->Port->setEnabled(enable);
	ui->Priority->setEnabled(enable);
	ui->SizeDecimation->setEnabled(enable);
}

void HyperionProperties::appendLogText(const char *msg)
{
	ui->LogText->appendPlainText(msg);
}

void HyperionProperties::clearLog()
{
	ui->LogText->clear();
}

void HyperionProperties::saveSettings()
{
	config_t* config = obs_frontend_get_global_config();
	if(config != nullptr)
	{
		bool autostart = ui->AutoStart->isChecked();
		QString address = ui->Address->text();
		int port = ui->Port->value();
		int priority = ui->Priority->value();
		int sizeDecimation = ui->SizeDecimation->value();

		config_set_bool(config, CONFIG_SECTION, OBS_SETTINGS_AUTOSTART, autostart);
		config_set_string(config, CONFIG_SECTION, OBS_SETTINGS_ADDRESS, address.toLocal8Bit().constData());
		config_set_int(config, CONFIG_SECTION, OBS_SETTINGS_PORT, port);
		config_set_int(config, CONFIG_SECTION, OBS_SETTINGS_PRIORITY, priority);
		config_set_int(config, CONFIG_SECTION, OBS_SETTINGS_SIZEDECIMATION, sizeDecimation);
	}
}

void HyperionProperties::onStart()
{
	QString address = ui->Address->text();
	int port = ui->Port->value();
	int priority = ui->Priority->value();
	int sizeDecimation = ui->SizeDecimation->value();

	hyperion_start_streaming(address, port, priority, sizeDecimation);
}

void HyperionProperties::onStop()
{
	enableStart(true);
	hyperion_stop_streaming();
}

void HyperionProperties::OnStartSignal(void *data, calldata_t *)
{
	auto page = (HyperionProperties *)data;
	page->clearLog();
	page->enableStart(false);
}

void HyperionProperties::OnStopSignal(void *data, calldata_t *)
{
	auto page = (HyperionProperties *)data;
	page->enableStart(true);
}
