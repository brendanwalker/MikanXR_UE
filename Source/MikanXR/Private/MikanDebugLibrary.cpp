#include "MikanDebugLibrary.h"
#include "MikanClient.h"
#include "MikanDataStore.h"
#include "MikanDMXDataStore.h"
#include "MikanEngineSubsystem.h"
#include "MikanTransformActor.h"
#include "MikanAPI.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"

namespace
{
	const TCHAR* GraphicsApiName(MikanClientGraphicsApi GraphicsApi)
	{
		switch (GraphicsApi)
		{
		case MikanClientGraphicsApi_Direct3D9: return TEXT("Direct3D9");
		case MikanClientGraphicsApi_Direct3D11: return TEXT("Direct3D11");
		case MikanClientGraphicsApi_Direct3D12: return TEXT("Direct3D12");
		case MikanClientGraphicsApi_OpenGL: return TEXT("OpenGL");
		case MikanClientGraphicsApi_Metal: return TEXT("Metal");
		case MikanClientGraphicsApi_Vulkan: return TEXT("Vulkan");
		default: return TEXT("UNKNOWN");
		}
	}

	FString MikanStringToFString(const Serialization::String& MikanString)
	{
		FString Result;
		Mikan::ToUnrealString(MikanString, Result);
		return Result;
	}

	TArray<FString> ErrorLines(const FString& Message)
	{
		TArray<FString> Lines;
		Lines.Add(TEXT("error: ") + Message);
		return Lines;
	}
}

// -- Connection -----
TArray<FString> UMikanDebugLibrary::GetConnectionStatus()
{
	TArray<FString> Lines;
	UMikanEngineSubsystem* Subsystem = FindEngineSubsystem(Lines);
	if (!Subsystem)
	{
		return Lines;
	}

	const MikanClientInfo& ClientInfo = Subsystem->GetClientInfo();
	Mikan::AppendDescribeLine(Lines, TEXT("connected"), Subsystem->GetIsConnected());
	Mikan::AppendDescribeLine(Lines, TEXT("wants_connected"), Subsystem->GetWantsToBeConnected());
	Mikan::AppendDescribeLine(Lines, TEXT("pending_requests"), Subsystem->GetPendingRequestCount());
	Mikan::AppendDescribeLine(Lines, TEXT("client_id"), MikanStringToFString(ClientInfo.clientId));
	Mikan::AppendDescribeLine(Lines, TEXT("engine_name"), MikanStringToFString(ClientInfo.engineName));
	Mikan::AppendDescribeLine(Lines, TEXT("engine_version"), MikanStringToFString(ClientInfo.engineVersion));
	Mikan::AppendDescribeLine(Lines, TEXT("application_name"), MikanStringToFString(ClientInfo.applicationName));
	Mikan::AppendDescribeLine(Lines, TEXT("application_version"), MikanStringToFString(ClientInfo.applicationVersion));
	Mikan::AppendDescribeLine(Lines, TEXT("graphics_api"), FString(GraphicsApiName(ClientInfo.graphicsAPI)));
	return Lines;
}

TArray<FString> UMikanDebugLibrary::SetWantsToBeConnected(bool bConnect)
{
	TArray<FString> Lines;
	UMikanEngineSubsystem* Subsystem = FindEngineSubsystem(Lines);
	if (!Subsystem)
	{
		return Lines;
	}

	Subsystem->SetWantsToBeConnected(bConnect);
	Mikan::AppendDescribeLine(Lines, TEXT("wants_connected"), Subsystem->GetWantsToBeConnected());
	return Lines;
}

TArray<FString> UMikanDebugLibrary::GetAppStage()
{
	TArray<FString> Lines;
	UMikanEngineSubsystem* Subsystem = FindEngineSubsystem(Lines);
	if (!Subsystem)
	{
		return Lines;
	}
	if (!Subsystem->GetIsConnected())
	{
		return ErrorLines(TEXT("not connected to Mikan"));
	}

	FString StageName;
	if (!Subsystem->GetCurrentAppStageName(StageName))
	{
		return ErrorLines(TEXT("get app stage request failed"));
	}

	Mikan::AppendDescribeLine(Lines, TEXT("app_stage"), StageName);
	return Lines;
}

TArray<FString> UMikanDebugLibrary::PushAppStage(const FString& StageName)
{
	TArray<FString> Lines;
	UMikanEngineSubsystem* Subsystem = FindEngineSubsystem(Lines);
	if (!Subsystem)
	{
		return Lines;
	}
	if (!Subsystem->GetIsConnected())
	{
		return ErrorLines(TEXT("not connected to Mikan"));
	}
	if (!Subsystem->SendPushAppStageRequest(StageName))
	{
		return ErrorLines(FString::Printf(TEXT("push app stage '%s' failed"), *StageName));
	}
	return Lines;
}

