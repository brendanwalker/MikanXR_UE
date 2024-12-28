#pragma once

#include "MikanAPIExport.h"
#include "MikanAPITypes.h"
#include "SerializationProperty.h"

#ifdef MIKANAPI_REFLECTION_ENABLED
#include "MikanRenderTargetRequests.rfkh.h"
#endif

struct MIKAN_API STRUCT(Serialization::CodeGenModule("MikanRenderTargetRequest")) AllocateRenderTargetTextures : 
	public MikanRequest
{
public:
	AllocateRenderTargetTextures()
	{
		MIKAN_REQUEST_TYPE_INFO_INIT(AllocateRenderTargetTextures)
	}

	FIELD()
	MikanRenderTargetDescriptor descriptor;

	#ifdef MIKANAPI_REFLECTION_ENABLED
	AllocateRenderTargetTextures_GENERATED
	#endif
};

struct MIKAN_API STRUCT(Serialization::CodeGenModule("MikanRenderTargetRequest")) WriteColorRenderTargetTexture :
	public MikanRequest
{
public:
	WriteColorRenderTargetTexture()
	{
		MIKAN_REQUEST_TYPE_INFO_INIT(WriteColorRenderTargetTexture)
	}

	FIELD()
	void* apiColorTexturePtr= nullptr;

	#ifdef MIKANAPI_REFLECTION_ENABLED
	WriteColorRenderTargetTexture_GENERATED
	#endif
};

struct MIKAN_API STRUCT(Serialization::CodeGenModule("MikanRenderTargetRequest")) WriteDepthRenderTargetTexture :
	public MikanRequest
{
public:
	WriteDepthRenderTargetTexture()
	{
		MIKAN_REQUEST_TYPE_INFO_INIT(WriteDepthRenderTargetTexture)
	}

	FIELD()
	void* apiDepthTexturePtr= nullptr;

	FIELD()
	float zNear= 0.f;

	FIELD()
	float zFar= 0.f;

	#ifdef MIKANAPI_REFLECTION_ENABLED
	WriteDepthRenderTargetTexture_GENERATED
	#endif
};

struct MIKAN_API STRUCT(Serialization::CodeGenModule("MikanRenderTargetRequest")) PublishRenderTargetTextures :
	public MikanRequest
{
public:
	PublishRenderTargetTextures()
	{
		MIKAN_REQUEST_TYPE_INFO_INIT(PublishRenderTargetTextures)
	}

	FIELD()
	int64_t frameIndex= 0;

	#ifdef MIKANAPI_REFLECTION_ENABLED
	PublishRenderTargetTextures_GENERATED
	#endif
};

struct MIKAN_API STRUCT(Serialization::CodeGenModule("MikanRenderTargetRequest")) FreeRenderTargetTextures :
	public MikanRequest
{
public:
	FreeRenderTargetTextures()
	{
		MIKAN_REQUEST_TYPE_INFO_INIT(FreeRenderTargetTextures)
	}

	#ifdef MIKANAPI_REFLECTION_ENABLED
	FreeRenderTargetTextures_GENERATED
	#endif
};


#ifdef MIKANAPI_REFLECTION_ENABLED
File_MikanRenderTargetRequests_GENERATED
#endif