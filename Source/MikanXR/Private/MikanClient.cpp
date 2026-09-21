#include "MikanClient.h"
#include "MikanEngineSubsystem.h"
#include "MikanDataStore.h"
#include "IMikanXRModule.h"
#include "MikanPropertyRequests.h"
#include "MikanMath.h"
#include "MikanCameraEvents.h"
#include "MikanAPI.h"
#include "Engine/World.h"
#include "GameFramework/WorldSettings.h"
#include "Kismet/GameplayStatics.h"

#include "MikanAnchorActor.h"
#include "MikanAnchorTypes.h"
#include "MikanBoxShapeActor.h"
#include "MikanBoxStencilActor.h"
#include "MikanCameraActor.h"
#include "MikanCameraTypes.h"
#include "MikanLightTypes.h"
#include "MikanLightEnvironmentActor.h"
#include "MikanModelShapeActor.h"
#include "MikanModelStencilActor.h"
#include "MikanQuadShapeActor.h"
#include "MikanQuadStencilActor.h"
#include "MikanRGBPixelGridActor.h"
#include "MikanRGBSpotLightActor.h"
#include "MikanSceneActor.h"
#include "MikanSceneTypes.h"
#include "MikanStageActor.h"
#include "MikanStageTypes.h"
#include "MikanShapeTypes.h"


AMikanClient::AMikanClient(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = true;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("MikanClientRoot"));

	// Set defaults for all mikan actor class types
	AnchorClass = AMikanAnchorActor::StaticClass();
	QuadStencilClass = AMikanQuadStencilActor::StaticClass();
	BoxStencilClass = AMikanBoxStencilActor::StaticClass();
	ModelStencilClass = AMikanModelStencilActor::StaticClass();
	QuadShapeClass = AMikanQuadShapeActor::StaticClass();
	BoxShapeClass = AMikanBoxShapeActor::StaticClass();
	ModelShapeClass = AMikanModelShapeActor::StaticClass();
	CameraClass = AMikanCameraActor::StaticClass();
	SceneClass = AMikanSceneActor::StaticClass();
	StageClass = AMikanStageActor::StaticClass();
	RGBSpotLightClass = AMikanRGBSpotLightActor::StaticClass();
	RGBPixelGridCameraClass = AMikanRGBPixelGridActor::StaticClass();
}

void AMikanClient::PostRegisterAllComponents()
{
	Super::PostRegisterAllComponents();

	BindToEngineSubsystem();
	SyncAllSpawnedActors();
}

void AMikanClient::Destroyed()
{
	UnbindFromEngineSubsystem();

	Super::Destroyed();
}

void AMikanClient::BeginPlay()
{
	Super::BeginPlay();

	SyncAllSpawnedActors();
}

UClass* AMikanClient::GetMikanActorClassForSystem(const FString& SystemName) const
{
	if (SystemName == FString(MikanAnchorComponentValues::k_ownerSystemName))
	{
		return AnchorClass;
	}
	else if (SystemName == FString(MikanQuadStencilComponentValues::k_ownerSystemName))
	{
		return QuadStencilClass;
	}
	else if (SystemName == FString(MikanBoxStencilComponentValues::k_ownerSystemName))
	{
		return BoxStencilClass;
	}
	else if (SystemName == FString(MikanModelStencilComponentValues::k_ownerSystemName))
	{
		return ModelStencilClass;
	}
	else if (SystemName == FString(MikanQuadShapeComponentValues::k_ownerSystemName))
	{
		return QuadShapeClass;
	}
	else if (SystemName == FString(MikanBoxShapeComponentValues::k_ownerSystemName))
	{
		return BoxShapeClass;
	}
	else if (SystemName == FString(MikanModelShapeComponentValues::k_ownerSystemName))
	{
		return ModelShapeClass;
	}
	else if (SystemName == FString(MikanCameraComponentValues::k_ownerSystemName))
	{
		return CameraClass;
	}
	else if (SystemName == FString(MikanSceneComponentValues::k_ownerSystemName))
	{
		return SceneClass;
	}
	else if (SystemName == FString(MikanStageComponentValues::k_ownerSystemName))
	{
		return StageClass;
	}
	else if (SystemName == FString(MikanRGBSpotLightComponentValues::k_ownerSystemName))
	{
		return RGBSpotLightClass;
	}
	else if (SystemName == FString(MikanRGBPixelGridComponentValues::k_ownerSystemName))
	{
		return RGBPixelGridCameraClass;
	}
	else if (SystemName == FString(MikanLightEnvironmentComponentValues::k_ownerSystemName))
	{
		return LightEnvironmentClass;
	}

	return nullptr;
}

