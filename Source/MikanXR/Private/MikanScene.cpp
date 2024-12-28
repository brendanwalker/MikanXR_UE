#include "MikanScene.h"
#include "MikanAnchorActor.h"
#include "MikanAnchorComponent.h"
#include "MikanCamera.h"
#include "MikanCaptureComponent.h"
#include "MikanMath.h"
#include "MikanQuadStencilActor.h"
#include "MikanBoxStencilActor.h"
#include "MikanModelStencilActor.h"
#include "MikanVideoSourceTypes.h"
#include "MikanSpatialAnchorTypes.h"
#include "MikanSpatialAnchorRequests.h"
#include "MikanStencilTypes.h"
#include "MikanStencilRequests.h"
#include "MikanStencilActor.h"
#include "MikanWorldSubsystem.h"
#include "MikanAPI.h"

UE_DISABLE_OPTIMIZATION

AMikanScene::AMikanScene(const FObjectInitializer& ObjectInitializer)
{
	SceneOrigin = CreateDefaultSubobject<USceneComponent>(TEXT("SceneOrigin"));
	RootComponent = SceneOrigin;
	CameraClass = AMikanCamera::StaticClass();
}

void AMikanScene::PostRegisterAllComponents()
{
	Super::PostRegisterAllComponents();

	// Listen for stencil changes
	auto* MikanWorldSubsystem = UMikanWorldSubsystem::GetInstance(GetWorld());
	if (MikanWorldSubsystem)
	{
		MikanAPI = MikanWorldSubsystem->GetMikanAPI();

		// Register the scene with the world subsystem (if not already registered)
		if (MikanWorldSubsystem->RegisterMikanScene(this))
		{
			MikanWorldSubsystem->OnMikanConnected.AddDynamic(this, &AMikanScene::HandleMikanConnected);
			MikanWorldSubsystem->OnAnchorListChanged.AddDynamic(this, &AMikanScene::HandleAnchorListChanged);
			MikanWorldSubsystem->OnAnchorNameChanged.AddDynamic(this, &AMikanScene::HandleAnchorNameChanged);
			MikanWorldSubsystem->OnAnchorPoseChanged.AddDynamic(this, &AMikanScene::HandleAnchorPoseChanged);

			// If we are already connected, handle the connection side effects
			if (MikanAPI->getIsConnected())
			{
				HandleMikanConnected();
			}
		}
	}
}

void AMikanScene::Destroyed()
{
	auto* MikanWorldSubsystem = UMikanWorldSubsystem::GetInstance(GetWorld());
	if (MikanWorldSubsystem)
	{
		MikanWorldSubsystem->OnMikanConnected.RemoveDynamic(this, &AMikanScene::HandleMikanConnected);
		MikanWorldSubsystem->OnAnchorListChanged.RemoveDynamic(this, &AMikanScene::HandleAnchorListChanged);
		MikanWorldSubsystem->OnAnchorNameChanged.RemoveDynamic(this, &AMikanScene::HandleAnchorNameChanged);
		MikanWorldSubsystem->OnAnchorPoseChanged.RemoveDynamic(this, &AMikanScene::HandleAnchorPoseChanged);

		MikanWorldSubsystem->UnregisterMikanScene(this);
	}

	Super::Destroyed();
}

void AMikanScene::BeginPlay()
{
	Super::BeginPlay();

	bIsScenePlaying= true;

	// Listen for stencil changes
	auto* MikanWorldSubsystem = UMikanWorldSubsystem::GetInstance(GetWorld());
	if (MikanWorldSubsystem)
	{
		MikanWorldSubsystem->OnQuadStencilListChanged.AddDynamic(this, &AMikanScene::HandleQuadStencilListChanged);
		MikanWorldSubsystem->OnBoxStencilListChanged.AddDynamic(this, &AMikanScene::HandleBoxStencilListChanged);
		MikanWorldSubsystem->OnModelStencilListChanged.AddDynamic(this, &AMikanScene::HandleModelStencilListChanged);
		MikanWorldSubsystem->OnStencilNameChanged.AddDynamic(this, &AMikanScene::HandleStencilNameChanged);
		MikanWorldSubsystem->OnStencilPoseChanged.AddDynamic(this, &AMikanScene::HandleStencilPoseChanged);

		// Create the camera for the scene
		SpawnSceneCamera();

		if (MikanAPI->getIsConnected())
		{
			HandleMikanConnected();
		}
	}
}

