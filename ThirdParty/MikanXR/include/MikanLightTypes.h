#pragma once

#include "MikanAPIExport.h"
#include "MikanAPITypes.h"
#include "MikanMathTypes.h"
#include "MikanPropertyTypes.h"
#include "MikanTransformTypes.h"
#include "SerializableList.h"
#include "SerializableString.h"
#include "SerializationProperty.h"

#ifdef MIKANAPI_REFLECTION_ENABLED
#include "MikanLightTypes.rfkh.h"
#endif

// -- System Values --

struct MIKAN_API STRUCT(Serialization::CodeGenModule("MikanLightTypes")) MikanDMXObjectSystemValues
	: public MikanSystemValues
{
	static const char* k_systemName;

	FIELD() Serialization::String network_interface_ip;

	FIELD() uint8_t dmx_priority= 100;

	FIELD() float transmit_rate_hz= 44.0f;

#ifdef MIKANAPI_REFLECTION_ENABLED
	MikanDMXObjectSystemValues_GENERATED
#endif
};

struct MIKAN_API STRUCT(Serialization::CodeGenModule("MikanLightTypes")) MikanRGBSpotLightSystemValues
	: public MikanSystemValues
{
	static const char* k_systemName;

#ifdef MIKANAPI_REFLECTION_ENABLED
	MikanRGBSpotLightSystemValues_GENERATED
#endif
};

struct MIKAN_API STRUCT(Serialization::CodeGenModule("MikanLightTypes")) MikanRGBPixelGridSystemValues
	: public MikanSystemValues
{
	static const char* k_systemName;

#ifdef MIKANAPI_REFLECTION_ENABLED
	MikanRGBPixelGridSystemValues_GENERATED
#endif
};

struct MIKAN_API STRUCT(Serialization::CodeGenModule("MikanLightTypes")) MikanDMXFixtureGroupSystemValues
	: public MikanSystemValues
{
	static const char* k_systemName;

#ifdef MIKANAPI_REFLECTION_ENABLED
	MikanDMXFixtureGroupSystemValues_GENERATED
#endif
};

struct MIKAN_API STRUCT(Serialization::CodeGenModule("MikanLightTypes")) MikanDMXPresetSystemValues
	: public MikanSystemValues
{
	static const char* k_systemName;

#ifdef MIKANAPI_REFLECTION_ENABLED
	MikanDMXPresetSystemValues_GENERATED
#endif
};

struct MIKAN_API STRUCT(Serialization::CodeGenModule("MikanLightTypes")) MikanDMXSequenceSystemValues
	: public MikanSystemValues
{
	static const char* k_systemName;

#ifdef MIKANAPI_REFLECTION_ENABLED
	MikanDMXSequenceSystemValues_GENERATED
#endif
};

struct MIKAN_API STRUCT(Serialization::CodeGenModule("MikanLightTypes")) MikanLightEnvironmentSystemValues
	: public MikanSystemValues
{
	static const char* k_systemName;

#ifdef MIKANAPI_REFLECTION_ENABLED
	MikanLightEnvironmentSystemValues_GENERATED
#endif
};

// -- Component Values --

/// Base values shared by all DMX fixture types.
struct MIKAN_API STRUCT(Serialization::CodeGenModule("MikanLightTypes")) MikanDMXFixtureComponentValues
	: public MikanTransformComponentValues
{
	FIELD() MikanStageID stage_id= INVALID_MIKAN_ID;

	FIELD() uint16_t dmx_universe= 1;

	FIELD() uint16_t dmx_start_channel= 1;

	FIELD() uint16_t dmx_channel_count= 3;

	FIELD() bool is_disabled= false;

#ifdef MIKANAPI_REFLECTION_ENABLED
	MikanDMXFixtureComponentValues_GENERATED
#endif
};