TArray<FString> UMikanDebugLibrary::PopAppStage()
{
	TArray<FString> Lines;
	UMikanEngineSubsystem* Subsystem = FindEngineSubsystem(Lines);
	if (!Subsystem)
	{
		return Lines;
	}
	if (!Subsystem->GetIsConnected())
	{
		return ErrorLines(TEXT("not connected to Mikan"));
	}
	if (!Subsystem->SendPopAppStageRequest())
	{
		return ErrorLines(TEXT("pop app stage failed"));
	}
	return Lines;
}

TArray<FString> UMikanDebugLibrary::SendRemoteControlCommand(const FString& CommandType, const TArray<FString>& CommandArgs)
{
	TArray<FString> Lines;
	UMikanEngineSubsystem* Subsystem = FindEngineSubsystem(Lines);
	if (!Subsystem)
	{
		return Lines;
	}
	if (!Subsystem->GetIsConnected())
	{
		return ErrorLines(TEXT("not connected to Mikan"));
	}

	bool bSuccess = false;
	TArray<FString> Results;
	Subsystem->SendRemoteControlCommand(CommandType, CommandArgs, bSuccess, Results);
	if (!bSuccess)
	{
		return ErrorLines(FString::Printf(TEXT("remote control command '%s' failed"), *CommandType));
	}
	return Results;
}

// -- Component data store -----
TArray<FString> UMikanDebugLibrary::ListSystems()
{
	TArray<FString> Lines;
	UMikanDataStore* DataStore = FindDataStore(Lines);
	if (!DataStore)
	{
		return Lines;
	}

	for (const auto& Pair : DataStore->GetSystemsTable())
	{
		const UMikanComponentSystem* System = Pair.Value;
		Lines.Add(FString::Printf(TEXT("%s %s %d"),
			*System->GetSystemName(),
			*System->GetComponentClassName(),
			System->GetComponentDataTable().Num()));
	}
	return Lines;
}

TArray<FString> UMikanDebugLibrary::ListComponents(const FString& SystemName)
{
	TArray<FString> Lines;
	UMikanComponentSystem* System = FindComponentSystem(SystemName, Lines);
	if (!System)
	{
		return Lines;
	}

	for (const auto& Pair : System->GetComponentDataTable())
	{
		Lines.Add(FString::Printf(TEXT("%d %s"), Pair.Key, *Pair.Value->GetComponentName()));
	}
	return Lines;
}

TArray<FString> UMikanDebugLibrary::DescribeComponent(const FString& SystemName, int32 ComponentId)
{
	TArray<FString> Lines;
	UMikanComponentSystem* System = FindComponentSystem(SystemName, Lines);
	if (!System)
	{
		return Lines;
	}

	// A component id of -1 names the system itself, matching how Mikan addresses
	// system-level properties on the wire and in its own automation channel
	if (ComponentId == INVALID_MIKAN_ID)
	{
		const UMikanSystemData* SystemData = System->GetSystemData();
		if (!SystemData)
		{
			return ErrorLines(FString::Printf(TEXT("no system values in %s"), *SystemName));
		}

		SystemData->Describe(Lines);
		return Lines;
	}

	const UMikanComponentData* ComponentData = System->FindComponentDataById(ComponentId);
	if (!ComponentData)
	{
		return ErrorLines(FString::Printf(TEXT("no component %d in %s"), ComponentId, *SystemName));
	}

	ComponentData->Describe(Lines);
	return Lines;
}

// -- DMX -----
TArray<FString> UMikanDebugLibrary::ListDMXUniverses()
{
	TArray<FString> Lines;
	UMikanEngineSubsystem* Subsystem = FindEngineSubsystem(Lines);
	if (!Subsystem)
	{
		return Lines;
	}

	const UMikanDMXDataStore* DMXDataStore = Subsystem->GetDMXDataStore();
	if (!DMXDataStore)
	{
		return ErrorLines(TEXT("no DMX data store"));
	}

	Lines.Add(FString::Printf(TEXT("last_update_server_time %g"), DMXDataStore->GetLastUpdateServerTime()));
	for (const auto& Pair : DMXDataStore->GetDMXUniverses())
	{
		const UMikanDMXUniverse* Universe = Pair.Value;
		int32 NonZeroChannels = 0;
		for (int32 Channel = 1; Channel <= UMikanDMXUniverse::kDMXUniverseChannelCount; ++Channel)
		{
			if (Universe->ReadChannelValue(Channel) != 0)
			{
				++NonZeroChannels;
			}
		}
		Lines.Add(FString::Printf(TEXT("universe %d %d"), Universe->GetUniverseId(), NonZeroChannels));
	}
	return Lines;
}