AMikanTransformActor* AMikanClient::SpawnMikanActor(UMikanTransformData* ComponentData)
{
	AMikanTransformActor* NewActor = nullptr;

	const FString& SystemName = ComponentData->GetOwnerSystem()->GetSystemName();
	UClass* ActorClass= GetMikanActorClassForSystem(SystemName);

	if (ActorClass)
	{
		UWorld* World = GetWorld();
		NewActor =
			World->SpawnActorDeferred<AMikanTransformActor>(
				ActorClass, FTransform::Identity, this);

		if (NewActor)
		{
			FTransform RelativeTransform = ComponentData->ComputeRelativeTransform(World);

			// For stage actors: prefer a user-saved transform over the Mikan API value.
			if (NewActor->IsA<AMikanStageActor>())
			{
				const int32 ComponentId = ComponentData->GetComponentId();
				if (const FTransform* SavedTransform = GetSavedStageTransform(ComponentId))
				{
					RelativeTransform = *SavedTransform;
				}
			}

			NewActor->SetFlags(RF_Transient); // Don't let this actor get serialized
			NewActor->FinishSpawning(RelativeTransform);
			
			// Apply the component data after the actor has spawned
			NewActor->BindMikanComponentData(ComponentData);
		}
	}

	return NewActor;
}

void AMikanClient::BindToEngineSubsystem()
{
	UMikanEngineSubsystem* EngineSubsystem = UMikanEngineSubsystem::Get();
	if (!EngineSubsystem) return;

	EngineSubsystem->OnMikanConnected.AddUniqueDynamic(this, &AMikanClient::HandleMikanConnected);
	EngineSubsystem->OnMikanDisconnected.AddUniqueDynamic(this, &AMikanClient::HandleMikanDisconnected);
	EngineSubsystem->OnDMXDataChanged.AddUniqueDynamic(this, &AMikanClient::HandleDMXDataChanged);
	EngineSubsystem->OnScriptMessage.AddUniqueDynamic(this, &AMikanClient::HandleScriptMessage);
	EngineSubsystem->OnCameraNewFrameRaw.AddUObject(this, &AMikanClient::HandleCameraNewFrameRaw);

	DataStore = EngineSubsystem->GetDataStore();
	DataStore->OnComponentListChanged.AddUObject(this, &AMikanClient::HandleComponentListChanged);
	DataStore->OnComponentsBatchRefreshed.AddUObject(this, &AMikanClient::RefreshAllSpawnedActorAttachments);
	DataStore->OnSystemDataChanged.AddUObject(this, &AMikanClient::HandleSystemDataChanged);

	// If the engine subsystem is already connected (e.g. PIE starting mid-session),
	// sync all actors immediately
	if (EngineSubsystem->GetIsConnected())
	{
		HandleMikanConnected();
	}
}

IMikanAPI* AMikanClient::GetMikanAPI() const
{
	UMikanEngineSubsystem* EngineSubsystem = UMikanEngineSubsystem::Get();
	if (EngineSubsystem)
	{
		return EngineSubsystem->GetMikanAPI();
	}

	return nullptr;
}

const MikanClientInfo* AMikanClient::GetClientInfo() const
{
	UMikanEngineSubsystem* EngineSubsystem = UMikanEngineSubsystem::Get();
	if (!EngineSubsystem)
	{
		return &EngineSubsystem->GetClientInfo();
	}

	return nullptr;
}

