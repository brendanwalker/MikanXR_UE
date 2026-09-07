#pragma once

#include "VRDeviceMath.h"
#include "MkRendererFwd.h"

// -- includes -----
enum class eVRDeviceType : int
{
	INVALID,
	HMD,
	VRTracker,
	VRController,

	COUNT
};

// -- definitions -----
class IVRDeviceSocket
{
public:
	IVRDeviceSocket()= default;
	virtual ~IVRDeviceSocket() {}

	virtual const char* getName() const= 0;
	virtual bool getSocketState(VRDevicePose& outRelativePose) const= 0;
};

class IVRDeviceMesh
{
public:
	IVRDeviceMesh()= default;
	virtual ~IVRDeviceMesh() {}

	virtual const char* getName() const= 0;
	virtual bool getMeshState(VRDevicePose& outRelativePose, bool& outIsVisible) const= 0;
	virtual IMkTriangulatedMeshConstPtr getTriangulatedMesh() const= 0;
	virtual IMkWireframeMeshConstPtr getWireframeMesh() const= 0;
};

/// VRDevice interface
class IVRDevice
{
public:
	IVRDevice()= default;
	virtual ~IVRDevice() {}

	// -- Device Properties
	virtual size_t getDeviceIndex() const= 0;
	virtual eVRDeviceType getDeviceType() const= 0;
	virtual const char* getDevicePath() const= 0;
	virtual bool getDevicePose(int vrFrameDelay, VRDevicePose& outPose) const= 0;
	virtual const char* getSerialNumber() const= 0;
	virtual const char* getTrackingSystem() const= 0;
	virtual const char* getTrackerRole() const= 0;
	virtual const char* getModelLabel() const= 0;
	virtual const char* getModelNumber() const= 0;
	virtual const char* getManufacturerName() const= 0;

	// -- Socket accessors
	virtual size_t getSocketCount() const= 0;
	virtual IVRDeviceSocket* geSocketByIndex(size_t socketIndex) const= 0;
	virtual IVRDeviceSocket* getSocketByName(const char* socketName) const= 0;

	// -- Mesh accessors
	virtual size_t getMeshCount() const= 0;
	virtual IVRDeviceMesh* getMeshByIndex(size_t meshIndex) const= 0;
	virtual IVRDeviceMesh* getMeshByName(const char* meshName) const= 0;
};

using IVRDevicePtr= std::shared_ptr<IVRDevice>;
using IVRDeviceConstPtr= std::shared_ptr<const IVRDevice>;
using IVRDeviceWeakPtr= std::weak_ptr<IVRDevice>;