void AMikanScene::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	DespawnSceneCamera();
	DespawnAllModelStencils();

	// Stop listening for stencil changes
	auto* MikanWorldSubsystem = UMikanWorldSubsystem::GetInstance(GetWorld());
	if (MikanWorldSubsystem)
	{
		MikanWorldSubsystem->OnQuadStencilListChanged.RemoveDynamic(this, &AMikanScene::HandleQuadStencilListChanged);
		MikanWorldSubsystem->OnBoxStencilListChanged.RemoveDynamic(this, &AMikanScene::HandleBoxStencilListChanged);
		MikanWorldSubsystem->OnModelStencilListChanged.RemoveDynamic(this, &AMikanScene::HandleModelStencilListChanged);
		MikanWorldSubsystem->OnStencilNameChanged.RemoveDynamic(this, &AMikanScene::HandleStencilNameChanged);
		MikanWorldSubsystem->OnStencilPoseChanged.RemoveDynamic(this, &AMikanScene::HandleStencilPoseChanged);
	}

	bIsScenePlaying= false;

	Super::EndPlay(EndPlayReason);
}

// Scene Events
void AMikanScene::HandleSceneActivated()
{

}

void AMikanScene::HandleSceneDeactivated()
{
}

// App Connection Events
void AMikanScene::HandleMikanConnected()
{
	HandleAnchorListChanged();

	if (bIsScenePlaying)
	{
		HandleQuadStencilListChanged();
		HandleBoxStencilListChanged();
		HandleModelStencilListChanged();
	}
}

// Spatial Anchor Events
void AMikanScene::HandleAnchorListChanged()
{
	UWorld* World = GetWorld();
	const float MetersToUU = World->GetWorldSettings()->WorldToMeters;

	// Get a list of all anchors in the scene
	TArray<AMikanAnchorActor*> AnchorActors;
	GatherSceneAnchorList(AnchorActors);

	// Fetch the list of spatial anchors from Mikan and apply them to the scene
	GetSpatialAnchorList listRequest;
	auto listResponse= MikanAPI->sendRequest(listRequest).fetchResponse();
	if (listResponse->resultCode == MikanAPIResult::Success)
	{
		auto SpatialAnchorList= std::static_pointer_cast<MikanSpatialAnchorListResponse>(listResponse);

		for (size_t Index = 0; Index < SpatialAnchorList->spatial_anchor_id_list.size(); ++Index)
		{
			const MikanSpatialAnchorID AnchorId= SpatialAnchorList->spatial_anchor_id_list[Index];

			GetSpatialAnchorInfo anchorRequest;
			anchorRequest.anchorId= AnchorId;

			auto anchorResponse= MikanAPI->sendRequest(anchorRequest).fetchResponse();
			if (anchorResponse->resultCode == MikanAPIResult::Success)
			{
				auto MikanAnchorResponse= std::static_pointer_cast<MikanSpatialAnchorInfoResponse>(anchorResponse);

				ApplyAnchorInfo(MikanAnchorResponse->anchor_info);
			}
		}
	}
}

void AMikanScene::HandleAnchorNameChanged(
	int32 AnchorID, 
	const FString& AnchorName)
{
	GetSpatialAnchorInfo anchorRequest;
	anchorRequest.anchorId= AnchorID;

	auto anchorResponse = MikanAPI->sendRequest(anchorRequest).fetchResponse();
	if (anchorResponse->resultCode == MikanAPIResult::Success)
	{
		auto MikanAnchorResponse = std::static_pointer_cast<MikanSpatialAnchorInfoResponse>(anchorResponse);

		// Unbind any other anchors with the same ID but different names
		TArray<AMikanAnchorActor*> AnchorActors;
		GatherSceneAnchorList(AnchorActors, AnchorID);
		for (AMikanAnchorActor* AnchorActor : AnchorActors)
		{
			if (AnchorActor->GetAnchorName() != AnchorName)
			{
				AnchorActor->UnbindAnchorId();
			}
		}

		// Apply anchor info to any actor with matching name
		ApplyAnchorInfo(MikanAnchorResponse->anchor_info);
	}
}

