#pragma once

#include "nlohmann/json.hpp"

using json= nlohmann::json;

// Base class for JSON SAX Parser for searching for a key-value pair in a JSON string
// Don't use this class directly, use one of the derived classes below instead
template <typename t_value_type>
class JsonSaxValueSearcherBase : public json::json_sax_t
{
public:
	JsonSaxValueSearcherBase()= default;

	bool hasKey(const std::string& jsonString, const std::string& key)
	{
		m_searchKey= key;
		m_resultValue= t_value_type();
		json::sax_parse(jsonString, this);

		return m_keyValueFound;
	}

	bool fetchKeyValuePair(const std::string& jsonString, const std::string& key, t_value_type& outValue)
	{
		m_searchKey= key;
		m_resultValue= t_value_type();
		json::sax_parse(jsonString, this);

		outValue= m_resultValue;
		return m_keyValueFound;
	}

	// SAX Parse Interface
	bool null() override
	{
		m_keyValueFound= false;
		return true;
	}

	bool boolean(bool val) override
	{
		m_keyValueFound= false;
		return true;
	}

	bool number_integer(number_integer_t val) override
	{
		m_keyValueFound= false;
		return true;
	}

	bool number_unsigned(number_unsigned_t val) override
	{
		m_keyValueFound= false;
		return true;
	}

	bool number_float(number_float_t val, const string_t& s) override
	{
		m_keyValueFound= false;
		return true;
	}

	bool string(string_t& val) override
	{
		m_keyValueFound= false;
		return true; // keep parsing
	}

	bool start_object(std::size_t elements) override
	{
		m_keyValueFound= false;
		return true;
	}

	bool end_object() override
	{
		m_keyValueFound= false;
		return true;
	}

	bool start_array(std::size_t elements) override
	{
		m_keyValueFound= false;
		return true;
	}

	bool end_array() override
	{
		m_keyValueFound= false;
		return true;
	}

	bool key(string_t& val) override
	{
		if (val == m_searchKey)
		{
			m_keyValueFound= true;
		}
		return true;
	}

	bool binary(json::binary_t& val) override
	{
		m_keyValueFound= false;
		return true;
	}

	bool parse_error(std::size_t position, const std::string& last_token, const json::exception& ex) override
	{
		m_keyValueFound= false;
		return false;
	}

protected:
	std::string m_searchKey;
	t_value_type m_resultValue;
	bool m_keyValueFound= false;
};

// Searcher for integer values
class JsonSaxIntegerValueSearcher : public JsonSaxValueSearcherBase<int>
{
public:
	JsonSaxIntegerValueSearcher()= default;

	bool number_integer(number_integer_t val) override
	{
		if (m_keyValueFound)
		{
			m_resultValue= (int)val;
			return false; // stop parsing
		}

		return true; // keep parsing
	}

	bool number_unsigned(number_unsigned_t val) override
	{
		if (m_keyValueFound)
		{
			m_resultValue= (int)val;
			return false; // stop parsing
		}

		return true; // keep parsing
	}
};

// Searcher for classId values
class JsonSaxInt64ValueSearcher : public JsonSaxValueSearcherBase<int64_t>
{
public:
	JsonSaxInt64ValueSearcher()= default;

	bool number_integer(number_integer_t val) override
	{
		if (m_keyValueFound)
		{
			m_resultValue= (int64_t)val;
			return false; // stop parsing
		}

		return true; // keep parsing
	}

	bool number_unsigned(number_unsigned_t val) override
	{
		if (m_keyValueFound)
		{
			m_resultValue= (int64_t)val;
			return false; // stop parsing
		}

		return true; // keep parsing
	}
};

// Searcher for string values
class JsonSaxStringValueSearcher : public JsonSaxValueSearcherBase<std::string>
{
public:
	JsonSaxStringValueSearcher()= default;

	bool string(string_t& val) override
	{
		if (m_keyValueFound)
		{
			m_resultValue= val;
			return false; // stop parsing
		}

		return true; // keep parsing
	}
};