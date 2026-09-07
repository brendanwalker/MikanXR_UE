#include "MikanEngineSubsystem.h"
#include "MikanAPI.h"
#include "MikanCameraEvents.h"
#include "MikanClientRequests.h"
#include "MikanClientEvents.h"
#include "MikanDataStore.h"
#include "MikanDMXDataStore.h"
#include "MikanLightEvents.h"
#include "MikanLightTypes.h"
#include "MikanLightRequests.h"
#include "MikanPropertyEvents.h"
#include "MikanPropertyRequests.h"
#include "MikanRemoteControlRequests.h"
#include "MikanRemoteControlEvents.h"
#include "MikanScriptEvents.h"
#include "MikanVideoSourceEvents.h"
#include "IMikanXRModule.h"
#include "Engine/Engine.h"
#include "RenderingThread.h"

// Upper bound on a synchronous remote control command round trip to the Mikan editor.
static constexpr uint32_t kRemoteControlCommandTimeoutMs = 5000;

UMikanEngineSubsystem::UMikanEngineSubsystem()
	: UEngineSubsystem()
{
}

UMikanEngineSubsystem* UMikanEngineSubsystem::Get()
{
	return GEngine ? GEngine->GetEngineSubsystem<UMikanEngineSubsystem>() : nullptr;
}

void UMikanEngineSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	if (IMikanXRModule::IsAvailable())
	{
		IMikanXRModule& MikanXRModule = IMikanXRModule::Get();

		MikanAPI = MikanXRModule.GetMikanAPI();
		MikanXRModule.GetClientInfo(&ClientInfo);

		TickDelegate = FTickerDelegate::CreateUObject(this, &UMikanEngineSubsystem::Tick);
		TickDelegateHandle = FTSTicker::GetCoreTicker().AddTicker(TickDelegate);

		DataStore = NewObject<UMikanDataStore>(this);
		DataStore->Initialize(MikanAPI);

		DMXDataStore = NewObject<UMikanDMXDataStore>(this);
		DMXDataStore->Initialize();
	}
}

void UMikanEngineSubsystem::Deinitialize()
{
	FlushRenderingCommands();

	if (IMikanXRModule::IsAvailable())
	{
		if (MikanAPI)
		{
			if (bIsConnected)
			{
				DisposeClientRequest disposeRequest = {};
				MikanAPI->sendRequest(disposeRequest).awaitResponse();
			}

			// Always disconnect to stop any in-progress auto-reconnect
			MikanAPI->disconnect();
		}

		FTSTicker::GetCoreTicker().RemoveTicker(TickDelegateHandle);
	}

	Super::Deinitialize();
}

bool UMikanEngineSubsystem::Tick(float DeltaTime)
{
	if (MikanAPI)
	{
		// Always drain the event queue — disconnect/connect events can arrive regardless of
		// the underlying WebSocket state (e.g. IXWebSocket fires Close then immediately
		// enters auto-reconnect Connecting state, keeping getIsConnected() true).
		MikanEventPtr mikanEvent;
		while (MikanAPI->fetchNextEvent(mikanEvent) == MikanAPIResult::Success)
		{
			// Track connection state from events, not from getIsConnected()
			if (typeid(*mikanEvent) == typeid(MikanConnectedEvent))
			{
				bIsConnected = true;

				// Send client info back to the server on connection
				InitClientRequest initClientRequest = {};
				initClientRequest.clientInfo = ClientInfo;
				MikanAPI->sendRequest(initClientRequest).awaitResponse();
			}
			else if (typeid(*mikanEvent) == typeid(MikanDisconnectedEvent))
			{
				bIsConnected = false;

				// The server connection is gone, so any in-flight async request will never get a
				// response. Drop them now rather than waiting out every deadline.
				CancelAllRequests();

				auto disconnectEvent = std::static_pointer_cast<MikanDisconnectedEvent>(mikanEvent);
				const char* reason = disconnectEvent->reason.getUtf8Value();

				if (disconnectEvent->code == MikanDisconnectCode_IncompatibleVersion)
				{
					bWantsToBeConnected = false;
					UE_LOG(MikanXRLog, Error, TEXT("MikanDisconnectedEvent: Disable reconnect due to incompatible client"));
				}
				else
				{
					UE_LOG(MikanXRLog, Log, TEXT("MikanDisconnectedEvent: %s"), ANSI_TO_TCHAR(reason));
				}
			}

			HandleMikanEvent(mikanEvent);
		}

		if (bIsConnected)
		{
			if (!bWantsToBeConnected)
			{
				MikanAPI->disconnect();
			}
		}
		else
		{
			if (bWantsToBeConnected && MikanReconnectTimeout <= 0.f)
			{
				MikanAPI->connect();

				// Back off after every attempt, not just a failed one: connect() is asynchronous and
				// returns before the socket is up, so retrying each frame stacks attempts on top of
				// one that is still in progress.
				MikanReconnectTimeout = 1.0f;
			}
			else
			{
				MikanReconnectTimeout -= DeltaTime;
			}
		}

		// Fire callbacks for any async requests whose responses have arrived. Runs regardless of
		// connection state so already-received responses (and timeouts) still get flushed.
		PumpPendingRequests();
	}

	return true;
}

