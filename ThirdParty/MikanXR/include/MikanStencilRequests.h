#pragma once

#include "MikanAPIExport.h"
#include "MikanAPITypes.h"
#include "MikanStencilTypes.h"
#include "SerializationProperty.h"

#ifdef MIKANAPI_REFLECTION_ENABLED
#include "MikanStencilRequests.rfkh.h"
#endif

// Stencil Request Types
// ------

struct MIKAN_API STRUCT(Serialization::CodeGenModule("MikanStencilRequest")) GetModelStencilRenderGeometry
	: public MikanRequest
{
public:
	GetModelStencilRenderGeometry(){MIKAN_REQUEST_TYPE_INFO_INIT(GetModelStencilRenderGeometry)}

	FIELD() MikanStencilID stencilId= INVALID_MIKAN_ID;

#ifdef MIKANAPI_REFLECTION_ENABLED
	GetModelStencilRenderGeometry_GENERATED
#endif
};

// Stencil Response Types
// ------

struct MIKAN_API STRUCT(Serialization::CodeGenModule("MikanStencilRequest")) MikanStencilModelRenderGeometryResponse
	: public MikanResponse
{
	MikanStencilModelRenderGeometryResponse(){MIKAN_RESPONSE_TYPE_INFO_INIT(MikanStencilModelRenderGeometryResponse)}

	FIELD() MikanStencilModelRenderGeometry render_geometry;

#ifdef MIKANAPI_REFLECTION_ENABLED
	MikanStencilModelRenderGeometryResponse_GENERATED
#endif
};

#ifdef MIKANAPI_REFLECTION_ENABLED
File_MikanStencilRequests_GENERATED
#endif