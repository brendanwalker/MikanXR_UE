#pragma once

#include "MikanAPIExport.h"
#include "MikanAPITypes.h"
#include "MikanScriptTypes.h"
#include "SerializationProperty.h"

#ifdef MIKANAPI_REFLECTION_ENABLED
#include "MikanScriptRequests.rfkh.h"
#endif

struct MIKAN_API STRUCT(Serialization::CodeGenModule("MikanScriptRequest")) SendScriptMessage :
	public MikanRequest
{
public:
	SendScriptMessage()
	{
		MIKAN_REQUEST_TYPE_INFO_INIT(SendScriptMessage)
	}

	FIELD()
	MikanScriptMessageInfo message;

	#ifdef MIKANAPI_REFLECTION_ENABLED
	SendScriptMessage_GENERATED
	#endif
};

#ifdef MIKANAPI_REFLECTION_ENABLED
File_MikanScriptRequests_GENERATED
#endif