UMikanEngineSubsystem::FMikanRequestHandle UMikanEngineSubsystem::SendRequestAsync(
	MikanRequest& Request,
	FMikanResponseCallback OnComplete,
	float TimeoutSeconds)
{
	if (!MikanAPI)
	{
		return 0;
	}

	const FMikanRequestHandle Handle = ++NextRequestHandle;
	const double DeadlineSeconds =
		TimeoutSeconds > 0.f ? FPlatformTime::Seconds() + (double)TimeoutSeconds : 0.0;

	// Aggregate-initialize so the move-only MikanResponseFuture is move-constructed in place.
	// (It has a move ctor but no move-assignment operator, so it can't be assigned after the fact.)
	PendingRequests.Add(FMikanPendingRequest{
		Handle,
		MikanAPI->sendRequest(Request), // non-blocking; returns immediately
		MoveTemp(OnComplete),
		DeadlineSeconds});

	return Handle;
}

void UMikanEngineSubsystem::CancelRequest(FMikanRequestHandle Handle)
{
	if (Handle == 0)
	{
		return;
	}

	for (int32 Index = 0; Index < PendingRequests.Num(); ++Index)
	{
		if (PendingRequests[Index].Handle == Handle)
		{
			PendingRequests.RemoveAtSwap(Index);
			return;
		}
	}
}

void UMikanEngineSubsystem::CancelAllRequests()
{
	PendingRequests.Reset();
}

void UMikanEngineSubsystem::PumpPendingRequests()
{
	if (PendingRequests.Num() == 0)
	{
		return;
	}

	const double Now = FPlatformTime::Seconds();

	// Iterate backwards so RemoveAtSwap only ever moves an already-visited entry into our slot.
	// Requests added by a callback during this pump land at the end and are picked up next tick.
	for (int32 Index = PendingRequests.Num() - 1; Index >= 0; --Index)
	{
		// A callback may cancel other requests (e.g. an actor re-issuing a fetch), shrinking the
		// array below Index. Skip any index that is no longer valid.
		if (Index >= PendingRequests.Num())
		{
			continue;
		}

		MikanResponsePtr Response;
		const bool bReady = PendingRequests[Index].Future.tryFetchResponse(Response);
		const bool bTimedOut =
			!bReady &&
			PendingRequests[Index].DeadlineSeconds > 0.0 &&
			Now >= PendingRequests[Index].DeadlineSeconds;

		if (bReady || bTimedOut)
		{
			// Move the callback out and drop the entry BEFORE invoking, so a callback that enqueues
			// another request (mutating PendingRequests) can't invalidate our slot.
			FMikanResponseCallback Callback = MoveTemp(PendingRequests[Index].OnComplete);

			if (bTimedOut)
			{
				// Both ends can still report the connection as up while nothing crosses it, so name
				// the recovery here rather than leaving a silently empty scene behind.
				UE_LOG(MikanXRLog, Warning,
					TEXT("Mikan request timed out after %.0f seconds with no response. "
						 "The connection may be stale: disconnect and reconnect to recover."),
					kDefaultRequestTimeoutSeconds);
			}

			PendingRequests.RemoveAtSwap(Index);

			if (Callback)
			{
				if (!Response)
				{
					// Timeout path: synthesize a response so callbacks have a uniform contract.
					Response = std::make_shared<MikanResponse>();
					Response->resultCode = MikanAPIResult::Timeout;
				}

				Callback(Response);
			}
		}
	}
}

