#pragma once

#include "CoreMinimal.h"
#include "Containers/Ticker.h"
#include "Subsystems/EngineSubsystem.h"
#include "MikanAPI.h"
#include "MikanDataStore.h"
#include "MikanDMXDataStore.h"

#include "MikanEngineSubsystem.generated.h"

struct MikanCameraNewFrameEvent;

// C++ multicast delegate for raw camera frame (Mikan types not usable in Blueprint)
DECLARE_MULTICAST_DELEGATE_OneParam(FMikanCameraNewFrameRawDelegate, const MikanCameraNewFrameEvent&);

// Blueprint-compatible delegates (shared with AMikanClient via this header)
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FMikanSimpleEvent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMikanScriptMessageEvent, const FString&, ScriptMessage);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FMikanAppStageChangedEvent, const FString&, FromStage, const FString&, ToStage);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FMikanRemoteControlEvent, const FString&, EventType, const TArray<FString>&, EventData);



UCLASS(HideCategories = (Navigation, Rendering, Physics, Collision, Cooking, Input, Actor))
class MIKANXR_API UMikanEngineSubsystem : public UEngineSubsystem
{
	GENERATED_BODY()

public:
	UMikanEngineSubsystem();

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	static UMikanEngineSubsystem* Get();
	bool Tick(float DeltaTime);

	inline class IMikanAPI* GetMikanAPI() { return MikanAPI; }
	inline const MikanClientInfo& GetClientInfo() const { return ClientInfo; }
	inline void SetWantsToBeConnected(bool bFlag) { bWantsToBeConnected= bFlag; }
	inline bool GetWantsToBeConnected() const { return bWantsToBeConnected; }
	inline bool GetIsConnected() const { return bIsConnected; }
	inline int32 GetPendingRequestCount() const { return PendingRequests.Num(); }
	inline class UMikanDataStore* GetDataStore() const { return DataStore; }
	inline class UMikanDMXDataStore* GetDMXDataStore() const { return DMXDataStore; }

	// -- Async request pump -----
	// Callback invoked on the game thread when a response arrives (or the request times out).
	using FMikanResponseCallback = TFunction<void(const MikanResponsePtr&)>;
	using FMikanRequestHandle = uint64;

	// Default deadline for an async request. The connection to the Mikan editor can go stale while
	// both ends still believe they are connected, and a response that never arrives would otherwise
	// leave its entry pending forever. Requests fail cleanly instead. Pass 0 to wait indefinitely.
	static constexpr float kDefaultRequestTimeoutSeconds = 15.f;

	// Fire a request without blocking. OnComplete runs on the game thread (drained in Tick) once the
	// response arrives, or with a synthesized Timeout response if TimeoutSeconds > 0 and it elapses
	// first. Because the callback runs on the game thread it may safely touch UObjects/actors — but
	// the response can outlive the caller, so capture a TWeakObjectPtr, not a raw this.
	// Returns a handle for CancelRequest; 0 means the request could not be started.
	FMikanRequestHandle SendRequestAsync(
		struct MikanRequest& Request,
		FMikanResponseCallback OnComplete,
		float TimeoutSeconds = kDefaultRequestTimeoutSeconds);

	// Drop a specific in-flight request. Its callback will not fire. Safe with a stale or 0 handle.
	void CancelRequest(FMikanRequestHandle Handle);

	// Drop all in-flight requests without firing their callbacks.
	void CancelAllRequests();

	// Remote Control API
	UFUNCTION(BlueprintCallable)
	bool SendPushAppStageRequest(const FString& StageName);
	UFUNCTION(BlueprintCallable)
	bool SendPopAppStageRequest();
	UFUNCTION(BlueprintCallable)
	bool GetCurrentAppStageName(FString& OutStateName);
	UFUNCTION(BlueprintCallable)
	void SendRemoteControlCommand(
		const FString& CommandType,
		const TArray<FString>& CommandArgs,
		bool& OutSuccess,
		TArray<FString>& OutResults);

	// App Connection Events
	UPROPERTY(BlueprintAssignable)
	FMikanSimpleEvent OnMikanConnected;
	UPROPERTY(BlueprintAssignable)
	FMikanSimpleEvent OnMikanDisconnected;

	// Light Events
	UPROPERTY(BlueprintAssignable)
	FMikanSimpleEvent OnDMXDataChanged;

	// Script Message Events
	UPROPERTY(BlueprintAssignable)
	FMikanScriptMessageEvent OnScriptMessage;

	// Remote Control Events
	UPROPERTY(BlueprintAssignable)
	FMikanAppStageChangedEvent OnAppStageChanged;
	UPROPERTY(BlueprintAssignable)
	FMikanRemoteControlEvent OnRemoteControlEvent;

	// Raw camera frame — C++ only (Mikan types not usable in Blueprint)
	FMikanCameraNewFrameRawDelegate OnCameraNewFrameRaw;

protected:
	void HandleMikanEvent(MikanEventPtr mikanEvent);
	void HandleMikanConnected();
	void HandleMikanDisconnected();
	void HandleCameraNewFrame(const MikanCameraNewFrameEvent& newFrameEvent);
	void HandleComponentPropertyChanged(const struct MikanPropertyUpdateEvent& PropertyUpdateEvent);
	void HandleLightDMXDataChanged(const struct MikanLightDMXDataChangedEvent& LightDMXDataEvent);
	void HandleScriptMessage(const struct MikanScriptMessagePostedEvent& ScriptMessageEvent);
	void HandleAppStateChangedMessage(const struct MikanAppStageChangedEvent& AppStageChangedEvent);
	void HandleRemoteControlEvent(const struct MikanRemoteControlEvent& RemoteControlEvent);
	void SendSubscribeToPropertyUpdates();
	void SendSubscribeToDMXUpdates();

	// Poll every in-flight async request and fire the callbacks of those that have completed
	// (or timed out). Called on the game thread from Tick.
	void PumpPendingRequests();

	struct FMikanPendingRequest
	{
		FMikanRequestHandle Handle = 0;
		MikanResponseFuture Future;
		FMikanResponseCallback OnComplete;
		double DeadlineSeconds = 0.0; // <= 0 means no timeout
	};

	class IMikanAPI* MikanAPI = nullptr;
	MikanClientInfo ClientInfo;

	TArray<FMikanPendingRequest> PendingRequests;
	FMikanRequestHandle NextRequestHandle = 0;

	FTickerDelegate TickDelegate;
	FTSTicker::FDelegateHandle TickDelegateHandle;
	float MikanReconnectTimeout = 0.f;
	bool bWantsToBeConnected = false;
	bool bIsConnected = false;


	UPROPERTY(Transient)
	class UMikanDataStore* DataStore = nullptr;

	UPROPERTY(Transient)
	class UMikanDMXDataStore* DMXDataStore = nullptr;
};
