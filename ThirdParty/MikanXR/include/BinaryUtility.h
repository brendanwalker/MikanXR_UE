#pragma once

#include "SerializationExport.h"

#include "stdint.h"

#include <array>
#include <string>
#include <vector>

//-- utility methods -----
namespace Serialization
{
	enum class Endian : int
	{
		Little,
		Big
	};

	SERIALIZATION_API Endian get_system_endianness();
	SERIALIZATION_API bool wants_byte_flip(Endian desired);

	SERIALIZATION_API int16_t read_int16(const uint8_t* inData, Endian desired);
	SERIALIZATION_API int32_t read_int24(const uint8_t* inData, Endian desired);
	SERIALIZATION_API int32_t read_int32(const uint8_t* inData, Endian desired);
	SERIALIZATION_API int64_t read_int64(const uint8_t* inData, Endian desired);
	SERIALIZATION_API float read_float(const uint8_t* inData, Endian desired);
	SERIALIZATION_API double read_double(const uint8_t* inData, Endian desired);

	SERIALIZATION_API void write_int16(uint8_t* outData, int16_t inValue, Endian desired);
	SERIALIZATION_API void write_int24(uint8_t* outData, int32_t inValue, Endian desired);
	SERIALIZATION_API void write_int32(uint8_t* outData, int32_t inValue, Endian desired);
	SERIALIZATION_API void write_int64(uint8_t* outData, int64_t inValue, Endian desired);
	SERIALIZATION_API void write_float(uint8_t* outData, float inValue, Endian desired);
	SERIALIZATION_API void write_double(uint8_t* outData, double inValue, Endian desired);
};

class SERIALIZATION_API BinaryWriter
{
public:
	BinaryWriter() = delete;
	BinaryWriter(std::vector<uint8_t>& inBuffer);

	void appendByte(uint8_t value);
	void appendBytes(const uint8_t* byteArray, size_t byteCount);

	template<int Count>
	void appendBytes(const std::array<uint8_t, Count>& byteArray)
	{
		m_buffer.insert(m_buffer.end(), byteArray.begin(), byteArray.end());
	}

private:
	std::vector<uint8_t>& m_buffer;
};

SERIALIZATION_API void to_binary(BinaryWriter& writer, bool inValue);
SERIALIZATION_API void to_binary(BinaryWriter& writer, uint8_t inValue);
SERIALIZATION_API void to_binary(BinaryWriter& writer, uint16_t inValue);
SERIALIZATION_API void to_binary(BinaryWriter& writer, uint32_t inValue);
SERIALIZATION_API void to_binary(BinaryWriter& writer, uint64_t inValue);
SERIALIZATION_API void to_binary(BinaryWriter& writer, int8_t inValue);
SERIALIZATION_API void to_binary(BinaryWriter& writer, int16_t inValue);
SERIALIZATION_API void to_binary(BinaryWriter& writer, int32_t inValue);
SERIALIZATION_API void to_binary(BinaryWriter& writer, int64_t inValue);
SERIALIZATION_API void to_binary(BinaryWriter& writer, float inValue);
SERIALIZATION_API void to_binary(BinaryWriter& writer, double inValue);
SERIALIZATION_API void to_binary(BinaryWriter& writer, const std::string& inString);

template<typename T, int Count>
inline void to_binary(BinaryWriter& writer, const std::array<T, Count>& inArray)
{
	to_binary(writer, Count);

	for (const T& value : inArray)
	{
		to_binary(writer, value);
	}
}

template<typename T>
inline void to_binary(BinaryWriter& writer, const std::vector<T>& inVector)
{
	to_binary(writer, (int32_t)inVector.size());

	for (const T& value : inVector)
	{
		to_binary(writer, value);
	}
}

class SERIALIZATION_API BinaryReader
{
public:
	BinaryReader() = delete;
	BinaryReader(const uint8_t* buffer, size_t bufferSize);

	uint8_t readByte();
	void readBytes(uint8_t* outBuffer, size_t byteCount);
	uint8_t* readBytesNoCopy(size_t byteCount);

	template<int Count>
	void readBytes(std::array<uint8_t, Count>& outBuffer)
	{
		readBytes(outBuffer.data(), Count);
	}

private:
	const uint8_t* m_buffer;
	size_t m_bufferSize;
	size_t m_bytesRead;
};

SERIALIZATION_API void from_binary(BinaryReader& reader, bool& outValue);
SERIALIZATION_API void from_binary(BinaryReader& reader, uint8_t& outValue);
SERIALIZATION_API void from_binary(BinaryReader& reader, uint16_t& outValue);
SERIALIZATION_API void from_binary(BinaryReader& reader, uint32_t& outValue);
SERIALIZATION_API void from_binary(BinaryReader& reader, uint64_t& outValue);
SERIALIZATION_API void from_binary(BinaryReader& reader, int8_t& outValue);
SERIALIZATION_API void from_binary(BinaryReader& reader, int16_t& outValue);
SERIALIZATION_API void from_binary(BinaryReader& reader, int32_t& outValue);
SERIALIZATION_API void from_binary(BinaryReader& reader, int64_t& outValue);
SERIALIZATION_API void from_binary(BinaryReader& reader, float& outValue);
SERIALIZATION_API void from_binary(BinaryReader& reader, double& outValue);
SERIALIZATION_API void from_binary(BinaryReader& reader, std::string& outString);

template<typename T, int Count>
inline void from_binary(BinaryReader& reader, std::array<T, Count>& outArray)
{
	from_binary(reader, Count);

	for (const T& value : outArray)
	{
		from_binary(reader, value);
	}
}

template<typename T>
inline void from_binary(BinaryReader& reader, std::vector<T>& outVector)
{
	int32_t count;
	from_binary(reader, count);

	outVector.resize(count);
	for (T& value : outVector)
	{
		from_binary(reader, value);
	}
}