void UMikanEngineSubsystem::HandleMikanEvent(MikanEventPtr mikanEvent)
{
	if (typeid(*mikanEvent) == typeid(MikanConnectedEvent))
	{
		HandleMikanConnected();
	}
	else if (typeid(*mikanEvent) == typeid(MikanDisconnectedEvent))
	{
		HandleMikanDisconnected();
	}
	else if (typeid(*mikanEvent) == typeid(MikanCameraNewFrameEvent))
	{
		auto newFrameEvent = std::static_pointer_cast<MikanCameraNewFrameEvent>(mikanEvent);
		HandleCameraNewFrame(*newFrameEvent.get());
	}
	else if (typeid(*mikanEvent) == typeid(MikanLightDMXDataChangedEvent))
	{
		auto lightDMXDataEvent = std::static_pointer_cast<MikanLightDMXDataChangedEvent>(mikanEvent);
		HandleLightDMXDataChanged(*lightDMXDataEvent.get());
	}
	else if (typeid(*mikanEvent) == typeid(MikanPropertyUpdateEvent))
	{
		auto propertyUpdateEvent = std::static_pointer_cast<MikanPropertyUpdateEvent>(mikanEvent);
		HandleComponentPropertyChanged(*propertyUpdateEvent.get());
	}
	else if (typeid(*mikanEvent) == typeid(MikanScriptMessagePostedEvent))
	{
		auto scriptMessageEvent = std::static_pointer_cast<MikanScriptMessagePostedEvent>(mikanEvent);
		HandleScriptMessage(*scriptMessageEvent.get());
	}
	else if (typeid(*mikanEvent) == typeid(MikanAppStageChangedEvent))
	{
		auto appStateMessageEvent = std::static_pointer_cast<MikanAppStageChangedEvent>(mikanEvent);
		HandleAppStateChangedMessage(*appStateMessageEvent.get());
	}
	else if (typeid(*mikanEvent) == typeid(MikanRemoteControlEvent))
	{
		auto remoteControlEvent = std::static_pointer_cast<MikanRemoteControlEvent>(mikanEvent);
		HandleRemoteControlEvent(*remoteControlEvent.get());
	}
}

void UMikanEngineSubsystem::HandleMikanConnected()
{
	if (DataStore)
	{
		DataStore->HandleMikanConnected();
	}

	SendSubscribeToPropertyUpdates();
	SendSubscribeToDMXUpdates();

	if (OnMikanConnected.IsBound())
		OnMikanConnected.Broadcast();
}

void UMikanEngineSubsystem::HandleMikanDisconnected()
{
	if (DataStore)
	{
		DataStore->HandleMikanDisconnected();
	}

	if (OnMikanDisconnected.IsBound())
		OnMikanDisconnected.Broadcast();
}

void UMikanEngineSubsystem::HandleCameraNewFrame(const MikanCameraNewFrameEvent& newFrameEvent)
{
	// Broadcast raw metric values — world subsystem applies WorldToMeters scale
	OnCameraNewFrameRaw.Broadcast(newFrameEvent);
}

void UMikanEngineSubsystem::HandleComponentPropertyChanged(const MikanPropertyUpdateEvent& PropertyUpdateEvent)
{
	const MikanPropertyValue& PropValue = PropertyUpdateEvent.propertyValue;
	const FString SystemName(PropValue.ownerSystem.getUtf8Value());
	const FString FieldName(PropValue.fieldName.getUtf8Value());

	// Route through DataStore to update cache and broadcast delegates to subscribers
	if (DataStore)
	{
		if (PropValue.componentId == INVALID_MIKAN_ID && FieldName.EndsWith(TEXT("ComponentIdList")))
		{
			// Currently there is only one ComponentList per system
			DataStore->HandleListChanged(SystemName);
		}
		else
		{
			DataStore->HandlePropertyUpdateEvent(PropertyUpdateEvent);
		}
	}
}

void UMikanEngineSubsystem::HandleLightDMXDataChanged(const MikanLightDMXDataChangedEvent& DMXDataEvent)
{
	if (DMXDataStore)
	{
		DMXDataStore->ApplyMikanUniverseData(DMXDataEvent);

		if (OnDMXDataChanged.IsBound())
			OnDMXDataChanged.Broadcast();
	}
}

void UMikanEngineSubsystem::HandleScriptMessage(const MikanScriptMessagePostedEvent& ScriptMessageEvent)
{
	if (OnScriptMessage.IsBound())
	{
		auto TCharMessage = 
			StringCast<TCHAR>(ScriptMessageEvent.message.getUtf8Value());
		OnScriptMessage.Broadcast(TCharMessage.Get());
	}
}

void UMikanEngineSubsystem::SendSubscribeToPropertyUpdates()
{
	if (MikanAPI)
	{
		SetPropertyNotifyMode subscribeRequest = {};
		subscribeRequest.notifyMode = MikanPropertyNotifyMode::NAME_AND_VALUE;
		MikanAPI->sendRequest(subscribeRequest).awaitResponse();
	}
}