struct MIKAN_API STRUCT(Serialization::CodeGenModule("MikanLightTypes")) MikanRGBSpotLightComponentValues
	: public MikanDMXFixtureComponentValues
{
	static const char* k_componentClassName;
	static const char* k_ownerSystemName;

	FIELD() float cone_angle_degrees= 0.f;

	FIELD() float cone_range_meters= 0.f;

#ifdef MIKANAPI_REFLECTION_ENABLED
	MikanRGBSpotLightComponentValues_GENERATED
#endif
};

/// Pixel grid values — pixel data is NOT included (use SetLightDMXData request for bulk writes).
struct MIKAN_API STRUCT(Serialization::CodeGenModule("MikanLightTypes")) MikanRGBPixelGridComponentValues
	: public MikanDMXFixtureComponentValues
{
	static const char* k_componentClassName;
	static const char* k_ownerSystemName;

	FIELD() int grid_columns= 8;

	FIELD() int grid_rows= 8;

#ifdef MIKANAPI_REFLECTION_ENABLED
	MikanRGBPixelGridComponentValues_GENERATED
#endif
};

/// A named set of DMX fixtures on one stage, the unit presets and sequences address.
struct MIKAN_API STRUCT(Serialization::CodeGenModule("MikanLightTypes")) MikanDMXFixtureGroupComponentValues
	: public MikanComponentValues
{
	static const char* k_componentClassName;
	static const char* k_ownerSystemName;

	FIELD() MikanStageID stage_id= INVALID_MIKAN_ID;

	/// Component ids of the member fixtures (spot lights and pixel grids)
	FIELD() Serialization::List<MikanLightID> fixture_ids;

#ifdef MIKANAPI_REFLECTION_ENABLED
	MikanDMXFixtureGroupComponentValues_GENERATED
#endif
};

/// A fixed set of DMX channel bytes for the fixtures of one group. The bytes
/// are one slice per fixture id, channel_counts long, concatenated in order.
struct MIKAN_API STRUCT(Serialization::CodeGenModule("MikanLightTypes")) MikanDMXPresetComponentValues
	: public MikanComponentValues
{
	static const char* k_componentClassName;
	static const char* k_ownerSystemName;

	FIELD() MikanDMXFixtureGroupID group_id= INVALID_MIKAN_ID;

	FIELD() Serialization::List<MikanLightID> fixture_ids;

	FIELD() Serialization::List<int> channel_counts;

	FIELD() Serialization::List<uint8_t> channel_data;

#ifdef MIKANAPI_REFLECTION_ENABLED
	MikanDMXPresetComponentValues_GENERATED
#endif
};

/// A Lua-driven animation of one fixture group. playback_state is 0 stopped,
/// 1 playing, 2 paused.
struct MIKAN_API STRUCT(Serialization::CodeGenModule("MikanLightTypes")) MikanDMXSequenceComponentValues
	: public MikanComponentValues
{
	static const char* k_componentClassName;
	static const char* k_ownerSystemName;

	FIELD() MikanDMXFixtureGroupID group_id= INVALID_MIKAN_ID;

	/// The handler a project script registered through ScriptContext.registerSequence
	FIELD() Serialization::String sequence_name;

	/// Zero or less runs until stopped
	FIELD() float duration_seconds= 10.f;

	FIELD() bool loop= true;

	FIELD() int playback_state= 0;

	FIELD() float time_since_start= 0.f;

#ifdef MIKANAPI_REFLECTION_ENABLED
	MikanDMXSequenceComponentValues_GENERATED
#endif
};

