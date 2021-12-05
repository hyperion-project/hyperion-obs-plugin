#include <obs-frontend-api.h>
#include <obs-module.h>

#include <QMainWindow>
#include <QAction>
#include <QImage>

#include "hyperion-obs.h"
#include "HyperionProperties.h"
#include "FlatBufferConnection.h"
#include "Image.h"

struct hyperion_output
{
	obs_output_t *output = nullptr;
	FlatBufferConnection* client = nullptr;
	bool active = false;
	int width = 0;
	int height = 0;
};

HyperionProperties *hyperionProperties;
obs_output_t* _hyperionOutput;

void hyperion_signal_init(const char *signal)
{
	signal_handler_t *handler = hyperion_get_signal_handler();
	signal_handler_add(handler,signal);
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

int Connect(void *data)
{
	hyperion_output *out_data = (hyperion_output*)data;
	obs_data_t *settings = obs_output_get_settings(out_data->output);

	out_data->client = new FlatBufferConnection("hyperion-obs", obs_data_get_string(settings, "Location"), 150, false, obs_data_get_int(settings, "Port"));
	return 0;
}

static void Disconnect(void *data)
{
	hyperion_output *out_data = (hyperion_output*)data;
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
	hyperion_output *data = (hyperion_output*)bzalloc(sizeof(struct hyperion_output));
	data->output = output;
	UNUSED_PARAMETER(settings);
	return data;
}

static void hyperion_output_destroy(void *data)
{
	hyperion_output *out_data = (hyperion_output*)data;
	if (out_data)
	{
		bfree(out_data);
	}
}

static bool hyperion_output_start(void *data)
{
	hyperion_output *out_data = (hyperion_output*)data;
	out_data->width = (int32_t)obs_output_get_width(out_data->output);
	out_data->height = (int32_t)obs_output_get_height(out_data->output);
	int ret = Connect(data);

	struct video_scale_info conv;
	conv.format = 	VIDEO_FORMAT_RGBA;
	conv.width = (int32_t)obs_output_get_width(out_data->output);
	conv.height = (int32_t)obs_output_get_height(out_data->output);

	obs_output_set_video_conversion(out_data->output, &conv);

	// video_t *video = obs_output_video(out_data->output);
	// image.resize(width, height); // this is set_image_size
	// set_image_size(video_output_get_width(video), video_output_get_width(video));

	// enum video_format format = video_output_get_format(video);

	// double video_frame_rate = video_output_get_frame_rate(video);

	if (!obs_output_can_begin_data_capture(out_data->output, 0))
	{
		return false;
	}

	out_data->active = true;
	return obs_output_begin_data_capture(out_data->output, 0);
}

static void hyperion_output_stop(void *data, uint64_t ts)
{
	hyperion_output *out_data = (hyperion_output*)data;
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
	hyperion_output *out_data = (hyperion_output*)param;
	if(out_data->active)
	{
		Image<ColorRgb> outputImage(out_data->width, out_data->height);
		uint8_t* destMemory = (uint8_t*)outputImage.memptr();
		int destLineSize = outputImage.width() * 3;

		for (int yDest = out_data->height - 1; yDest >= 0; --yDest)
		{
			uint8_t* currentDest = destMemory + destLineSize * yDest;
			uint8_t* endDest = currentDest + destLineSize;
			uint8_t* currentSource = frame->data[0] + frame->linesize[0];

			currentSource += 2;

			while (currentDest < endDest)
			{
				*currentDest++ = *currentSource--;
				*currentDest++ = *currentSource--;
				*currentDest++ = *currentSource;
				currentSource += 6;
			}
		}

		QMetaObject::invokeMethod(out_data->client, "setImage", Qt::QueuedConnection, Q_ARG(Image<ColorRgb>, outputImage));
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
OBS_MODULE_AUTHOR("Hyperion-Project");
OBS_MODULE_USE_DEFAULT_LOCALE("hyperion-obs", "en-US")

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
	_hyperionOutput = obs_output_create("hyperion_output", "HyperionOutput", settings, NULL);
	obs_data_release(settings);
	hyperion_signal_init("void close(string msg, bool running)");

	QMainWindow* main_window = (QMainWindow*)obs_frontend_get_main_window();
	QAction *action = (QAction*)obs_frontend_add_tools_menu_qaction(obs_module_text("Name"));

	obs_frontend_push_ui_translation(obs_module_get_string);
	hyperionProperties = new HyperionProperties(main_window);
	obs_frontend_pop_ui_translation();

	auto menu_cb = []
	{
		hyperionProperties->setVisible(!hyperionProperties->isVisible());
	};

	action->connect(action, &QAction::triggered, menu_cb);

    return true;
}

void obs_module_unload(void)
{
#ifdef _WIN32
	WSACleanup();
#endif
}

void hyperion_enable(const char *location, const int port)
{
	obs_data_t *settings = obs_output_get_settings(_hyperionOutput);
	obs_data_set_string(settings, "Location", location);
	obs_data_set_int(settings, "Port", port);
	obs_output_update(_hyperionOutput, settings);
	obs_data_release(settings);
	obs_output_start(_hyperionOutput);
}

void hyperion_disable()
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
