#pragma once

#include "MikanAPIExport.h"
#include "MikanAPITypes.h"
#include "MikanMathTypes.h"
#include "SerializableList.h"
#include "SerializableMap.h"
#include "SerializableString.h"
#include "SerializableObjectPtr.h"
#include "SerializationProperty.h"

#ifdef MIKANAPI_REFLECTION_ENABLED
#include "MikanVariantTypes.rfkh.h"
#endif

#if defined(MIKANAPI_REFLECTION_ENABLED) && defined(SERIALIZATION_REFLECTION_ENABLED)
#define MIKAN_SETTER_API_ENABLED 1
#else
#define MIKAN_SETTER_API_ENABLED 0
#endif

#if MIKAN_SETTER_API_ENABLED
// Headers needed for the private setter API
#include <filesystem>
#include <string>
#include <type_traits>
#include <vector>
#endif

enum class ENUM(Serialization::CodeGenModule("MikanVariantTypes")) MikanVariantType : int
{
	INVALID ENUMVALUE_STRING("INVALID_TYPE"),

	// Primitive Types
	BOOL ENUMVALUE_STRING("BOOL_TYPE"),
	UBYTE ENUMVALUE_STRING("UBYTE_TYPE"),
	USHORT ENUMVALUE_STRING("USHORT_TYPE"),
	INT ENUMVALUE_STRING("INT_TYPE"),
	LONG ENUMVALUE_STRING("LONG_TYPE"),
	FLOAT ENUMVALUE_STRING("FLOAT_TYPE"),
	DOUBLE ENUMVALUE_STRING("DOUBLE_TYPE"),
	STRING ENUMVALUE_STRING("MK_STRING_TYPE"),
	VECTOR2F ENUMVALUE_STRING("VECTOR2F_TYPE"),
	VECTOR3F ENUMVALUE_STRING("VECTOR3F_TYPE"),
	VECTOR4F ENUMVALUE_STRING("VECTOR4F_TYPE"),
	QUATERNIONF ENUMVALUE_STRING("QUATERNIONF_TYPE"),
	MATRIX4F ENUMVALUE_STRING("MATRIX4F_TYPE"),
	VECTOR2D ENUMVALUE_STRING("VECTOR2D_TYPE"),
	VECTOR3D ENUMVALUE_STRING("VECTOR3D_TYPE"),
	VECTOR4D ENUMVALUE_STRING("VECTOR4D_TYPE"),
	QUATERNIOND ENUMVALUE_STRING("QUATERNIOND_TYPE"),

	// Array Types
	BOOL_ARRAY ENUMVALUE_STRING("BOOL_ARRAY_TYPE"),
	UBYTE_ARRAY ENUMVALUE_STRING("UBYTE_ARRAY_TYPE"),
	INT_ARRAY ENUMVALUE_STRING("INT_ARRAY_TYPE"),
	FLOAT_ARRAY ENUMVALUE_STRING("FLOAT_ARRAY_TYPE"),
	STRING_ARRAY ENUMVALUE_STRING("STRING_ARRAY_TYPE"),

	// Map Types
	STRING_MAP ENUMVALUE_STRING("STRING_MAP_TYPE"),

	// Object Types
	POLYMORPHIC_OBJECT ENUMVALUE_STRING("POLYMORPHIC_OBJECT_TYPE"),
};

MIKAN_API_FUNC(const char*) mikanVariantTypeToString(MikanVariantType variantType);

/// Bundle containing all intrinsic video source properties
struct MIKAN_API STRUCT(Serialization::CodeGenModule("MikanVariantTypes")) MikanVariant
{
	FIELD() MikanVariantType value_type;

	// MikanVariantBase derived type
	FIELD() Serialization::PolymorphicObjectPtr value_ptr;

	MikanVariant();

	// Construct by variant type
	template <typename T>
	explicit MikanVariant(T&& t)
		: value_type(MikanVariantType::INVALID)
		, value_ptr()
	{
		setValue(std::forward<T>(t));
	}

