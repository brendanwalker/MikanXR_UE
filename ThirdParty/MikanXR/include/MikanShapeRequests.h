#pragma once

#include "MikanAPIExport.h"
#include "MikanAPITypes.h"
#include "MikanShapeTypes.h"
#include "MikanStencilTypes.h"
#include "SerializationProperty.h"

#ifdef MIKANAPI_REFLECTION_ENABLED
#include "MikanShapeRequests.rfkh.h"
#endif

// Shape Request Types
// ------

struct MIKAN_API STRUCT(Serialization::CodeGenModule("MikanShapeRequest")) GetModelShapeRenderGeometry
	: public MikanRequest
{
public:
	GetModelShapeRenderGeometry(){MIKAN_REQUEST_TYPE_INFO_INIT(GetModelShapeRenderGeometry)}

	FIELD() MikanShapeID shapeId= INVALID_MIKAN_ID;

#ifdef MIKANAPI_REFLECTION_ENABLED
	GetModelShapeRenderGeometry_GENERATED
#endif
};

// Shape Response Types
// ------

struct MIKAN_API STRUCT(Serialization::CodeGenModule("MikanShapeRequest")) MikanShapeModelRenderGeometryResponse
	: public MikanResponse
{
	MikanShapeModelRenderGeometryResponse(){MIKAN_RESPONSE_TYPE_INFO_INIT(MikanShapeModelRenderGeometryResponse)}

	FIELD() MikanStencilModelRenderGeometry render_geometry;

#ifdef MIKANAPI_REFLECTION_ENABLED
	MikanShapeModelRenderGeometryResponse_GENERATED
#endif
};

#ifdef MIKANAPI_REFLECTION_ENABLED
File_MikanShapeRequests_GENERATED
#endif