TArray<FString> UMikanDebugLibrary::DescribeDMXUniverse(int32 UniverseId)
{
	TArray<FString> Lines;
	UMikanEngineSubsystem* Subsystem = FindEngineSubsystem(Lines);
	if (!Subsystem)
	{
		return Lines;
	}

	UMikanDMXDataStore* DMXDataStore = Subsystem->GetDMXDataStore();
	const UMikanDMXUniverse* Universe = DMXDataStore ? DMXDataStore->GetDMXUniverse(UniverseId) : nullptr;
	if (!Universe)
	{
		return ErrorLines(FString::Printf(TEXT("no DMX universe %d"), UniverseId));
	}

	for (int32 Channel = 1; Channel <= UMikanDMXUniverse::kDMXUniverseChannelCount; ++Channel)
	{
		const uint8 Value = Universe->ReadChannelValue(Channel);
		if (Value != 0)
		{
			Lines.Add(FString::Printf(TEXT("%d %d"), Channel, (int32)Value));
		}
	}
	return Lines;
}

// -- Spawned actors -----
TArray<FString> UMikanDebugLibrary::GetMikanClientPath()
{
	TArray<FString> Lines;
	AMikanClient* Client = FindMikanClient(Lines);
	if (!Client)
	{
		return Lines;
	}

	Lines.Add(Client->GetPathName());
	return Lines;
}

TArray<FString> UMikanDebugLibrary::ListSpawnedActors()
{
	TArray<FString> Lines;
	AMikanClient* Client = FindMikanClient(Lines);
	if (!Client)
	{
		return Lines;
	}

	for (const auto& SystemPair : Client->GetSystemActorTable())
	{
		for (const auto& ActorPair : SystemPair.Value.SpawnedActorsTable)
		{
			const AMikanTransformActor* Actor = ActorPair.Value;
			if (!Actor)
			{
				continue;
			}

			const UMikanTransformData* TransformData = Actor->GetTransformData();
			Lines.Add(FString::Printf(TEXT("%s %d %d %s"),
				*SystemPair.Key,
				ActorPair.Key,
				TransformData ? (int32)TransformData->GetParentTransformId() : -1,
				*Actor->GetPathName()));
		}
	}
	return Lines;
}

TArray<FString> UMikanDebugLibrary::DescribeActor(int32 TransformId)
{
	TArray<FString> Lines;
	AMikanClient* Client = FindMikanClient(Lines);
	if (!Client)
	{
		return Lines;
	}

	const AMikanTransformActor* Actor = Client->GetTransformActorByIdConst(TransformId);
	if (!Actor)
	{
		return ErrorLines(FString::Printf(TEXT("no spawned actor for transform %d"), TransformId));
	}

	const UMikanTransformData* TransformData = Actor->GetTransformData();
	const AMikanTransformActor* ParentActor = Actor->GetParentTransformActor();
	const AActor* AttachParent = Actor->GetAttachParentActor();

	Mikan::AppendDescribeLine(Lines, TEXT("transform_id"), (int32)Actor->GetTransformId());
	Mikan::AppendDescribeLine(Lines, TEXT("transform_name"), Actor->GetTransformName());
	Mikan::AppendDescribeLine(Lines, TEXT("class"), Actor->GetClass()->GetName());
	Mikan::AppendDescribeLine(Lines, TEXT("path"), Actor->GetPathName());
	Mikan::AppendDescribeLine(Lines, TEXT("owner_system"),
		TransformData ? TransformData->GetOwnerSystem()->GetSystemName() : FString(TEXT("none")));
	Mikan::AppendDescribeLine(Lines, TEXT("parent_transform_id"),
		TransformData ? (int32)TransformData->GetParentTransformId() : -1);
	Mikan::AppendDescribeLine(Lines, TEXT("parent_transform_actor"),
		ParentActor ? ParentActor->GetPathName() : FString(TEXT("none")));
	Mikan::AppendDescribeLine(Lines, TEXT("attach_parent"),
		AttachParent ? AttachParent->GetPathName() : FString(TEXT("none")));
	Mikan::AppendDescribeLine(Lines, TEXT("hidden"), Actor->IsHidden());
	AppendTransformLines(Lines, TEXT("relative"), Actor->GetRootComponent()->GetRelativeTransform());
	AppendTransformLines(Lines, TEXT("world"), Actor->GetActorTransform());
	return Lines;
}

TArray<FString> UMikanDebugLibrary::RefetchFromMikan()
{
	TArray<FString> Lines;
	AMikanClient* Client = FindMikanClient(Lines);
	if (!Client)
	{
		return Lines;
	}

#if WITH_EDITOR
	Client->EditorRefetchFromMikan();
	return Lines;
#else
	return ErrorLines(TEXT("refetch is editor only"));
#endif
}