	// Assign by variant type
	template <typename T>
	MikanVariant& operator=(T&& t)
	{
		setValue(std::forward<T>(t));
		return *this;
	}

	void clear();

	bool getBoolValue() const;
	uint8_t getUByteValue() const;
	uint16_t getUShortValue() const;
	int getIntValue() const;
	long getLongValue() const;
	float getFloatValue() const;
	double getDoubleValue() const;
	const Serialization::String& getSerializationStringValue() const;
	const char* getUtf8Value() const;
	const MikanVector2f& getVector2fValue() const;
	const MikanVector3f& getVector3fValue() const;
	const MikanVector4f& getVector4fValue() const;
	const MikanQuatf& getQuaternionfValue() const;
	const MikanMatrix4f& getMatrix4fValue() const;
	const MikanVector2d& getVector2dValue() const;
	const MikanVector3d& getVector3dValue() const;
	const MikanVector4d& getVector4dValue() const;
	const MikanQuatd& getQuaterniondValue() const;
	const float getVectorComponentValue(size_t index) const;
	const Serialization::List<bool>& getBoolArrayValue() const;
	const Serialization::List<uint8_t>& getUByteArrayValue() const;
	const Serialization::List<int>& getIntArrayValue() const;
	const Serialization::List<float>& getFloatArrayValue() const;
	const Serialization::List<Serialization::String>& getStringArrayValue() const;
	const Serialization::Map<Serialization::String, Serialization::String>& getStringMapValue() const;
	const Serialization::PolymorphicObjectPtr& getPolymorphicObjectValue() const;

	// These setters take std::string/std::vector by design. They are gated behind the reflection
	// macros, which are PRIVATE to MikanClientAPI (see its CMakeLists.txt) and are never defined in
	// non-reflection consumers such as the Unreal plugin. Those consumers compile this whole block
	// out, so these std::string/std::vector signatures are only ever used within reflection-enabled,
	// same-CRT targets (the DLL itself, the editor, tests) and never cross a Debug/Release CRT
	// boundary. Do NOT move a setter out of this block without making its signature CRT-safe first.
#if MIKAN_SETTER_API_ENABLED
	void setValue(const MikanVariant& other);
	void setValue(bool value);
	void setValue(uint8_t value);
	void setValue(uint16_t value);
	void setValue(int value);
	void setValue(long value);
	void setValue(float value);
	void setValue(double value);
	void setValue(const char* value);
	void setValue(const std::string& value);
	void setValue(const std::filesystem::path& value);
	void setValue(const MikanVector2f& value);
	void setValue(const MikanVector3f& value);
	void setValue(const MikanVector4f& value);
	void setValue(const MikanQuatf& value);
	void setValue(const MikanMatrix4f& value);
	void setValue(const MikanVector2d& value);
	void setValue(const MikanVector3d& value);
	void setValue(const MikanVector4d& value);
	void setValue(const MikanQuatd& value);
	void setVectorComponentValue(size_t index, float value);
	void setValue(const std::vector<bool>& value);
	void setValue(const std::vector<int>& value);
	void setValue(const std::vector<float>& value);
	void setValue(const std::vector<std::string>& value);
	void setValue(const std::vector<Serialization::String>& value);
	void setValue(const std::vector<uint8_t>& value);
	void setValue(const Serialization::List<uint8_t>& value);
	void setValue(const Serialization::List<bool>& value);
	void setValue(const Serialization::List<int>& value);
	void setValue(const Serialization::List<float>& value);
	void setValue(const Serialization::List<Serialization::String>& value);
	void setValue(const Serialization::Map<Serialization::String, Serialization::String>& value);
	void setValue(const Serialization::PolymorphicObjectPtr& value);
#endif // MIKAN_SETTER_API_ENABLED

#ifdef MIKANAPI_REFLECTION_ENABLED
	MikanVariant_GENERATED
#endif // MIKANAPI_REFLECTION_ENABLED
};