void AMikanClient::UnbindFromEngineSubsystem()
{
	if (DataStore)
	{
		DataStore->OnComponentListChanged.RemoveAll(this);
		DataStore->OnComponentsBatchRefreshed.RemoveAll(this);
		DataStore->OnSystemDataChanged.RemoveAll(this);
		DataStore = nullptr;
	}

	UMikanEngineSubsystem* EngineSubsystem = UMikanEngineSubsystem::Get();
	if (EngineSubsystem)
	{
		EngineSubsystem->OnMikanConnected.RemoveDynamic(this, &AMikanClient::HandleMikanConnected);
		EngineSubsystem->OnMikanDisconnected.RemoveDynamic(this, &AMikanClient::HandleMikanDisconnected);
		EngineSubsystem->OnDMXDataChanged.RemoveDynamic(this, &AMikanClient::HandleDMXDataChanged);
		EngineSubsystem->OnScriptMessage.RemoveDynamic(this, &AMikanClient::HandleScriptMessage);
		EngineSubsystem->OnCameraNewFrameRaw.RemoveAll(this);
	}
}

void AMikanClient::SetActiveMikanScene(AMikanSceneActor* DesiredScene)
{
	if (DesiredScene != ActiveMikanScene)
	{
		AMikanSceneActor* OldScene = ActiveMikanScene;
		ActiveMikanScene = DesiredScene;

		if (OldScene != nullptr)
			OldScene->HandleSceneDeactivated();
		if (ActiveMikanScene != nullptr)
			ActiveMikanScene->HandleSceneActivated();

		RefreshSceneVisibility();

		OnActiveSceneChanged.Broadcast(OldScene, ActiveMikanScene);
	}
}

void AMikanClient::RefreshActiveSceneFromEditor()
{
	if (!DataStore)
	{
		return;
	}

	UMikanComponentSystem* SceneSystem = DataStore->GetComponentSystem(MikanSceneSystemValues::k_systemName);
	if (!SceneSystem)
	{
		return;
	}

	UMikanSceneSystemData* SceneSystemData = SceneSystem->GetTypedSystemData<UMikanSceneSystemData>();
	if (!SceneSystemData)
	{
		return;
	}

	// The actor may not be spawned yet when the system values land first. The batch-refresh
	// path calls this again once every actor exists, so the order of the two arrivals is moot.
	AMikanSceneActor* DesiredScene = Cast<AMikanSceneActor>(
		GetTransformActorById(SceneSystemData->GetCurrentSceneId()));

	SetActiveMikanScene(DesiredScene);
}

void AMikanClient::RefreshSceneVisibility()
{
	FMikanSystemActors* SceneActors = SystemActorTable.Find(FString(MikanSceneSystemValues::k_systemName));
	if (!SceneActors)
	{
		return;
	}

	for (const auto& Pair : SceneActors->SpawnedActorsTable)
	{
		AMikanSceneActor* SceneActor = Cast<AMikanSceneActor>(Pair.Value);
		if (!SceneActor)
		{
			continue;
		}

		UMikanSceneData* SceneData = SceneActor->GetSceneData();
		const bool bForceRender = SceneData && SceneData->GetForceRender();
		const bool bVisible = (SceneActor == ActiveMikanScene) || bForceRender;

		SceneActor->SetSceneSubtreeHidden(!bVisible);
	}
}

bool AMikanClient::RegisterMikanRenderable(class UMikanRenderableComponent* Renderable)
{
	if (Renderable != nullptr && !RegisteredRenderables.Contains(Renderable))
	{
		RegisteredRenderables.Add(Renderable);
		OnRenderableRegistered.Broadcast(Renderable);
		return true;
	}

	return false;
}

