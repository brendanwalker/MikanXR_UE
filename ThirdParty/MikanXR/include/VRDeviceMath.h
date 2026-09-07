#pragma once

struct VRDevicePosition
{
	float x= 0.0f;
	float y= 0.0f;
	float z= 0.0f;
};

struct VRDeviceQuat
{
	float w= 1.f;
	float x= 0.f;
	float y= 0.f;
	float z= 0.f;
};

// OpenGL right-handed coordinate system
// +X = Right
// +Y = Up
// -Z = Forward
// World Units = Meters
struct VRDevicePose
{
	VRDevicePosition position;
	VRDeviceQuat orientation;
};