void AMikanScene::HandleAnchorPoseChanged(
	int32 AnchorID, 
	const FTransform& SceneTransform)
{
	TArray<AMikanAnchorActor*> AnchorActors;
	GatherSceneAnchorList(AnchorActors, AnchorID);
	for (AMikanAnchorActor* AnchorActor : AnchorActors)
	{
		AnchorActor->SetActorRelativeTransform(SceneTransform);
	}
}

void AMikanScene::GatherSceneAnchorList(
	TArray<AMikanAnchorActor *>& OutAnchorActors,
	int32 AnchorIdFilter)
{
	OutAnchorActors.Reset();

	TArray<USceneComponent*> AllSceneChildComponents;
	SceneOrigin->GetChildrenComponents(true, AllSceneChildComponents);
	for (USceneComponent* AttachedChild : AllSceneChildComponents)
	{
		UMikanAnchorComponent* AnchorComponent = Cast<UMikanAnchorComponent>(AttachedChild);

		if (AnchorComponent != nullptr)
		{
			auto* AnchorActor = CastChecked<AMikanAnchorActor>(AnchorComponent->GetOwner());

			if (AnchorIdFilter == -1 || AnchorActor->GetAnchorId() == AnchorIdFilter)
			{
				OutAnchorActors.Add(AnchorActor);
			}
		}
	}
}

void AMikanScene::ApplyAnchorInfo(const struct MikanSpatialAnchorInfo& AnchorInfo)
{
	TArray<AMikanAnchorActor*> AnchorActors;
	GatherSceneAnchorList(AnchorActors, -1);

	auto AnchorNameTChar = StringCast<TCHAR>(AnchorInfo.anchor_name.getValue().c_str());
	FString AnchorName = FString(AnchorNameTChar.Get());

	for (AMikanAnchorActor* AnchorActor : AnchorActors)
	{
		if (AnchorActor->GetAnchorName() == AnchorName)
		{
			AnchorActor->ApplyAnchorInfo(AnchorInfo);
		}
	}
}

// Stencil Events
void AMikanScene::HandleQuadStencilListChanged()
{
	const float MetersToUU = GetWorld()->GetWorldSettings()->WorldToMeters;

	GetQuadStencilList quadStencilListRequest;
	auto listResponse = MikanAPI->sendRequest(quadStencilListRequest).fetchResponse();
	if (listResponse->resultCode == MikanAPIResult::Success)
	{
		auto QuadStencilList = std::static_pointer_cast<MikanStencilListResponse>(listResponse);
		auto& NewStencilIDList = QuadStencilList->stencil_id_list;

		// See if any existing stencils no longer exist in Mikan and need to be removed
		TArray<MikanStencilID> ExistingIDs;
		QuadStencils.GetKeys(ExistingIDs);
		for (MikanStencilID ExistingID : ExistingIDs)
		{
			auto it = std::find(NewStencilIDList.begin(), NewStencilIDList.end(), ExistingID);
			if (it == NewStencilIDList.end())
			{
				// Destroy the existing stencil actor
				AMikanStencilActor* ExistingStencil = QuadStencils[ExistingID];
				if (ExistingStencil)
				{
					ExistingStencil->Destroy();
				}

				// Remove the stencil entry from the table
				QuadStencils.Remove(ExistingID);
			}
		}
		QuadStencils.Compact();

		// Update any existing Model Stencils or spawn new ones
		for (MikanStencilID StencilID : NewStencilIDList)
		{
			// Update or create quad stencil actor
			GetQuadStencil quadStencilRequest;
			quadStencilRequest.stencilId= StencilID;

			auto Response = MikanAPI->sendRequest(quadStencilRequest).fetchResponse();
			if (Response->resultCode == MikanAPIResult::Success)
			{
				auto StencilResponse = std::static_pointer_cast<MikanStencilQuadInfoResponse>(Response);

				AMikanQuadStencilActor** ExistingStencil = QuadStencils.Find(StencilID);
				if (ExistingStencil != nullptr)
				{
					// Update the existing stencil
					(*ExistingStencil)->ApplyQuadStencilInfo(StencilResponse->quad_info);
				}
				else
				{
					// Spawn the stencil actor
					auto* NewStencil = AMikanQuadStencilActor::SpawnStencil(this, StencilResponse->quad_info);
					if (NewStencil != nullptr)
					{
						QuadStencils.Emplace(StencilID, NewStencil);
					}
				}
			}
		}
	}
}

