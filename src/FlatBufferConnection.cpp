// stl includes
#include <stdexcept>

// Qt includes
#include <QRgb>

// flatbuffer includes
#include "FlatBufferConnection.h"

// flatbuffer FBS
#include "hyperion_reply_generated.h"
#include "hyperion_request_generated.h"

#define QSTRING_CSTR(str) str.toLocal8Bit().constData()

FlatBufferConnection::FlatBufferConnection(const QString& origin, const QString& host, int priority, quint16 port)
	: _socket()
	, _origin(origin)
	, _priority(priority)
	, _host(host)
	, _port(port)
	, _prevSocketState(QAbstractSocket::UnconnectedState)
	, _registered(false)
{
	connect(&_socket, &QTcpSocket::readyRead, this, &FlatBufferConnection::readData, Qt::UniqueConnection);
	connect(&_socket, &QTcpSocket::disconnected, this, &FlatBufferConnection::disconnected);

	// init connect
	connectToHost();

	// start the connection timer
	_timer.setInterval(5000);

	connect(&_timer, &QTimer::timeout, this, &FlatBufferConnection::connectToHost);
	_timer.start();
}

FlatBufferConnection::~FlatBufferConnection()
{
	_timer.stop();
	_socket.close();
}

void FlatBufferConnection::readData()
{
	_receiveBuffer += _socket.readAll();

	// check if we can read a header
	while(_receiveBuffer.size() >= 4)
	{
		uint32_t messageSize =
			((_receiveBuffer[0]<<24) & 0xFF000000) |
			((_receiveBuffer[1]<<16) & 0x00FF0000) |
			((_receiveBuffer[2]<< 8) & 0x0000FF00) |
			((_receiveBuffer[3]    ) & 0x000000FF);

		// check if we can read a complete message
		if((uint32_t) _receiveBuffer.size() < messageSize + 4) return;

		// extract message only and remove header + msg from buffer :: QByteArray::remove() does not return the removed data
		const QByteArray msg = _receiveBuffer.mid(4, messageSize);
		_receiveBuffer.remove(0, messageSize + 4);

		const uint8_t* msgData = reinterpret_cast<const uint8_t*>(msg.constData());
		flatbuffers::Verifier verifier(msgData, messageSize);

		if (hyperionnet::VerifyReplyBuffer(verifier))
		{
			const hyperionnet::Reply* reply = hyperionnet::GetReply(msgData);
			if (!parseReply(reply))
			{
				struct calldata call_data;
				calldata_init(&call_data);
				calldata_set_string(&call_data, "msg", QSTRING_CSTR(QString("Reply received with error: %1").arg(reply->error()->c_str())));
				calldata_set_bool(&call_data, "running", true);
				signal_handler_t *handler = hyperion_get_signal_handler();
				signal_handler_signal(handler, "stop", &call_data);
				calldata_free(&call_data);
				delete this;
			}
			continue;
		}

		emit logMessage("Unable to parse reply");
	}
}

void FlatBufferConnection::setRegister(const QString& origin, int priority)
{
	auto registerReq = hyperionnet::CreateRegister(_builder, _builder.CreateString(QSTRING_CSTR(origin)), priority);
	auto req = hyperionnet::CreateRequest(_builder, hyperionnet::Command_Register, registerReq.Union());

	_builder.Finish(req);
	uint32_t size = _builder.GetSize();
	const uint8_t header[] = {
		uint8_t((size >> 24) & 0xFF),
		uint8_t((size >> 16) & 0xFF),
		uint8_t((size >>  8) & 0xFF),
		uint8_t((size	  ) & 0xFF)};

	// write message
	int count = 0;
	count += _socket.write(reinterpret_cast<const char *>(header), 4);
	count += _socket.write(reinterpret_cast<const char *>(_builder.GetBufferPointer()), size);
	_socket.flush();
	_builder.Clear();
}

void FlatBufferConnection::setImage(const Image<ColorRgb> &image)
{
	auto imgData = _builder.CreateVector(reinterpret_cast<const uint8_t*>(image.memptr()), image.size());
	auto rawImg = hyperionnet::CreateRawImage(_builder, imgData, image.width(), image.height());
	auto imageReq = hyperionnet::CreateImage(_builder, hyperionnet::ImageType_RawImage, rawImg.Union(), -1);
	auto req = hyperionnet::CreateRequest(_builder,hyperionnet::Command_Image,imageReq.Union());

	_builder.Finish(req);
	sendMessage(_builder.GetBufferPointer(), _builder.GetSize());
	_builder.Clear();
}

void FlatBufferConnection::connectToHost()
{
	// try connection only when
	if (_socket.state() == QAbstractSocket::UnconnectedState)
	   _socket.connectToHost(_host, _port);
}

void FlatBufferConnection::sendMessage(const uint8_t* buffer, uint32_t size)
{
	// print out connection message only when state is changed
	if (_socket.state() != _prevSocketState )
	{
		_registered = false;
		switch (_socket.state() )
		{
			case QAbstractSocket::UnconnectedState:
				emit logMessage(QString("No connection to Hyperion: %1:%2").arg(_host).arg(_port));
				break;
			case QAbstractSocket::ConnectedState:
				emit logMessage(QString("Connected to Hyperion: %1:%2").arg(_host).arg(_port));
				break;
			default:
				emit logMessage(QString("Connecting to Hyperion: %1:%2").arg(_host).arg(_port));
				break;
	  }
	  _prevSocketState = _socket.state();
	}

	if (_socket.state() == QAbstractSocket::ConnectedState)
	{
		if(!_registered)
		{
			setRegister(_origin, _priority);
		}
		else
		{
			const uint8_t header[] = {
				uint8_t((size >> 24) & 0xFF),
				uint8_t((size >> 16) & 0xFF),
				uint8_t((size >>  8) & 0xFF),
				uint8_t((size	  ) & 0xFF)};

			// write message
			int count = 0;
			count += _socket.write(reinterpret_cast<const char *>(header), 4);
			count += _socket.write(reinterpret_cast<const char *>(buffer), size);
			_socket.flush();
		}
	}
}

bool FlatBufferConnection::parseReply(const hyperionnet::Reply *reply)
{
	if (reply->error() == NULL)
	{
		if (_registered)
		{
			return true;
		}

		const auto registered = reply->registered();
		if (registered == _priority)
		{
			_registered = true;
		}
		else
		{
			_registered = false;
		}
		return true;
	}
	return false;
}

void FlatBufferConnection::disconnected()
{
	struct calldata call_data;
	calldata_init(&call_data);
	calldata_set_string(&call_data, "msg", "Connection to Hyperion server was closed");
	calldata_set_bool(&call_data, "running", true);
	signal_handler_t *handler = hyperion_get_signal_handler();
	signal_handler_signal(handler, "stop", &call_data);
	calldata_free(&call_data);

	_timer.stop();
	_socket.close();
}

void FlatBufferConnection::logMessage(const QString& message)
{
	struct calldata call_data;
	calldata_init(&call_data);
	calldata_set_string(&call_data, "msg", message.toStdString().c_str());
	signal_handler_t *handler = hyperion_get_signal_handler();
	signal_handler_signal(handler, "log", &call_data);
	calldata_free(&call_data);
}
