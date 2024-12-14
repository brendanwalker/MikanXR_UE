#pragma once

#include "MikanAPIExport.h"
#include "MikanAPITypes.h"
#include "MikanVRDeviceTypes.h"
#include "SerializationProperty.h"
#include "SerializableList.h"

#ifdef MIKANAPI_REFLECTION_ENABLED
#include "MikanVRDeviceRequests.rfkh.h"
#endif

// VR Device Request Types
// ------

struct MIKAN_API STRUCT(Serialization::CodeGenModule("MikanScriptRequest")) GetVRDeviceList :
	public MikanRequest
{
public:
	GetVRDeviceList()
	{
		MIKAN_REQUEST_TYPE_INFO_INIT(GetVRDeviceList)
	}

	#ifdef MIKANAPI_REFLECTION_ENABLED
	GetVRDeviceList_GENERATED
	#endif
};

struct MIKAN_API STRUCT(Serialization::CodeGenModule("MikanScriptRequest")) GetVRDeviceInfo :
	public MikanRequest
{
public:
	GetVRDeviceInfo()
	{
		MIKAN_REQUEST_TYPE_INFO_INIT(GetVRDeviceInfo)
	}

	FIELD()
	MikanVRDeviceID deviceId;

	#ifdef MIKANAPI_REFLECTION_ENABLED
	GetVRDeviceInfo_GENERATED
	#endif
};

struct MIKAN_API STRUCT(Serialization::CodeGenModule("MikanScriptRequest")) SubscribeToVRDevicePoseUpdates :
	public MikanRequest
{
public:
	SubscribeToVRDevicePoseUpdates()
	{
		MIKAN_REQUEST_TYPE_INFO_INIT(SubscribeToVRDevicePoseUpdates)
	}

	FIELD()
	MikanVRDeviceID deviceId;

	#ifdef MIKANAPI_REFLECTION_ENABLED
	SubscribeToVRDevicePoseUpdates_GENERATED
	#endif
};

struct MIKAN_API STRUCT(Serialization::CodeGenModule("MikanScriptRequest")) UnsubscribeFromVRDevicePoseUpdates :
	public MikanRequest
{
public:
	UnsubscribeFromVRDevicePoseUpdates()
	{
		MIKAN_REQUEST_TYPE_INFO_INIT(UnsubscribeFromVRDevicePoseUpdates)
	}

	FIELD()
	MikanVRDeviceID deviceId;

	#ifdef MIKANAPI_REFLECTION_ENABLED
	UnsubscribeFromVRDevicePoseUpdates_GENERATED
	#endif
};

// VR Device Response Types
// ------

struct STRUCT(Serialization::CodeGenModule("MikanScriptRequest")) MikanVRDeviceListResponse : 
	public MikanResponse
{
	MikanVRDeviceListResponse()
	{
		MIKAN_RESPONSE_TYPE_INFO_INIT(MikanVRDeviceListResponse)
	}

	FIELD()
	Serialization::List<MikanVRDeviceID> vr_device_id_list;

	#ifdef MIKANAPI_REFLECTION_ENABLED
	MikanVRDeviceListResponse_GENERATED
	#endif
};

struct STRUCT(Serialization::CodeGenModule("MikanScriptRequest")) MikanVRDeviceInfoResponse : 
	public MikanResponse
{
	MikanVRDeviceInfoResponse()
	{
		MIKAN_RESPONSE_TYPE_INFO_INIT(MikanVRDeviceInfoResponse)
	}

	FIELD()
	MikanVRDeviceInfo vr_device_info;

	#ifdef MIKANAPI_REFLECTION_ENABLED
	MikanVRDeviceInfoResponse_GENERATED
	#endif
};


#ifdef MIKANAPI_REFLECTION_ENABLED
File_MikanVRDeviceRequests_GENERATED
#endif