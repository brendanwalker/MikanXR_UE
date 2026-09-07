#pragma once

// -- includes -----
#include "IVideoDevice.h"

#include <cstdint>
#include <memory>

// -- definitions -----

struct ARKitVideoConnectionSettings
{
	// Video RTP on basePort+0, carrying frame-coupled pose in its own per-packet
	// header extension (see ARKitRTPHeaderExtension.h) - there is no longer a
	// separate pose UDP channel.
	uint16_t basePort= 0;
};

struct ARKitPoseFrameBuffer
{
	float transform[16]= {}; // row-major 4x4, camera-to-world
	float fx= 0.f;
	float fy= 0.f;
	float cx= 0.f;
	float cy= 0.f;
	float imageWidth= 0.f;
	float imageHeight= 0.f;
};

// A bundle of video + (optional) pose for the same frameSeq, built directly from
// the video RTP stream's per-packet header extension (see
// ARKitRTPHeaderExtension.h in the MikanARKitVideo plugin) - hasVideo is always
// true when this bundle is dispatched; hasPose reflects whether the sender
// attached the pose-bearing extension payload to this particular packet (see
// IFrameCoupledPoseProvider, ticket E4). Pose can no longer arrive decoupled from
// video the way it could under the old separate-pose-channel design, since both
// now travel in the same RTP packet.
struct ARKitVideoFrameBundle
{
	uint32_t frameSeq= 0;
	uint64_t timestampUs= 0;

	bool hasVideo= false;
	const uint8_t* videoData= nullptr; // decoded frame buffer, valid only during the callback
	int videoWidth= 0;
	int videoHeight= 0;

	bool hasPose= false;
	ARKitPoseFrameBuffer pose;
};

class IARKitVideoDeviceListener
{
public:
	// Called when the async open has completed successfully
	virtual void notifyDeviceOpened(const class IARKitVideoDevice* device) {}

	// Called when the video source has been disconnected
	virtual void notifyDeviceClosed(const class IARKitVideoDevice* device)= 0;

	// Called when a decoded video frame (with optional pose) is ready for a frameSeq
	virtual void notifyFrameBundleReceived(const ARKitVideoFrameBundle& bundle)= 0;
};

// ARKitVideoDevice interface
class IARKitVideoDevice : public IVideoDevice
{
public:
	IARKitVideoDevice()= default;
	virtual ~IARKitVideoDevice() {}

	virtual void update(float deltaSeconds)= 0;

	// -- Device Listener
	virtual void addListener(IARKitVideoDeviceListener* listener)= 0;
	virtual void removeListener(IARKitVideoDeviceListener* listener)= 0;

	// -- Stream Properties
	virtual const char* getDevicePath() const= 0;
	virtual const char* getFriendlyName() const= 0;

	// -- Device Activation
	virtual eVideoOpeningStatus getVideoOpeningStatus() const= 0;
	virtual eVideoOpeningStatus open()= 0;
	virtual void close()= 0;

	// -- Video Streaming
	virtual eVideoStreamingStatus startVideoStream()= 0;
	virtual eVideoStreamingStatus getVideoStreamingStatus() const= 0;
	virtual void stopVideoStream()= 0;

	// -- Zero-copy CUDA-GL texture access (ticket E3; NV12 planes as of "Phase 6") --
	// Return the GL texture id (name) of the most recently decoded frame's luma
	// (Y, full-resolution, single-channel) and chroma (UV, half-resolution,
	// two-channel interleaved) planes, or 0 if none has arrived yet. This plugin
	// has no GL context/shader-cache access of its own (see MikanARKitVideoDevice's
	// own notes), so it stops at exposing the raw decoded planes - it does not
	// produce a displayable RGBA texture itself. MikanCoreApp (this header's
	// library) is also a lower layer than MikanRenderer - it can't depend on
	// IMkTexturePtr - so both cross the boundary as raw GL texture ids;
	// IARKitVideoDeviceListener's caller (Editor-side code, which does depend on
	// MikanRenderer and does have shader-cache access) is expected to wrap each via
	// IMkExternalTexture::setExternalPlatformTexture(&glId) and run its own
	// NV12->RGBA GLSL conversion pass (see ARKitVideoSourceComponent::
	// getDirectColorTexture/processDirectVideoFrame). Each texture is written in
	// place every frame (CUDA WRITE_DISCARD - see CudaGLInterop.h), so the same
	// non-zero id remains valid to keep reusing across frames; only a
	// resize/reopen changes it. Must be called from the GL-context-owning thread,
	// same as every other CUDA-GL interop call in this pipeline.
	virtual uint32_t getLumaTextureGlId() const= 0;
	virtual uint32_t getChromaTextureGlId() const= 0;
};

using IARKitVideoDevicePtr= std::shared_ptr<IARKitVideoDevice>;
using IARKitVideoDeviceConstPtr= std::shared_ptr<const IARKitVideoDevice>;
using IARKitVideoDeviceWeakPtr= std::weak_ptr<IARKitVideoDevice>;