/// A probe holding the scene's estimated low-frequency lighting, recovered from
/// a captured video frame.
///
/// The environment is order-2 spherical harmonics, which is enough to carry the
/// diffuse lighting of a Lambertian surface but cannot represent a sharp light
/// source. Clients should treat it as a soft environment (a SkyLight) and add
/// their own key light if they need crisp shadows.
///
/// This derives from MikanTransformComponentValues so the probe has a world
/// position: a single environment assumes spatially-invariant lighting, which
/// real interiors violate, so multiple probes are the expected escape hatch.
struct MIKAN_API STRUCT(Serialization::CodeGenModule("MikanLightTypes")) MikanLightEnvironmentComponentValues
	: public MikanTransformComponentValues
{
	static const char* k_componentClassName;
	static const char* k_ownerSystemName;

	/// 27 floats: 9 order-2 SH coefficients, each RGB, in Mikan world space.
	/// Laid out flat rather than as vectors because the serializer's list
	/// element types do not include a 3-vector. Index (coefficient * 3 +
	/// channel). These are RADIANCE, so evaluating them directly against the SH
	/// basis produces an environment map; the Lambertian convolution factors
	/// are already folded out.
	///
	/// May evaluate NEGATIVE in some directions: order-2 SH rings around sharp
	/// lights and no regularization removes that. Clamp before use.
	FIELD() Serialization::List<float> sh_coefficients;

	/// Manual exposure calibration. The underlying decomposition recovers
	/// shading only up to a global scale, so this is set once per shoot by eye.
	FIELD() float exposure_scale= 1.f;

	/// l=1 over l=0 band energy - how directional the estimate is. Below about
	/// 0.25 the scene is effectively uniform ambient and key_light_direction is
	/// meaningless. Clients should not present a low-directionality estimate as
	/// a confident one.
	FIELD() float directionality= 0.f;

	/// Suggested key light direction in world space. Only meaningful when
	/// directionality is high.
	FIELD() MikanVector3f key_light_direction;

#ifdef MIKANAPI_REFLECTION_ENABLED
	MikanLightEnvironmentComponentValues_GENERATED
#endif
};

/// The format of the DMX buffer stored in universe's channel buffer
enum class ENUM(Serialization::CodeGenModule("MikanLightTypes")) MikanDMXBufferFormat : int
{
	DMXUncompressed ENUMVALUE_STRING("DMXUncompressed")= 0,
	DMXRLEEncoded ENUMVALUE_STRING("DMXRLEEncoded")= 1,
};

/// DMX Channel Data for a single DMX Universe
struct MIKAN_API STRUCT(Serialization::CodeGenModule("MikanLightTypes")) MikanUniverseDMXData
{
	// The DMX Universe ID for this channel data
	FIELD() uint16_t dmx_universe_id= 1;

	// The format of buffer (i.e. compressed or raw)
	FIELD() MikanDMXBufferFormat buffer_format= MikanDMXBufferFormat::DMXUncompressed;

	// The dmx channel data buffer
	FIELD() Serialization::List<uint8_t> buffer_data;

#ifdef MIKANAPI_REFLECTION_ENABLED
	MikanUniverseDMXData_GENERATED
#endif
};

// Run-Length-Encode(RLE) a source DMX channel buffer into a MikanUniverseDMXData.
// Fills outUniverseData->buffer_data with [count(1..255), value] pairs and sets
// buffer_format to RLEEncoded. Returns the number of encoded bytes written.
MIKAN_API_FUNC(size_t) mikanRLEEncodeDMXUniverseBuffer(const size_t in_buffer_size, const uint8_t* in_buffer,
													   MikanUniverseDMXData* outUniverseData);

// Extract the raw channel values from a MikanUniverseDMXData. Honors buffer_format:
// an RLEEncoded buffer is expanded, a Raw buffer is copied directly. Writes at most
// out_buffer_max_size bytes and returns the number of bytes written to out_buffer.
MIKAN_API_FUNC(size_t) mikanRLEDecodeDMXUniverseBuffer(const MikanUniverseDMXData* universeData,
													   const size_t out_buffer_max_size, uint8_t* out_buffer);

struct MIKAN_API STRUCT(Serialization::CodeGenModule("MikanLightTypes")) MikanDMXData
{
	// The server timestamp for the universe data
	FIELD() double server_time_seconds;

	// DMX data for all requested universes
	FIELD() Serialization::List<MikanUniverseDMXData> universes;

#ifdef MIKANAPI_REFLECTION_ENABLED
	MikanDMXData_GENERATED
#endif
};

#ifdef MIKANAPI_REFLECTION_ENABLED
File_MikanLightTypes_GENERATED
#endif
