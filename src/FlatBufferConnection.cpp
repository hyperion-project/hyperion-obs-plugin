// stl includes
#include <stdexcept>

// Qt includes
#include <QRgb>

// flatbuffer includes
#include "FlatBufferConnection.h"

// flatbuffer FBS
#include "hyperion_reply_generated.h"
#include "hyperion_request_generated.h"

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
	connect(&_socket, &QTcpSocket::disconnected, this, &FlatBufferConnection::serverDisconnected);

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
				emit logMessage(QString("Reply received with error: %1").arg(reply->error()->c_str()));
			}
			continue;
		}

		emit logMessage("Unable to parse reply");
	}
}

void FlatBufferConnection::setRegister(const QString& origin, int priority)
{
	auto registerReq = hyperionnet::CreateRegister(_builder, _builder.CreateString(origin.toLocal8Bit().constData()), priority);
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
	QAbstractSocket::SocketState socketState = _socket.state();
	if (socketState != _prevSocketState )
	{
		_registered = false;
		switch (socketState)
		{
			case QAbstractSocket::UnconnectedState:
				emit logMessage(QString("No connection to %1:%2").arg(_host).arg(_port));
				break;
			case QAbstractSocket::ConnectingState:
				emit logMessage(QString("Connecting to %1, port: %2").arg(_host).arg(_port));
				break;
			case QAbstractSocket::ConnectedState:
				emit logMessage(QString("Connected to %1, port: %2").arg(_host).arg(_port));
				break;
			case QAbstractSocket::ClosingState:
				emit logMessage(QString("Closing connection to %1, port: %2").arg(_host).arg(_port));
				break;
			default:
				break;

		}
	  	_prevSocketState = socketState;
	}

	if (socketState == QAbstractSocket::ConnectedState)
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
