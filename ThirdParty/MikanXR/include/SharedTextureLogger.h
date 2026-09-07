#pragma once

#include <functional>
#include <string>

enum class SharedTextureLogLevel : int
{
	info,
	error,
};
using SharedTextureLogCallback= std::function<void(SharedTextureLogLevel level, const std::string&)>;

class SharedTextureLogger
{
public:
	void setLogCallback(SharedTextureLogCallback callback) { m_logCallback= callback; }

	void log(SharedTextureLogLevel level, const std::string& message)
	{
		if (m_logCallback)
		{
			m_logCallback(level, message);
		}
	}

private:
	SharedTextureLogCallback m_logCallback;
};