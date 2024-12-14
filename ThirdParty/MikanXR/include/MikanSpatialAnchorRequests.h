#pragma once

#include "MikanAPIExport.h"
#include "MikanAPITypes.h"
#include "MikanMathTypes.h"
#include "MikanSpatialAnchorTypes.h"
#include "SerializationProperty.h"
#include "SerializableString.h"
#include "SerializableList.h"

#ifdef MIKANAPI_REFLECTION_ENABLED
#include "MikanSpatialAnchorRequests.rfkh.h"
#endif

// Spatial Anchor Request Types
struct MIKAN_API STRUCT(Serialization::CodeGenModule("MikanSpatialAnchorRequest")) GetSpatialAnchorList :
	public MikanRequest
{
public:
	GetSpatialAnchorList()
	{
		MIKAN_REQUEST_TYPE_INFO_INIT(GetSpatialAnchorList)
	}

	#ifdef MIKANAPI_REFLECTION_ENABLED
	GetSpatialAnchorList_GENERATED
	#endif
};

struct MIKAN_API STRUCT(Serialization::CodeGenModule("MikanSpatialAnchorRequest")) GetSpatialAnchorInfo :
	public MikanRequest
{
public:
	GetSpatialAnchorInfo()
	{
		MIKAN_REQUEST_TYPE_INFO_INIT(GetSpatialAnchorInfo)
	}

	FIELD()
	MikanSpatialAnchorID anchorId;

	#ifdef MIKANAPI_REFLECTION_ENABLED
	GetSpatialAnchorInfo_GENERATED
	#endif
};

struct MIKAN_API STRUCT(Serialization::CodeGenModule("MikanSpatialAnchorRequest")) FindSpatialAnchorInfoByName :
	public MikanRequest
{
public:
	FindSpatialAnchorInfoByName()
	{
		MIKAN_REQUEST_TYPE_INFO_INIT(FindSpatialAnchorInfoByName)
	}

	FIELD()
	Serialization::String anchorName;

	#ifdef MIKANAPI_REFLECTION_ENABLED
	FindSpatialAnchorInfoByName_GENERATED
	#endif
};

// Spatial Anchor Response Types
struct STRUCT(Serialization::CodeGenModule("MikanSpatialAnchorListResponse")) MikanSpatialAnchorListResponse : 
	public MikanResponse
{
	MikanSpatialAnchorListResponse()
	{
		MIKAN_RESPONSE_TYPE_INFO_INIT(MikanSpatialAnchorListResponse)
	}

	FIELD()
	Serialization::List<MikanSpatialAnchorID> spatial_anchor_id_list;

	#ifdef MIKANAPI_REFLECTION_ENABLED
	MikanSpatialAnchorListResponse_GENERATED
	#endif
};

struct STRUCT(Serialization::CodeGenModule("MikanSpatialAnchorInfoResponse")) MikanSpatialAnchorInfoResponse : 
	public MikanResponse
{
	MikanSpatialAnchorInfoResponse()
	{
		MIKAN_RESPONSE_TYPE_INFO_INIT(MikanSpatialAnchorInfoResponse)
	}

	FIELD()
	MikanSpatialAnchorInfo anchor_info;

	#ifdef MIKANAPI_REFLECTION_ENABLED
	MikanSpatialAnchorInfoResponse_GENERATED
	#endif
};

#ifdef MIKANAPI_REFLECTION_ENABLED
File_MikanSpatialAnchorRequests_GENERATED
#endif