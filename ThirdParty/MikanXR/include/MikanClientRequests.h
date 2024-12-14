#pragma once

#include "MikanAPIExport.h"
#include "MikanAPITypes.h"
#include "MikanClientTypes.h"
#include "SerializationProperty.h"

#ifdef MIKANAPI_REFLECTION_ENABLED
#include "MikanClientRequests.rfkh.h"
#endif

struct MIKAN_API STRUCT(Serialization::CodeGenModule("MikanClientRequests")) InitClientRequest :
	public MikanRequest
{
public:
	InitClientRequest()
	{
		MIKAN_REQUEST_TYPE_INFO_INIT(InitClientRequest)
	}

	FIELD()
	MikanClientInfo clientInfo;

	#ifdef MIKANAPI_REFLECTION_ENABLED
	InitClientRequest_GENERATED
	#endif
};

struct MIKAN_API STRUCT(Serialization::CodeGenModule("MikanClientRequests")) DisposeClientRequest :
	public MikanRequest
{
public:
	DisposeClientRequest()
	{
		MIKAN_REQUEST_TYPE_INFO_INIT(DisposeClientRequest)
	}

	FIELD()
	Serialization::String clientId;

	#ifdef MIKANAPI_REFLECTION_ENABLED
	DisposeClientRequest_GENERATED
	#endif
};

#ifdef MIKANAPI_REFLECTION_ENABLED
File_MikanClientRequests_GENERATED
#endif