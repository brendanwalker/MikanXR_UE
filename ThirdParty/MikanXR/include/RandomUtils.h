#pragma once

#include "MikanUtilityExport.h"

#include <string>

//-- utility methods -----
namespace RandomUtils
{
MIKAN_UTILITY_FUNC(std::string) RandomHexString(const unsigned int length);
}