#include <obs-frontend-api.h>
#include <obs-module.h>
#include <util/threading.h>

#include <QMainWindow>
#include <QAction>
#include <QImage>

#include "hyperion-obs.h"
#include "HyperionProperties.h"
#include "FlatBufferConnection.h"
#include "Image.h"

// Constants
namespace {
const char OBS_AUTHOR[] = "Hyperion-Project";
const char OBS_MODULE_NAME[] = "hyperion-obs";
const char OBS_DEFAULT_LOCALE[] = "en-US";
const char OBS_OUTPUT_NAME[] = "Hyperion";
const char OBS_MENU_ID[] = "UI.Menu";

const int  FLATBUFFER_DEFAULT_PRIORITY = 150;
} //End of constants

struct hyperion_output
{
	obs_output_t *output = nullptr;
	FlatBufferConnection* client = nullptr;
	uint32_t width = 0;
	uint32_t height = 0;
	int sizeDecimation = DEFAULT_SIZEDECIMATION;
	bool active = false;
	pthread_mutex_t mutex;
};

HyperionProperties *hyperionProperties;
obs_output_t* _hyperionOutput;

void hyperion_signal_init(const char *signal)
{
	signal_handler_t *handler = hyperion_get_signal_handler();
	signal_handler_add(handler, signal);
}

void hyperion_signal_stop(const char *msg, bool running)
{
	struct calldata call_data;
	calldata_init(&call_data);
	calldata_set_string(&call_data, "msg", msg);
	calldata_set_bool(&call_data, "running", running);
	signal_handler_t *handler = hyperion_get_signal_handler();
	signal_handler_signal(handler, "stop", &call_data);
	calldata_free(&call_data);
}

void hyperion_signal_log(const char *msg)
{
	struct calldata call_data;
	calldata_init(&call_data);
	calldata_set_string(&call_data, "msg", msg);
	signal_handler_t *handler = hyperion_get_signal_handler();
	signal_handler_signal(handler, "log", &call_data);
	calldata_free(&call_data);
}

int Connect(void *data)
{
	hyperion_output *out_data = static_cast<hyperion_output*>(data);
	obs_data_t *settings = obs_output_get_settings(out_data->output);
	if (!out_data->active)
	{
		out_data->client = new FlatBufferConnection(OBS_MODULE_NAME, obs_data_get_string(settings, OBS_SETTINGS_ADDRESS), FLATBUFFER_DEFAULT_PRIORITY, obs_data_get_int(settings, OBS_SETTINGS_PORT));
	}

	return 0;
}

static void Disconnect(void *data)
{
	hyperion_output *out_data = static_cast<hyperion_output*>(data);
	delete out_data->client;
	out_data->client = nullptr;
}

static const char *hyperion_output_getname(void *unused)
{
	UNUSED_PARAMETER(unused);
	return obs_module_text("Hyperion OBS Output");
}

static void *hyperion_output_create(obs_data_t *settings, obs_output_t *output)
{
	hyperion_output *data = static_cast<hyperion_output*>(bzalloc(sizeof(struct hyperion_output)));
	data->output = output;

	pthread_mutex_init_value(&data->mutex);
	if (pthread_mutex_init(&data->mutex, NULL) == 0)
	{
		UNUSED_PARAMETER(settings);
		return data;
	}

	return nullptr;
}

static void hyperion_output_destroy(void *data)
{
	hyperion_output *out_data = static_cast<hyperion_output*>(data);
	if (out_data != nullptr)
	{
		pthread_mutex_destroy(&out_data->mutex);
		bfree(out_data);
	}
}

static bool hyperion_output_start(void *data)
{
	hyperion_output *out_data = static_cast<hyperion_output*>(data);
	out_data->width = obs_output_get_width(out_data->output);
	out_data->height = obs_output_get_height(out_data->output);

	obs_data_t *settings = obs_output_get_settings(out_data->output);
	out_data->sizeDecimation = obs_data_get_int(settings, OBS_SETTINGS_SIZEDECIMATION);

	int ret = Connect(data);

	struct video_scale_info conv;
	conv.format = 	VIDEO_FORMAT_RGBA;
	conv.width = obs_output_get_width(out_data->output);
	conv.height = obs_output_get_height(out_data->output);

	obs_output_set_video_conversion(out_data->output, &conv);

	//video_t *video = obs_output_video(out_data->output);
	//enum video_format format = video_output_get_format(video);
	// double video_frame_rate = video_output_get_frame_rate(video);

	if (!obs_output_can_begin_data_capture(out_data->output, 0))
	{
		return false;
	}

	out_data->active = true;

	hyperion_signal_log("Let's GO");

	return obs_output_begin_data_capture(out_data->output, 0);
}