struct MIKAN_API STRUCT(Serialization::CodeGenModule("MikanVariantTypes")) MikanVariantBase
	: public Serialization::PolymorphicStruct
{
#ifdef MIKANAPI_REFLECTION_ENABLED
	MikanVariantBase_GENERATED
#endif
};

struct MIKAN_API STRUCT(Serialization::CodeGenModule("MikanVariantTypes")) MikanBoolValue : public MikanVariantBase
{
	FIELD() bool value= false;

#ifdef MIKANAPI_REFLECTION_ENABLED
	MikanBoolValue_GENERATED
#endif
};

struct MIKAN_API STRUCT(Serialization::CodeGenModule("MikanVariantTypes")) MikanUByteValue : public MikanVariantBase
{
	FIELD() uint8_t value= false;

#ifdef MIKANAPI_REFLECTION_ENABLED
	MikanUByteValue_GENERATED
#endif
};

struct MIKAN_API STRUCT(Serialization::CodeGenModule("MikanVariantTypes")) MikanUShortValue : public MikanVariantBase
{
	FIELD() uint16_t value= false;

#ifdef MIKANAPI_REFLECTION_ENABLED
	MikanUShortValue_GENERATED
#endif
};

struct MIKAN_API STRUCT(Serialization::CodeGenModule("MikanVariantTypes")) MikanIntValue : public MikanVariantBase
{
	FIELD() int value= 0;

#ifdef MIKANAPI_REFLECTION_ENABLED
	MikanIntValue_GENERATED
#endif
};

struct MIKAN_API STRUCT(Serialization::CodeGenModule("MikanVariantTypes")) MikanLongValue : public MikanVariantBase
{
	FIELD() long value= 0;

#ifdef MIKANAPI_REFLECTION_ENABLED
	MikanLongValue_GENERATED
#endif
};

struct MIKAN_API STRUCT(Serialization::CodeGenModule("MikanVariantTypes")) MikanFloatValue : public MikanVariantBase
{
	FIELD() float value= 0.0f;

#ifdef MIKANAPI_REFLECTION_ENABLED
	MikanFloatValue_GENERATED
#endif
};

struct MIKAN_API STRUCT(Serialization::CodeGenModule("MikanVariantTypes")) MikanDoubleValue : public MikanVariantBase
{
	FIELD() double value= 0.0;

#ifdef MIKANAPI_REFLECTION_ENABLED
	MikanDoubleValue_GENERATED
#endif
};

struct MIKAN_API STRUCT(Serialization::CodeGenModule("MikanVariantTypes")) MikanStringValue : public MikanVariantBase
{
	FIELD() Serialization::String value;

#ifdef MIKANAPI_REFLECTION_ENABLED
	MikanStringValue_GENERATED
#endif
};

struct MIKAN_API STRUCT(Serialization::CodeGenModule("MikanVariantTypes")) MikanVector2fValue : public MikanVariantBase
{
	FIELD() MikanVector2f value;

#ifdef MIKANAPI_REFLECTION_ENABLED
	MikanVector2fValue_GENERATED
#endif
};

struct MIKAN_API STRUCT(Serialization::CodeGenModule("MikanVariantTypes")) MikanVector3fValue : public MikanVariantBase
{
	FIELD() MikanVector3f value;

#ifdef MIKANAPI_REFLECTION_ENABLED
	MikanVector3fValue_GENERATED
#endif
};

struct MIKAN_API STRUCT(Serialization::CodeGenModule("MikanVariantTypes")) MikanVector4fValue : public MikanVariantBase
{
	FIELD() MikanVector4f value;

#ifdef MIKANAPI_REFLECTION_ENABLED
	MikanVector4fValue_GENERATED
#endif
};

struct MIKAN_API STRUCT(Serialization::CodeGenModule("MikanVariantTypes")) MikanQuatfValue : public MikanVariantBase
{
	FIELD() MikanQuatf value;

#ifdef MIKANAPI_REFLECTION_ENABLED
	MikanQuatfValue_GENERATED
#endif
};

