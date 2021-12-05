#pragma once

// Qt includes
#include <QString>
#include <QColor>
#include <QTcpSocket>
#include <QTimer>
#include <QMap>
#include <QHostAddress>

// hyperion util
#include "Image.h"
#include "ColorRgb.h"

#include <flatbuffers/flatbuffers.h>

const int FLATBUFFER_DEFAULT_PORT = 19400;

namespace hyperionnet
{
struct Reply;
}

///
/// Connection class to setup an connection to the hyperion server and execute commands.
///
class FlatBufferConnection : public QObject
{

	Q_OBJECT

public:
	///
	/// @brief Constructor
	/// @param host The hostname or IP-address of the Hyperion Flatbuffer server (for example "192.168.0.32")
	/// @param port The port of the Hyperion Flatpuffer server (default is 19400)
	/// @param skipReply  If true skip reply
	///
	FlatBufferConnection(const QString& origin, const QString& host, int priority, bool skipReply, quint16 port = FLATBUFFER_DEFAULT_PORT);

	///
	/// @brief Destructor
	///
	~FlatBufferConnection() override;

	/// @brief Do not read reply messages from Hyperion if set to true
	void setSkipReply(bool skip);

	///
	/// @brief Register a new priority with given origin
	/// @param origin  The user friendly origin string
	/// @param priority The priority to register
	///
	void setRegister(const QString& origin, int priority);

	///
	/// @brief Send a command message and receive its reply
	/// @param message The message to send
	///
	void sendMessage(const uint8_t* buffer, uint32_t size);

public slots:
	///
	/// @brief Set the leds according to the given image
	/// @param image The image
	///
	void setImage(const Image<ColorRgb> &image);

private slots:
	///
	/// @brief Try to connect to the Hyperion host
	///
	void connectToHost();

	///
	/// @brief Slot called when new data has arrived
	///
	void readData();

private:

	///
	/// @brief Parse a reply message
	/// @param reply The received reply
	/// @return true if the reply indicates success
	///
	bool parseReply(const hyperionnet::Reply *reply);

private:
	/// The TCP-Socket with the connection to the server
	QTcpSocket _socket;

	QString _origin;
	int _priority;

	/// Host address
	QString _host;

	/// Host port
	uint16_t _port;

	/// buffer for reply
	QByteArray _receiveBuffer;

	QTimer _timer;
	QAbstractSocket::SocketState  _prevSocketState;
	flatbuffers::FlatBufferBuilder _builder;
	bool _registered;
};
