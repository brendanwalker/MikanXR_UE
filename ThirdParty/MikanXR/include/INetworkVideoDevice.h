#pragma once

// -- includes -----
#include "IVideoDevice.h"

#include <memory>

enum class eNetworkVideoProtocol : int
{
	INVALID= -1,

	RTMP,
	RTSP,

	COUNT
};

struct NetworkVideoConnectionSettings
{
	eNetworkVideoProtocol protocol;
	const char* address;
	const char* path;
	int port;
};

struct NetworkVideoStreamProperties
{
	char name[256];               // name of the video mode (e.g., "1080p30", "720p60")
	char bufferFormat[32];        // format of the video frame buffer (e.g., "YUY2", "MJPEG")
	VideoColorimetry colorimetry; // Color conversion information base on format
	int width;                    // width of the video frame in pixels
	int height;                   // height of the video frame in pixels
	int stride;                   // stride is the number of bytes per row in the video frame buffer
	int frame_rate_numerator;     // numerator of the frame rate (e.g., 30 for 30 fps)
	int frame_rate_demonenator;   // denominator of the frame rate (e.g., 1 for 30 fps)
};

struct NetworkVideoFrameBuffer
{
	int width;  // width of the video frame in pixels
	int height; // height of the video frame in pixels
	const uint8_t* data;
	size_t byte_count;
};

class INetworkVideoDeviceListener
{
public:
	// Called when the async open has completed successfully
	virtual void notifyVideoDeviceOpened(const class INetworkVideoDevice* device) {}

	// Called when the video source has been disconnected
	virtual void notifyVideoDeviceClosed(const class INetworkVideoDevice* device)= 0;

	// Called when the video source has updated its dimensions or other properties
	virtual void notifyVideoModePropertiesChanged(const class INetworkVideoDevice* device)= 0;

	// Called when new video frame has been received from the video source
	virtual void notifyVideoFrameReceived(const NetworkVideoFrameBuffer& bufferInfo)= 0;
};

// UsbVideoDevice interface
class INetworkVideoDevice : public IVideoDevice
{
public:
	INetworkVideoDevice()= default;
	virtual ~INetworkVideoDevice() {}

	virtual void update(float deltaSeconds)= 0;

	// -- Device Listener
	virtual void addListener(INetworkVideoDeviceListener* listener)= 0;
	virtual void removeListener(INetworkVideoDeviceListener* listener)= 0;

	// -- Stream Properties
	virtual const char* getDevicePath() const= 0;
	virtual const char* getFriendlyName() const= 0;
	virtual bool getStreamProperties(NetworkVideoStreamProperties& outProperties) const= 0;

	// -- Device Activation
	virtual eVideoOpeningStatus getVideoOpeningStatus() const= 0;
	virtual eVideoOpeningStatus open()= 0;
	virtual void close()= 0;

	// -- Video Streaming
	virtual eVideoStreamingStatus startVideoStream()= 0;
	virtual eVideoStreamingStatus getVideoStreamingStatus() const= 0;
	virtual void stopVideoStream()= 0;
};

using INetworkVideoDevicePtr= std::shared_ptr<INetworkVideoDevice>;
using INetworkVideoDeviceConstPtr= std::shared_ptr<const INetworkVideoDevice>;
using INetworkVideoDeviceWeakPtr= std::weak_ptr<INetworkVideoDevice>;
