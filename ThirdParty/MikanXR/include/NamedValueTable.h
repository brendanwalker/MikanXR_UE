#pragma once

#include <string>
#include <map>
#include <vector>

template <typename t_value_type>
class NamedValueTable
{
public:
	using NamedValueMap= std::map<std::string, t_value_type>;

	int getNumEntries() const { return (int)dataValueMap.size(); }
	bool hasValue(const std::string& key) const { return dataValueMap.find(key) != dataValueMap.end(); }
	void setValue(const std::string& key, t_value_type value) { dataValueMap.insert_or_assign(key, value); }
	bool tryGetValue(const std::string& key, t_value_type& outValue) const
	{
		const auto& it= dataValueMap.find(key);
		if (it != dataValueMap.end())
		{
			outValue= it->second;
			return true;
		}
		else
		{
			return false;
		}
	}
	t_value_type getValueOrDefault(const std::string& key, t_value_type defaultValue) const
	{
		const auto& it= dataValueMap.find(key);
		return (it != dataValueMap.end()) ? it->second : defaultValue;
	}
	void removeValue(const std::string& key)
	{
		const auto& it= dataValueMap.find(key);
		if (it != dataValueMap.end())
			dataValueMap.erase(it);
	}
	const NamedValueMap& getMap() const { return dataValueMap; }
	std::vector<std::string> getNames() const
	{
		std::vector<std::string> names;

		for (auto it= dataValueMap.begin(); it != dataValueMap.end(); it++)
		{
			names.push_back(it->first);
		}

		return names;
	}
	void clear() { dataValueMap.clear(); }

protected:
	std::map<std::string, t_value_type> dataValueMap;
};