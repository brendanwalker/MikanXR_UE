#pragma once

#include <memory>
#include <future>
#include <functional>
#include <string>

/// The ID of a VR Device
using MikanVRDeviceID = int32_t;

/// The ID of a stencil
using MikanStencilID = int32_t;

/// The ID of a spatial anchor
using MikanSpatialAnchorID = int32_t;

using MikanResponsePtr = std::shared_ptr<struct MikanResponse>;
using MikanResponsePromise = std::promise<MikanResponsePtr>;
using MikanResponseFuture = std::future<MikanResponsePtr>;

using MikanEventPtr = std::shared_ptr<struct MikanEvent>;