void AMikanClient::UnregisterMikanRenderable(class UMikanRenderableComponent* Renderable)
{
	if (Renderable != nullptr && RegisteredRenderables.Contains(Renderable))
	{
		RegisteredRenderables.Remove(Renderable);
		OnRenderableUnregistered.Broadcast(Renderable);
	}
}

#if WITH_EDITOR
void AMikanClient::EditorRefetchFromMikan()
{
	EditorClearMikanActors();

	if (!DataStore)
	{
		EditorFetchStatus = EEditorFetchStatus::Failed;
		EditorFetchMessage = TEXT("no component data store");
		return;
	}

	EditorFetchStatus = EEditorFetchStatus::InProgress;
	EditorFetchMessage.Reset();

	// FetchAllComponents is now asynchronous: actors respawn over the next frames as the
	// per-system OnComponentListChanged and OnComponentsBatchRefreshed broadcasts fire. Run a
	// final full sync once the whole store has been rebuilt.
	TWeakObjectPtr<AMikanClient> WeakThis(this);
	DataStore->FetchAllComponents(
		[WeakThis](bool bSuccess)
		{
			AMikanClient* Self = WeakThis.Get();
			if (!Self)
			{
				return;
			}

			Self->SyncAllSpawnedActors();

			int32 ComponentCount = 0;
			if (Self->DataStore)
			{
				for (const auto& Pair : Self->DataStore->GetSystemsTable())
				{
					if (Pair.Value)
					{
						ComponentCount += Pair.Value->GetComponentDataTable().Num();
					}
				}
			}

			if (bSuccess)
			{
				Self->EditorFetchStatus = EEditorFetchStatus::Succeeded;
				Self->EditorFetchMessage = FString::Printf(TEXT("%d components"), ComponentCount);
			}
			else
			{
				Self->EditorFetchStatus = EEditorFetchStatus::Failed;
				Self->EditorFetchMessage =
					FString::Printf(TEXT("%d components, some requests went unanswered"), ComponentCount);

				UE_LOG(MikanXRLog, Error,
					TEXT("Fetch From Mikan finished with unanswered requests. The scene is incomplete: "
						 "disconnect and reconnect, then fetch again."));
			}
		});
}

void AMikanClient::EditorClearMikanActors()
{
	DespawnAllSpawnedActors();
	
	if (DataStore)
	{
		DataStore->FlushAllComponents();
	}

	EditorFetchStatus = EEditorFetchStatus::Idle;
	EditorFetchMessage.Reset();
}
#endif

void AMikanClient::SyncAllSpawnedActors()
{
	if (!DataStore)
		return;

	// Spawn/Despawn actors for each system
	for (const auto& Pair : DataStore->GetSystemsTable())
	{
		const UMikanComponentSystem* System = Pair.Value;

		SyncSystemSpawnedActors(System, false);
	}

	// Refresh actor attachments now that all actor spawn / despawn changes are made
	RefreshAllSpawnedActorAttachments();
}

void AMikanClient::HandleComponentListChanged(
	const UMikanComponentSystem* System)
{
	// During a full-store batch fetch, defer the attachment refresh until every system has
	// synced (OnComponentsBatchRefreshed). Refreshing here would run against partially-synced
	// state where other systems' actors may still be stale or pending destruction.
	const bool bRefreshAttachments = !(DataStore && DataStore->IsBatchFetching());

	SyncSystemSpawnedActors(System, bRefreshAttachments);
}

void AMikanClient::HandleSystemDataChanged(const UMikanComponentSystem* System, const FString& FieldName)
{
	// An empty field name is the initial fetch, which carries current_scene_id along with
	// everything else the system holds
	if (System && System->GetSystemName() == MikanSceneSystemValues::k_systemName
		&& (FieldName.IsEmpty() || FieldName == "current_scene_id"))
	{
		RefreshActiveSceneFromEditor();
	}
}