void AMikanScene::HandleBoxStencilListChanged()
{
	GetBoxStencilList listRequest;
	auto listResponse = MikanAPI->sendRequest(listRequest).fetchResponse();
	if (listResponse->resultCode == MikanAPIResult::Success)
	{
		auto BoxStencilList = std::static_pointer_cast<MikanStencilListResponse>(listResponse);
		auto& NewStencilIDList = BoxStencilList->stencil_id_list;

		// See if any existing stencils no longer exist in Mikan and need to be removed
		TArray<MikanStencilID> ExistingIDs;
		BoxStencils.GetKeys(ExistingIDs);
		for (MikanStencilID ExistingID : ExistingIDs)
		{
			auto it = std::find(NewStencilIDList.begin(), NewStencilIDList.end(), ExistingID);
			if (it == NewStencilIDList.end())
			{
				// Destroy the existing stencil actor
				AMikanStencilActor* ExistingStencil = BoxStencils[ExistingID];
				if (ExistingStencil)
				{
					ExistingStencil->Destroy();
				}

				// Remove the stencil entry from the table
				BoxStencils.Remove(ExistingID);
			}
		}
		BoxStencils.Compact();

		// Update any existing Model Stencils or spawn new ones
		for (MikanStencilID StencilID : NewStencilIDList)
		{
			// Update or create box stencil object
			GetBoxStencil StencilRequest;
			StencilRequest.stencilId = StencilID;

			auto Response = MikanAPI->sendRequest(StencilRequest).fetchResponse();
			if (Response->resultCode == MikanAPIResult::Success)
			{
				auto StencilResponse = std::static_pointer_cast<MikanStencilBoxInfoResponse>(Response);

				AMikanBoxStencilActor** ExistingStencil = BoxStencils.Find(StencilID);
				if (ExistingStencil != nullptr)
				{
					// Update the existing stencil
					(*ExistingStencil)->ApplyBoxStencilInfo(StencilResponse->box_info);
				}
				else
				{
					// Spawn the stencil actor
					auto* NewStencil = AMikanBoxStencilActor::SpawnStencil(this, StencilResponse->box_info);

					if (NewStencil != nullptr)
					{
						// Keep track of the stencil actor
						BoxStencils.Emplace(StencilID, NewStencil);
					}
				}
			}
		}
	}
}

