#pragma once

#include "MikanAPITypes.h"

#define MIKAN_TIMEOUT_DEFAULT		1000

class MIKAN_API MikanResponseFuture
{
public:
	MikanResponseFuture();
	MikanResponseFuture(MikanAPIResult result);
	MikanResponseFuture(
		class MikanRequestManager* owner, 
		MikanRequestID requestId, 
		MikanResponsePromise& promise);
	MikanResponseFuture(MikanResponseFuture&& _Other) noexcept;
	virtual ~MikanResponseFuture();

	MikanResponseFuture(const MikanResponseFuture&) = delete;
	MikanResponseFuture& operator=(const MikanResponseFuture&) = delete;

	bool isValid() const { return m_impl != nullptr; }
	MikanResponsePtr get(uint32_t timeoutMilliseconds = MIKAN_TIMEOUT_DEFAULT);

protected:
	static MikanResponsePtr makeSimpleMikanResponse(MikanAPIResult result);

private:
	struct MikanResponseFutureImpl* m_impl= nullptr;
};