void AMikanClient::SyncSystemSpawnedActors(
	const UMikanComponentSystem* System,
	bool bRefreshAttachments)
{
	const FString& SystemName = System->GetSystemName();
	const TMap<int32, UMikanComponentData*>& SystemComponentsTable= System->GetComponentDataTable();

	// Get the spawned actors table for the given system name
	// (Create a new entry if one doesn't exist)
	FMikanSystemActors& SystemActors = SystemActorTable.FindOrAdd(SystemName);

	// Destroy system actors that no longer have a corresponding entry in the system components table
	for (auto It = SystemActors.SpawnedActorsTable.CreateIterator(); It; ++It)
	{
		const MikanComponentID ComponentId = It.Key();
		AMikanTransformActor* SpawnedActor = It.Value();

		if (!SystemComponentsTable.Contains(ComponentId))
		{
			if (SpawnedActor)
			{
				SpawnedActor->Destroy();
			}
			It.RemoveCurrent();
		}
	}

	// Spawn new system actors corresponding to new system component entries
	for (const auto& Pair : SystemComponentsTable)
	{
		MikanComponentID ComponentId = Pair.Key;
		UMikanTransformData* TransformComponentData= Cast<UMikanTransformData>(Pair.Value);

		if (TransformComponentData)
		{
			AMikanTransformActor* TransformActor = nullptr;

			if (AMikanTransformActor** Entry = SystemActors.SpawnedActorsTable.Find(ComponentId))
			{
				TransformActor = *Entry;

				// On reconnect the DataStore rebuilds component data objects, so an existing
				// actor may still be bound to a now-orphaned data object. Rebind it to the
				// fresh data so it tracks the current values and property-change delegates.
				if (TransformActor)
				{
					TransformActor->BindMikanComponentData(TransformComponentData);
				}
			}
			else
			{
				TransformActor = SpawnMikanActor(TransformComponentData);
				if (TransformActor)
				{
					SystemActors.SpawnedActorsTable.Add(ComponentId, TransformActor);
				}
			}
		}
	}

	if (bRefreshAttachments)
	{
		RefreshAllSpawnedActorAttachments();
	}
}

void AMikanClient::DespawnAllSpawnedActors()
{
	for (auto& SystemActorTablePair : SystemActorTable)
	{
		FMikanSystemActors& SystemActors = SystemActorTablePair.Value;

		for (auto& SpawnedActorTablePair : SystemActors.SpawnedActorsTable)
		{
			AMikanTransformActor* SpawnedActor = SpawnedActorTablePair.Value;

			if (SpawnedActor)
			{
				SpawnedActor->Destroy();
			}
		}
	}

	SystemActorTable.Reset();
}

void AMikanClient::RefreshAllSpawnedActorAttachments()
{
	for (auto& SystemActorTablePair : SystemActorTable)
	{
		FMikanSystemActors& SystemActors = SystemActorTablePair.Value;

		for (auto& SpawnedActorTablePair : SystemActors.SpawnedActorsTable)
		{
			AMikanTransformActor* SpawnedActor = SpawnedActorTablePair.Value;

			if (SpawnedActor)
			{
				SpawnedActor->OnMikanAttachmentChanged();
			}
		}
	}

	// Attachment is what the hide walk follows, and newly spawned actors arrive visible,
	// so the scene gate is re-applied every time the tree is rebuilt
	RefreshActiveSceneFromEditor();
	RefreshSceneVisibility();
}

const AMikanTransformActor* AMikanClient::GetTransformActorByIdConst(int32 TransformID) const
{
	for (const auto& Pair : SystemActorTable)
	{
		const FMikanSystemActors& SystemActors = Pair.Value;
		const AMikanTransformActor* const* Entry = SystemActors.SpawnedActorsTable.Find(TransformID);

		if (Entry)
		{
			return *Entry;
		}
	}

	return nullptr;
}

AMikanTransformActor* AMikanClient::GetTransformActorById(int32 TransformID)
{
	return const_cast<AMikanTransformActor*>(GetTransformActorByIdConst(TransformID));
}

const FTransform* AMikanClient::GetSavedStageTransform(int32 StageId) const
{
	return StageTransforms.Find(StageId);
}