void UMikanEngineSubsystem::SendSubscribeToDMXUpdates()
{
	if (MikanAPI)
	{
		SetLightDMXDataSubcription subscribeRequest = {};
		subscribeRequest.subscribe = true;
		subscribeRequest.light_ids.clear();
		MikanAPI->sendRequest(subscribeRequest).awaitResponse();
	}
}

bool UMikanEngineSubsystem::SendPushAppStageRequest(const FString& StageName)
{
	const auto StageNameAnsi = StringCast<ANSICHAR>(*StageName);

	PushAppStage PushAppStageRequest = {};
	PushAppStageRequest.app_state_name.setUtf8Value(StageNameAnsi.Get());

	MikanResponsePtr response = MikanAPI->sendRequest(PushAppStageRequest).fetchResponse();
	return response->resultCode == MikanAPIResult::Success;
}

bool UMikanEngineSubsystem::SendPopAppStageRequest()
{
	PopAppStage PopRequest = {};
	MikanResponsePtr response = MikanAPI->sendRequest(PopRequest).fetchResponse();
	return response->resultCode == MikanAPIResult::Success;
}

bool UMikanEngineSubsystem::GetCurrentAppStageName(FString& OutStateName)
{
	GetAppStageInfo GetAppStageRequest = {};
	MikanResponsePtr response = MikanAPI->sendRequest(GetAppStageRequest).fetchResponse();

	if (response->resultCode == MikanAPIResult::Success)
	{
		auto appInfoResponse = 
			std::static_pointer_cast<MikanAppStageInfoResponse>(response);

		OutStateName = FString(appInfoResponse->app_stage_info.app_state_name.getUtf8Value());
		return true;
	}

	return false;
}

void UMikanEngineSubsystem::SendRemoteControlCommand(
	const FString& CommandType,
	const TArray<FString>& CommandArgs,
	bool& OutSuccess,
	TArray<FString>& OutResults)
{
	const auto CommandTypeAnsi = StringCast<ANSICHAR>(*CommandType);

	MikanRemoteControlCommand RemoteControlCommand;
	RemoteControlCommand.command.setUtf8Value(CommandTypeAnsi.Get());

	int32 CommandArgCount = CommandArgs.Num();
	if (CommandArgCount > 0)
	{
		RemoteControlCommand.parameters.resize(CommandArgCount);
		for (int32 ArgIndex = 0; ArgIndex < CommandArgCount; ArgIndex++)
		{
			const auto ArgAnsi = StringCast<ANSICHAR>(*CommandArgs[ArgIndex]);
			RemoteControlCommand.parameters[ArgIndex].setUtf8Value(ArgAnsi.Get());
		}
	}

	// Bounded wait: a zero timeout blocks the game thread until the reply arrives, which is forever
	// if the connection drops mid-request.
	MikanResponsePtr response = MikanAPI->sendRequest(RemoteControlCommand).fetchResponse(kRemoteControlCommandTimeoutMs);
	if (response->resultCode == MikanAPIResult::Success)
	{
		auto commandResult = std::static_pointer_cast<MikanRemoteControlCommandResult>(response);
		OutResults.Empty();

		for (const auto& Result : commandResult->results)
		{
			const FString ResultStr(Result.getUtf8Value());

			OutResults.Add(ResultStr);
		}

		OutSuccess = true;
	}
	else
	{
		OutSuccess = false;
	}
}

void UMikanEngineSubsystem::HandleAppStateChangedMessage(const MikanAppStageChangedEvent& AppStageChangedEvent)
{
	if (OnAppStageChanged.IsBound())
	{
		const FString FromStageName(AppStageChangedEvent.old_app_state_name.getUtf8Value());
		const FString ToStageName(AppStageChangedEvent.new_app_state_name.getUtf8Value());

		OnAppStageChanged.Broadcast(FromStageName, ToStageName);
	}
}

void UMikanEngineSubsystem::HandleRemoteControlEvent(const MikanRemoteControlEvent& RemoteControlEvent)
{
	if (OnRemoteControlEvent.IsBound())
	{
		const FString EventTypeName(RemoteControlEvent.remoteControlEvent.getUtf8Value());

		TArray<FString> EventData;
		for (const auto& Param : RemoteControlEvent.parameters)
		{
			const FString ParamStr(Param.getUtf8Value());

			EventData.Add(ParamStr);
		}

		OnRemoteControlEvent.Broadcast(EventTypeName, EventData);
	}
}
