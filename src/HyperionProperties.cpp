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
	connect(ui->ButtonStart, SIGNAL(clicked()), this, SLOT(onStart()));
	connect(ui->ButtonStop, SIGNAL(clicked()), this, SLOT(onStop()));

	config_t* config = obs_frontend_get_global_config();
	config_set_default_bool(config, CONFIG_SECTION, "AutoStart", false);
	config_set_default_string(config, CONFIG_SECTION, "Location", "127.0.0.1");
	config_set_default_int(config, CONFIG_SECTION, "Port", 19400);

	const bool autostart = config_get_bool(config, CONFIG_SECTION, "AutoStart");
	const char* location = config_get_string(config, CONFIG_SECTION, "Location");
	const int port = config_get_int(config, CONFIG_SECTION, "Port");

	setWindowTitle(tr("Name"));
	ui->LabelLocation->setText(tr("LabelLocation"));
	ui->AutoStart->setChecked(autostart);
	ui->Location->setText(location);
	ui->Port->setValue(port);
	
	ui->LabelWarning->setStyleSheet("QLabel { color : red; }");
	enableStart(true);

	if(autostart)
		onStart();
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
	ui->LabelWarning->setText(msg);
}

void HyperionProperties::saveSettings()
{
	bool autostart = ui->AutoStart->isChecked();
	QByteArray location = ui->Location->text().toUtf8();
	int port = ui->Port->value();

	config_t* config = obs_frontend_get_global_config();
	if(config)
	{
		config_set_bool(config, CONFIG_SECTION, "AutoStart", autostart);
		config_set_string(config, CONFIG_SECTION, "Location", location.constData());
		config_set_int(config, CONFIG_SECTION, "Port", port);
	}
}

void HyperionProperties::onStart()
{
	QByteArray location = ui->Location->text().toUtf8();
	int port = ui->Port->value();
	signal_handler_t *handler = hyperion_get_signal_handler();
	signal_handler_connect(handler, "stop", output_stopped , this);
	enableStart(false);
	setWarningText("");
	saveSettings();
	hyperion_enable(location.constData(), port);
}

void HyperionProperties::onStop()
{
	hyperion_disable();
}

static void output_stopped(void *data, calldata_t *cd)
{
	auto page = (HyperionProperties*) data;
	auto output = (obs_output_t*) calldata_ptr(cd, "output");
	bool running = calldata_bool(cd, "running");
	const char* msg = calldata_string(cd, "msg");

	if (running)
		page->setWarningText(msg);
		
	signal_handler_t *handler = obs_output_get_signal_handler(output);
	page->enableStart(true);
	signal_handler_disconnect(handler, "stop", output_stopped , page);
}
