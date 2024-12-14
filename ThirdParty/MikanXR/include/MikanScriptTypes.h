#pragma once

#include "MikanAPIExport.h"
#include "SerializationProperty.h"
#include "SerializableString.h"

#ifdef MIKANAPI_REFLECTION_ENABLED
#include "MikanScriptTypes.rfkh.h"
#endif

// Script Event Types
struct STRUCT(Serialization::CodeGenModule("MikanScriptTypes")) MikanScriptMessageInfo 
{
	Serialization::String content;

	#ifdef MIKANAPI_REFLECTION_ENABLED
	MikanScriptMessageInfo_GENERATED
	#endif
};

#ifdef MIKANAPI_REFLECTION_ENABLED
File_MikanScriptTypes_GENERATED
#endif