void AMikanScene::HandleModelStencilListChanged()
{
	GetModelStencilList listRequest;
	auto listResponse= MikanAPI->sendRequest(listRequest).fetchResponse();
	if (listResponse->resultCode == MikanAPIResult::Success)
	{
		auto ModelStencilList= std::static_pointer_cast<MikanStencilListResponse>(listResponse);
		auto& NewStencilIDList= ModelStencilList->stencil_id_list;

		// See if any existing stencils no longer exist in Mikan and need to be removed
		TArray<MikanStencilID> ExistingIDs;
		ModelStencils.GetKeys(ExistingIDs);
		for (MikanStencilID ExistingID : ExistingIDs)
		{
			auto it= std::find(NewStencilIDList.begin(), NewStencilIDList.end(), ExistingID);
			if (it == NewStencilIDList.end())
			{
				// Destroy the existing stenil actor
				AMikanStencilActor* ExistingStencil= ModelStencils[ExistingID];
				if (ExistingStencil)
				{
					ExistingStencil->Destroy();
				}

				// Remove the stencil entry from the table
				ModelStencils.Remove(ExistingID);
			}
		}
		ModelStencils.Compact();

		// Update any existing Model Stencils or spawn new ones
		for (MikanStencilID StencilID : NewStencilIDList)
		{
			// Update or create core stencil object
			GetModelStencil StencilRequest;
			StencilRequest.stencilId= StencilID;

			auto Response= MikanAPI->sendRequest(StencilRequest).fetchResponse();
			if (Response->resultCode == MikanAPIResult::Success)
			{
				auto StencilResponse= std::static_pointer_cast<MikanStencilModelInfoResponse>(Response);

				AMikanModelStencilActor** ExistingStencil= ModelStencils.Find(StencilID);
				if (ExistingStencil != nullptr)
				{
					// Update the existing stencil
					(*ExistingStencil)->ApplyModelStencilInfo(StencilResponse->model_info);
				}
				else
				{
					// Spawn the stencil actor
					auto* NewStencil = AMikanModelStencilActor::SpawnStencil(this, StencilResponse->model_info);

					if (NewStencil != nullptr)
					{
						// Keep track of the stencil actor
						ModelStencils.Emplace(StencilID, NewStencil);
					}
				}
			}
		}
	}
}

void AMikanScene::HandleStencilNameChanged(int32 StencilID, const FString& StencilName)
{
	AMikanStencilActor* Stencil = GetMikanStencilById(StencilID);
	if (Stencil != nullptr)
	{
		Stencil->ApplyStencilName(StencilName);
	}
}

void AMikanScene::HandleStencilPoseChanged(int32 StencilID, const FTransform& SceneTransform)
{
	AMikanStencilActor* Stencil = GetMikanStencilById(StencilID);
	if (Stencil != nullptr)
	{
		Stencil->SetActorRelativeTransform(SceneTransform);
	}
}

AMikanStencilActor* AMikanScene::GetMikanStencilById(MikanStencilID StencilID)
{
	AMikanModelStencilActor** ExistingStencil = ModelStencils.Find(StencilID);
	if (ExistingStencil != nullptr)
	{
		return *ExistingStencil;
	}

	return nullptr;
}

// Script Message Events
void AMikanScene::HandleScriptMessage(const FString& Message)
{
	OnMikanScriptMessage(Message);
}

// Internal Methods
void AMikanScene::SpawnSceneCamera()
{
	DespawnSceneCamera();

	UWorld* World = GetWorld();
	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Name= TEXT("MikanSceneCamera");
	SpawnParameters.Owner = this;
	SceneCamera = World->SpawnActor<AMikanCamera>(CameraClass, SceneOrigin->GetComponentTransform(), SpawnParameters);
	SceneCamera->AttachToComponent(SceneOrigin, FAttachmentTransformRules::SnapToTargetIncludingScale);
}

void AMikanScene::DespawnSceneCamera()
{
	if (SceneCamera != nullptr)
	{
		SceneCamera->Destroy();
		SceneCamera = nullptr;
	}
}

void AMikanScene::DespawnAllQuadStencils()
{
	for (auto& Elem : QuadStencils)
	{
		AMikanStencilActor* Stencil = Elem.Value;

		if (Stencil != nullptr)
		{
			Stencil->Destroy();
		}
	}
	QuadStencils.Empty();
}

void AMikanScene::DespawnAllBoxStencils()
{
	for (auto& Elem : BoxStencils)
	{
		AMikanStencilActor* Stencil = Elem.Value;

		if (Stencil != nullptr)
		{
			Stencil->Destroy();
		}
	}
	BoxStencils.Empty();
}

void AMikanScene::DespawnAllModelStencils()
{
	for (auto& Elem : ModelStencils)
	{
		AMikanStencilActor* Stencil = Elem.Value;

		if (Stencil != nullptr)
		{
			Stencil->Destroy();
		}
	}
	ModelStencils.Empty();
}

UE_ENABLE_OPTIMIZATION
