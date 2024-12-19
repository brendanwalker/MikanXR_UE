#pragma once

#include "SerializationExport.h"

#ifdef SERIALIZATION_REFLECTION_ENABLED
#include "SerializableMap.rfkh.h"
#endif

#include <map>
#include <memory>

namespace Serialization NAMESPACE()
{
	class SERIALIZATION_API CLASS() IMapConstEnumerator
	{
	public:
		virtual ~IMapConstEnumerator() = default;

		virtual void reset() = 0;
		virtual bool isValid() const = 0;
		virtual void next() = 0;
		virtual const void* getKeyRaw() const = 0;
		virtual const void* getValueRaw() const = 0;

		#ifdef SERIALIZATION_REFLECTION_ENABLED
		Serialization_IMapConstEnumerator_GENERATED
		#endif
	};

	class SERIALIZATION_API CLASS() IMapEnumerator
	{
	public:
		virtual ~IMapEnumerator() = default;

		virtual void reset() = 0;
		virtual bool isValid() const = 0;
		virtual void next() = 0;
		virtual void* getKeyRaw() = 0;
		virtual void* getValueRaw() = 0;

		#ifdef SERIALIZATION_REFLECTION_ENABLED
		Serialization_IMapEnumerator_GENERATED
		#endif
	};

	template <typename t_key, typename t_value>
	class CLASS() MapConstEnumerator : public IMapConstEnumerator
	{
	public:
		MapConstEnumerator(const std::map<t_key, t_value>& map)
			: m_start(map.begin())
			, m_end(map.end())
			, m_it(m_start)
		{
		}

		virtual void reset() override
		{
			m_it = m_start;
		}

		virtual bool isValid() const override
		{
			return m_it != m_end;
		}

		virtual void next() override
		{
			if (isValid())
			{
				m_it++;
			}
		}

		virtual const void* getKeyRaw() const override
		{
			if (m_it != m_end)
			{
				return &m_it->first;
			}

			return nullptr;
		}

		virtual const void* getValueRaw() const override
		{
			if (m_it != m_end)
			{
				return &m_it->second;
			}

			return nullptr;
		}
	
		#ifdef SERIALIZATION_REFLECTION_ENABLED
		Serialization_MapConstEnumerator_GENERATED
		#endif

	private:
		typename std::map<t_key, t_value>::const_iterator m_start;
		typename std::map<t_key, t_value>::const_iterator m_end;
		typename std::map<t_key, t_value>::const_iterator m_it;
	};

	template <typename t_key, typename t_value>
	class CLASS() MapEnumerator : public IMapEnumerator
	{
	public:
		MapEnumerator(std::map<t_key, t_value>& map)
			: m_start(map.begin())
			, m_end(map.end())
			, m_it(m_start)
		{}

		virtual void reset() override
		{
			m_it = m_start;
		}

		virtual bool isValid() const override
		{
			return m_it != m_end;
		}

		virtual void next() override
		{
			if (isValid())
			{
				m_it++;
			}
		}

		virtual void* getKeyRaw() override
		{
			if (m_it != m_end)
			{
				return (void *)(&m_it->first);
			}

			return nullptr;
		}

		virtual void* getValueRaw() override
		{
			if (m_it != m_end)
			{
				return (void *)(&m_it->second);
			}

			return nullptr;
		}

		#ifdef SERIALIZATION_REFLECTION_ENABLED
		Serialization_MapEnumerator_GENERATED
		#endif

	private:
		typename std::map<t_key, t_value>::iterator m_start;
		typename std::map<t_key, t_value>::iterator m_end;
		typename std::map<t_key, t_value>::iterator m_it;
	};

	template <typename t_key, typename t_value>
	class CLASS() Map : public std::map<t_key, t_value>
	{
	public:
		METHOD()
		void clear() noexcept
		{
			return std::map<t_key, t_value>::clear();
		}

		METHOD()
		std::size_t size() const noexcept
		{
			return std::map<t_key, t_value>::size();
		}

		METHOD()
		std::shared_ptr<IMapEnumerator> getEnumerator()
		{
			return std::make_shared<MapEnumerator<t_key, t_value>>(*this);
		}

		METHOD()
		std::shared_ptr<IMapConstEnumerator> getConstEnumerator() const
		{
			return std::make_shared<MapConstEnumerator<t_key, t_value>>(*this);
		}

		METHOD()
		const void* getOrAddRawValueMutable(const t_key& key)
		{
			auto it = std::map<t_key, t_value>::find(key);
			if (it != std::map<t_key, t_value>::end())
			{
				return &it->second;
			}
			else
			{
				auto result= std::map<t_key, t_value>::insert({key, t_value()});
				auto iter= result.first;

				return &iter->second;
			}
		}

		METHOD()
		const void* getRawValue(const t_key& key) const
		{
			auto it = std::map<t_key, t_value>::find(key);
			if (it != std::map<t_key, t_value>::end())
			{
				return &it->second;
			}
			else
			{
				return nullptr;
			}
		}

		METHOD()
		void* getRawValueMutable(const t_key& key)
		{
			return const_cast<void*>(getRawValue(key));
		}

		#ifdef SERIALIZATION_REFLECTION_ENABLED
		Serialization_Map_GENERATED
		#endif
	};
};

#ifdef SERIALIZATION_REFLECTION_ENABLED
File_SerializableMap_GENERATED
#endif