TArray<FString> UMikanDebugLibrary::GetFetchStatus()
{
	TArray<FString> Lines;
	AMikanClient* Client = FindMikanClient(Lines);
	if (!Client)
	{
		return Lines;
	}

#if WITH_EDITOR
	const TCHAR* StatusName = TEXT("idle");
	switch (Client->GetEditorFetchStatus())
	{
	case AMikanClient::EEditorFetchStatus::InProgress: StatusName = TEXT("in_progress"); break;
	case AMikanClient::EEditorFetchStatus::Succeeded: StatusName = TEXT("succeeded"); break;
	case AMikanClient::EEditorFetchStatus::Failed: StatusName = TEXT("failed"); break;
	default: break;
	}

	Mikan::AppendDescribeLine(Lines, TEXT("fetch_status"), FString(StatusName));
	Mikan::AppendDescribeLine(Lines, TEXT("fetch_message"), Client->GetEditorFetchMessage());
	return Lines;
#else
	return ErrorLines(TEXT("fetch status is editor only"));
#endif
}

TArray<FString> UMikanDebugLibrary::ClearMikanActors()
{
	TArray<FString> Lines;
	AMikanClient* Client = FindMikanClient(Lines);
	if (!Client)
	{
		return Lines;
	}

#if WITH_EDITOR
	Client->EditorClearMikanActors();
	return Lines;
#else
	return ErrorLines(TEXT("clear is editor only"));
#endif
}

// -- Helpers -----
UWorld* UMikanDebugLibrary::ResolveDebugWorld()
{
	if (!GEngine)
	{
		return nullptr;
	}

	// A running PIE session or a game process wins. Otherwise fall back to the editor world.
	UWorld* EditorWorld = nullptr;
	for (const FWorldContext& Context : GEngine->GetWorldContexts())
	{
		UWorld* World = Context.World();
		if (!World)
		{
			continue;
		}
		if (Context.WorldType == EWorldType::PIE || Context.WorldType == EWorldType::Game)
		{
			return World;
		}
		if (Context.WorldType == EWorldType::Editor)
		{
			EditorWorld = World;
		}
	}
	return EditorWorld;
}

UMikanEngineSubsystem* UMikanDebugLibrary::FindEngineSubsystem(TArray<FString>& OutLines)
{
	UMikanEngineSubsystem* Subsystem = UMikanEngineSubsystem::Get();
	if (!Subsystem)
	{
		OutLines = ErrorLines(TEXT("Mikan engine subsystem unavailable"));
	}
	return Subsystem;
}

UMikanDataStore* UMikanDebugLibrary::FindDataStore(TArray<FString>& OutLines)
{
	UMikanEngineSubsystem* Subsystem = FindEngineSubsystem(OutLines);
	if (!Subsystem)
	{
		return nullptr;
	}

	UMikanDataStore* DataStore = Subsystem->GetDataStore();
	if (!DataStore)
	{
		OutLines = ErrorLines(TEXT("no component data store"));
	}
	return DataStore;
}

UMikanComponentSystem* UMikanDebugLibrary::FindComponentSystem(const FString& SystemName, TArray<FString>& OutLines)
{
	UMikanDataStore* DataStore = FindDataStore(OutLines);
	if (!DataStore)
	{
		return nullptr;
	}

	UMikanComponentSystem* const* SystemEntry = DataStore->GetSystemsTable().Find(SystemName);
	if (!SystemEntry || !*SystemEntry)
	{
		OutLines = ErrorLines(FString::Printf(TEXT("unknown system '%s' (see ListSystems)"), *SystemName));
		return nullptr;
	}
	return *SystemEntry;
}

AMikanClient* UMikanDebugLibrary::FindMikanClient(TArray<FString>& OutLines)
{
	UWorld* World = ResolveDebugWorld();
	if (!World)
	{
		OutLines = ErrorLines(TEXT("no world"));
		return nullptr;
	}

	for (TActorIterator<AMikanClient> It(World); It; ++It)
	{
		return *It;
	}

	OutLines = ErrorLines(FString::Printf(TEXT("no MikanClient actor in world %s"), *World->GetPathName()));
	return nullptr;
}

void UMikanDebugLibrary::AppendTransformLines(TArray<FString>& OutLines, const TCHAR* Prefix, const FTransform& Transform)
{
	Mikan::AppendDescribeLine(OutLines, *FString::Printf(TEXT("%s_location"), Prefix), Transform.GetLocation());
	Mikan::AppendDescribeLine(OutLines, *FString::Printf(TEXT("%s_rotation"), Prefix), Transform.GetRotation());
	Mikan::AppendDescribeLine(OutLines, *FString::Printf(TEXT("%s_scale"), Prefix), Transform.GetScale3D());
}