struct MIKAN_API STRUCT(Serialization::CodeGenModule("MikanVariantTypes")) MikanMatrix4fValue : public MikanVariantBase
{
	FIELD() MikanMatrix4f value;

#ifdef MIKANAPI_REFLECTION_ENABLED
	MikanMatrix4fValue_GENERATED
#endif
};

struct MIKAN_API STRUCT(Serialization::CodeGenModule("MikanVariantTypes")) MikanVector2dValue : public MikanVariantBase
{
	FIELD() MikanVector2d value;

#ifdef MIKANAPI_REFLECTION_ENABLED
	MikanVector2dValue_GENERATED
#endif
};

struct MIKAN_API STRUCT(Serialization::CodeGenModule("MikanVariantTypes")) MikanVector3dValue : public MikanVariantBase
{
	FIELD() MikanVector3d value;

#ifdef MIKANAPI_REFLECTION_ENABLED
	MikanVector3dValue_GENERATED
#endif
};

struct MIKAN_API STRUCT(Serialization::CodeGenModule("MikanVariantTypes")) MikanVector4dValue : public MikanVariantBase
{
	FIELD() MikanVector4d value;

#ifdef MIKANAPI_REFLECTION_ENABLED
	MikanVector4dValue_GENERATED
#endif
};

struct MIKAN_API STRUCT(Serialization::CodeGenModule("MikanVariantTypes")) MikanQuatdValue : public MikanVariantBase
{
	FIELD() MikanQuatd value;

#ifdef MIKANAPI_REFLECTION_ENABLED
	MikanQuatdValue_GENERATED
#endif
};

struct MIKAN_API STRUCT(Serialization::CodeGenModule("MikanVariantTypes")) MikanBoolArrayValue : public MikanVariantBase
{
	FIELD() Serialization::List<bool> value;

#ifdef MIKANAPI_REFLECTION_ENABLED
	MikanBoolArrayValue_GENERATED
#endif
};

struct MIKAN_API STRUCT(Serialization::CodeGenModule("MikanVariantTypes")) MikanUByteArrayValue
	: public MikanVariantBase
{
	FIELD() Serialization::List<uint8_t> value;

#ifdef MIKANAPI_REFLECTION_ENABLED
	MikanUByteArrayValue_GENERATED
#endif
};

struct MIKAN_API STRUCT(Serialization::CodeGenModule("MikanVariantTypes")) MikanIntArrayValue : public MikanVariantBase
{
	FIELD() Serialization::List<int> value;

#ifdef MIKANAPI_REFLECTION_ENABLED
	MikanIntArrayValue_GENERATED
#endif
};

struct MIKAN_API STRUCT(Serialization::CodeGenModule("MikanVariantTypes")) MikanFloatArrayValue
	: public MikanVariantBase
{
	FIELD() Serialization::List<float> value;

#ifdef MIKANAPI_REFLECTION_ENABLED
	MikanFloatArrayValue_GENERATED
#endif
};

struct MIKAN_API STRUCT(Serialization::CodeGenModule("MikanVariantTypes")) MikanStringArrayValue
	: public MikanVariantBase
{
	FIELD() Serialization::List<Serialization::String> value;

#ifdef MIKANAPI_REFLECTION_ENABLED
	MikanStringArrayValue_GENERATED
#endif
};

struct MIKAN_API STRUCT(Serialization::CodeGenModule("MikanVariantTypes")) MikanStringMapValue : public MikanVariantBase
{
	FIELD() Serialization::Map<Serialization::String, Serialization::String> value;

#ifdef MIKANAPI_REFLECTION_ENABLED
	MikanStringMapValue_GENERATED
#endif
};

#ifdef MIKANAPI_REFLECTION_ENABLED
File_MikanVariantTypes_GENERATED
#endif // MIKANAPI_REFLECTION_ENABLED