static void hyperion_output_stop(void *data, uint64_t ts)
{
	hyperion_output *out_data = static_cast<hyperion_output*>(data);
	UNUSED_PARAMETER(ts);

	if(out_data->active)
	{
		out_data->active = false;
		obs_output_end_data_capture(out_data->output);	
		Disconnect(data);
		hyperion_signal_stop("stop", false);
	}
}

static void hyperion_output_raw_video(void *param, struct video_data *frame)
{
	hyperion_output *out_data = static_cast<hyperion_output*>(param);

	if(out_data->active)
	{
		pthread_mutex_lock(&out_data->mutex);

		if ( out_data->sizeDecimation == 0)
		{
			out_data->sizeDecimation = DEFAULT_SIZEDECIMATION;
		}
		int outputWidth = out_data->width / out_data->sizeDecimation;

		QImage RGBAImage(static_cast<const uchar*>(frame->data[0]), static_cast<int>(out_data->width), static_cast<int>(out_data->height), 4 * out_data->width, QImage::Format_RGBA8888);
		QImage RGBImage = RGBAImage.scaledToWidth(outputWidth).convertToFormat(QImage::Format_RGB888);

		Image<ColorRgb> outputImage(RGBImage.width(), RGBImage.height());
		for (int y = 0; y < RGBImage.height(); y++)
		{
			memcpy((unsigned char*)outputImage.memptr() + y * outputImage.width() * 3, static_cast<unsigned char*>(RGBImage.scanLine(y)), RGBImage.width() * 3);
		}


		QMetaObject::invokeMethod(out_data->client, "setImage", Qt::QueuedConnection, Q_ARG(Image<ColorRgb>, outputImage));
		pthread_mutex_unlock(&out_data->mutex);
	}
}

struct obs_output_info create_hyperion_output_info()
{
	struct obs_output_info output_info = {};
	output_info.id = "hyperion_output";
	output_info.flags = OBS_OUTPUT_VIDEO;
	output_info.get_name = hyperion_output_getname;
	output_info.create = hyperion_output_create;
	output_info.destroy = hyperion_output_destroy;
	output_info.start = hyperion_output_start;
	output_info.stop = hyperion_output_stop;
	output_info.raw_video = hyperion_output_raw_video;
	return output_info;
};

OBS_DECLARE_MODULE()
OBS_MODULE_AUTHOR(OBS_AUTHOR);
OBS_MODULE_USE_DEFAULT_LOCALE(OBS_MODULE_NAME, OBS_DEFAULT_LOCALE)

bool obs_module_load(void)
{
#ifdef _WIN32
	WSADATA wsad;
	WSAStartup(MAKEWORD(2, 2), &wsad);
#endif

	qRegisterMetaType<Image<ColorRgb>>("Image<ColorRgb>");

	obs_output_info hyperion_output_info = create_hyperion_output_info();
	obs_register_output(&hyperion_output_info);

	obs_data_t *settings = obs_data_create();
	_hyperionOutput = obs_output_create("hyperion_output", OBS_OUTPUT_NAME, settings, nullptr);
	obs_data_release(settings);
	hyperion_signal_init("void stop(string msg, bool running)");
	hyperion_signal_init("void log(string msg)");

	QMainWindow* main_window = static_cast<QMainWindow*>(obs_frontend_get_main_window());
	QAction *action = static_cast<QAction*>(obs_frontend_add_tools_menu_qaction(obs_module_text(OBS_MENU_ID)));

	obs_frontend_push_ui_translation(obs_module_get_string);
	hyperionProperties = new HyperionProperties(main_window);
	obs_frontend_pop_ui_translation();

	auto menu_cb = []
	{
		hyperionProperties->setVisible(!hyperionProperties->isVisible());
	};

	QAction::connect(action, &QAction::triggered, menu_cb);

    return true;
}

void obs_module_unload(void)
{
#ifdef _WIN32
	WSACleanup();
#endif
}

void hyperion_start_streaming(QString& address, int port, int sizeDecimation)
{
	obs_data_t *settings = obs_output_get_settings(_hyperionOutput);
	obs_data_set_string(settings, OBS_SETTINGS_ADDRESS, address.toLocal8Bit().constData());
	obs_data_set_int(settings, OBS_SETTINGS_PORT, port);
	obs_data_set_int(settings, OBS_SETTINGS_SIZEDECIMATION, sizeDecimation);
	obs_output_update(_hyperionOutput, settings);
	obs_data_release(settings);
	obs_output_start(_hyperionOutput);
}

void hyperion_stop_streaming()
{
	obs_output_stop(_hyperionOutput);
}

void hyperion_release()
{
	obs_output_stop(_hyperionOutput);
	obs_output_release(_hyperionOutput);
}

signal_handler_t* hyperion_get_signal_handler()
{
	return obs_output_get_signal_handler(_hyperionOutput);	
}
