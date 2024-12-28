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
	bool isCompleted() const;

	// Non-Blocking Response Fetch
	// Return true if the response is ready, false otherwise
	bool tryFetchResponse(MikanResponsePtr& outResponse);

	// Non-Blocking Response Fetch
	// Return true if the response is ready, false otherwise
	MikanResponsePtr fetchResponse(uint32_t timeoutMilliseconds = MIKAN_TIMEOUT_DEFAULT);

	// Blocking Response Await
	// Returns once the response has been received or the timeout is reached
	void awaitResponse(uint32_t timeoutMilliseconds = MIKAN_TIMEOUT_DEFAULT);

protected:
	static MikanResponsePtr makeSimpleMikanResponse(MikanAPIResult result);

private:
	struct MikanResponseFutureImpl* m_impl= nullptr;
};