void AMikanClient::SetSavedStageTransform(int32 StageId, const FTransform& Transform)
{
	StageTransforms.FindOrAdd(StageId) = Transform;
}

// Engine subsystem relay handlers

void AMikanClient::HandleMikanConnected()
{
	SyncAllSpawnedActors();

	if (OnMikanConnected.IsBound())
		OnMikanConnected.Broadcast();
}

void AMikanClient::HandleMikanDisconnected()
{
	SyncAllSpawnedActors();

	if (OnMikanDisconnected.IsBound())
		OnMikanDisconnected.Broadcast();
}

void AMikanClient::HandleDMXDataChanged()
{
	if (OnDMXDataChanged.IsBound())
		OnDMXDataChanged.Broadcast();
}

void AMikanClient::HandleScriptMessage(const FString& Message)
{
	if (OnScriptMessage.IsBound())
		OnScriptMessage.Broadcast(Message);
}

void AMikanClient::HandleCameraNewFrameRaw(const MikanCameraNewFrameEvent& NewFrameEvent)
{
	if (!OnNewFrameEvent.IsBound())
		return;

	// Update the capture actor transform
	const float MetersToUU = GetWorld()->GetWorldSettings()->WorldToMeters;

	const FVector CameraPosition = FMikanMath::MikanVector3fToFVector(NewFrameEvent.camera_position) * MetersToUU;
	const FVector CameraForward = FMikanMath::MikanVector3fToFVector(NewFrameEvent.camera_forward);
	const FVector CameraUp = FMikanMath::MikanVector3fToFVector(NewFrameEvent.camera_up);
	const FQuat CameraQuat = FRotationMatrix::MakeFromXZ(CameraForward, CameraUp).ToQuat();
	const FTransform CameraTransformInScene(CameraQuat, CameraPosition);

	// Update the capture components projection matrices
	float NearClippingPlaneUU = NewFrameEvent.z_bounds.x * MetersToUU;
	float FarClippingPlaneUU = NewFrameEvent.z_bounds.y * MetersToUU;
	const FMatrix ProjectionMatrix =
		FMikanMath::ComputeProjectionMatFromCameraIntrinsics(
			NewFrameEvent.pixel_size.x,
			NewFrameEvent.pixel_size.y,
			NewFrameEvent.focal_length.x,
			NewFrameEvent.focal_length.y,
			NewFrameEvent.principal_point.x,
			NewFrameEvent.principal_point.y,
			NearClippingPlaneUU,
			FarClippingPlaneUU);

	// Extract the Horizontal FOV from the given ProjectionMatrix
	const float ExtractedHalfFOVX = FMath::Atan(1.f / ProjectionMatrix.M[0][0]);
	const float HorizontalFOVRadians = 2.f * ExtractedHalfFOVX;
	const float HorizontalFOVDegrees = FMath::RadiansToDegrees(HorizontalFOVRadians);

	FMikanCameraNewFrameEvent UnrealNewFrameEvent = {};
	UnrealNewFrameEvent.CameraID = NewFrameEvent.camera_id;
	UnrealNewFrameEvent.CameraTransform = CameraTransformInScene;
	UnrealNewFrameEvent.ProjectionMatrix= ProjectionMatrix;
	UnrealNewFrameEvent.NearClippingPlaneUU= NearClippingPlaneUU;
	UnrealNewFrameEvent.FarClippingPlaneUU= FarClippingPlaneUU;
	UnrealNewFrameEvent.HorizontalFOVDegrees= HorizontalFOVDegrees;
	UnrealNewFrameEvent.FrameWidth= NewFrameEvent.pixel_size.x;
	UnrealNewFrameEvent.FrameHeight= NewFrameEvent.pixel_size.y;
	UnrealNewFrameEvent.FrameIndex= NewFrameEvent.frame;
	
	OnNewFrameEvent.Broadcast(UnrealNewFrameEvent);
}
