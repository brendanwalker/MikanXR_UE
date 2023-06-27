#include "MikanScene.h"
#include "MikanAnchorComponent.h"
#include "MikanCamera.h"
#include "MikanCaptureComponent.h"
#include "MikanMath.h"
#include "MikanWorldSubsystem.h"
#include "MikanClient_CAPI.h"

PRAGMA_DISABLE_OPTIMIZATION

AMikanScene::AMikanScene(const FObjectInitializer& ObjectInitializer)
{
	SceneOrigin = CreateDefaultSubobject<USceneComponent>(TEXT("SceneOrigin"));
	RootComponent = SceneOrigin;

	MikanToSceneTransform= FTransform();
}

void AMikanScene::BeginPlay()
{
	Super::BeginPlay();

	MikanToSceneTransform= FTransform();

	auto* MikanWorldSubsystem = UMikanWorldSubsystem::GetInstance(GetWorld());
	if (MikanWorldSubsystem)
	{
		MikanWorldSubsystem->BindMikanScene(this);
	}

	// Find the first attached mikan scene
	BindSceneCamera();

	// Gather all of the scene anchor components attached to the scene
	RebuildSceneAnchorList();
}

void AMikanScene::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	auto* MikanWorldSubsystem = UMikanWorldSubsystem::GetInstance(GetWorld());
	if (MikanWorldSubsystem)
	{
		MikanWorldSubsystem->UnbindMikanScene(this);
	}
}

FMikanAnchorInfo* AMikanScene::GetMikanAnchorInfoById(MikanSpatialAnchorID AnchorId)
{
	return MikanAnchorMap.Find(AnchorId);
}

FMikanAnchorInfo* AMikanScene::GetMikanAnchorInfoByName(const FString& AnchorName)
{
	for (auto It = MikanAnchorMap.CreateIterator(); It; ++It)
	{
		FMikanAnchorInfo& AnchorInfo= It.Value();

		if (AnchorInfo.AnchorName == AnchorName)
		{
			return &AnchorInfo;
		}
	}

	return nullptr;
}

void AMikanScene::HandleMikanConnected()
{
	HandleCameraIntrinsicsChanged();
	HandleAnchorListChanged();
}

void AMikanScene::HandleMikanDisconnected()
{

}

void AMikanScene::HandleAnchorListChanged()
{
	UWorld* World = GetWorld();

	MikanAnchorMap.Reset();

	// Rebuild the list of anchors
	MikanSpatialAnchorList SpatialAnchorList;
	if (Mikan_GetSpatialAnchorList(&SpatialAnchorList) == MikanResult_Success)
	{
		for (int Index = 0; Index < SpatialAnchorList.spatial_anchor_count; ++Index)
		{
			const MikanSpatialAnchorID AnchorId= SpatialAnchorList.spatial_anchor_id_list[Index];

			MikanSpatialAnchorInfo MikanAnchorInfo;
			if (Mikan_GetSpatialAnchorInfo(AnchorId, &MikanAnchorInfo) == MikanResult_Success)
			{
				FMikanAnchorInfo SceneAnchorInfo = {};

				// Copy over the anchor id
				check(AnchorId == MikanAnchorInfo.anchor_id);
				SceneAnchorInfo.AnchorID= AnchorId;

				// Copy Anchor Name (convert to UTF-16 from ansi string)
				SceneAnchorInfo.AnchorName = ANSI_TO_TCHAR(MikanAnchorInfo.anchor_name);

				// Get the transform of the anchor in Mikan Space
				const float MetersToUU = World->GetWorldSettings()->WorldToMeters;
				SceneAnchorInfo.MikanSpaceTransform =
					FMikanMath::MikanTransformToFTransform(MikanAnchorInfo.world_transform, MetersToUU);

				// Add anchor to the anchor table
				MikanAnchorMap.Emplace(AnchorId, SceneAnchorInfo);
			}
		}
	}

	// We can now recompute the mikan->scene transform now that the anchors are up to date
	RecomputeMikanToSceneTransform();

	// Finally, update scene transform on all registered scene anchors
	for (UMikanAnchorComponent* SceneAnchor : SceneAnchors)
	{
		SceneAnchor->FetchAnchorInfo();
	}
}

void AMikanScene::HandleAnchorPoseChanged(const MikanAnchorPoseUpdateEvent& AnchorPoseEvent)
{
	FMikanAnchorInfo* SceneAnchorInfo= GetMikanAnchorInfoById(AnchorPoseEvent.anchor_id);

	if (SceneAnchorInfo != nullptr)
	{
		// Update the scene anchor transform from the event
		const float MetersToUU = GetWorld()->GetWorldSettings()->WorldToMeters;
		SceneAnchorInfo->MikanSpaceTransform =
			FMikanMath::MikanTransformToFTransform(AnchorPoseEvent.transform, MetersToUU);

		// Update all transforms associated anchor components ()
		for (UMikanAnchorComponent* Anchor : SceneAnchors)
		{
			Anchor->UpdateSceneTransform();
		}
	}
}

void AMikanScene::HandleCameraIntrinsicsChanged()
{
	AMikanCamera* MikanCamera = GetMikanCamera();

	if (MikanCamera != nullptr)
	{
		MikanCamera->HandleCameraIntrinsicsChanged();
	}
}

void AMikanScene::HandleCameraAttachmentChanged()
{
	MikanVideoSourceAttachmentInfo AttachInfo;
	if (Mikan_GetVideoSourceAttachment(&AttachInfo) == MikanResult_Success)
	{
		MikanSceneScale = AttachInfo.camera_scale;
		RecomputeMikanToSceneTransform();
	}
}

void AMikanScene::HandleNewVideoSourceFrame(const MikanVideoSourceNewFrameEvent& newFrameEvent)
{
	AMikanCamera* MikanCamera= GetMikanCamera();

	if (MikanCamera != nullptr)
	{
		UWorld* World = GetWorld();

		MikanVector3f MikanCameraForward= newFrameEvent.cameraForward;
		MikanVector3f MikanCameraUp= newFrameEvent.cameraUp;
		MikanVector3f MikanCameraPosition= newFrameEvent.cameraPosition;

		static bool bDebug= false;
		static float offset= 0.5f;
		if (bDebug)
		{
			MikanCameraForward= {0.f, 0.f, 1.f};
			MikanCameraUp= {0.f, 1.f, 0.f};
			MikanCameraPosition= {0.f, 0.f, -offset};
		}

		// Compute the camera transform in Mikan Space (but in Unreal coordinate system and units)
		const float MetersToUU = World->GetWorldSettings()->WorldToMeters;
		const FVector UECameraPosition = FMikanMath::MikanVector3fToFVector(MikanCameraPosition) * MetersToUU;
		const FVector UECameraForward = FMikanMath::MikanVector3fToFVector(MikanCameraForward);
		const FVector UECameraUp = FMikanMath::MikanVector3fToFVector(MikanCameraUp);
		const FQuat UECameraQuat = FRotationMatrix::MakeFromXZ(UECameraForward, UECameraUp).ToQuat();
		const FTransform UECameraTransform(UECameraQuat, UECameraPosition);

		// Compute the Scene space camera transform
		const FTransform SceneCameraTransform = UECameraTransform * MikanToSceneTransform;

		// Update the scene camera transform
		MikanCamera->SetActorRelativeTransform(SceneCameraTransform);

		// Render out a new frame at the update camera transform
		MikanCamera->MikanCaptureComponent->CaptureFrame(newFrameEvent.frame);
	}
}

void AMikanScene::RecomputeMikanToSceneTransform()
{
	const FVector CameraScale3D = FVector::OneVector * MikanSceneScale;
	const FTransform CameraScaleTransform = FTransform(FQuat::Identity, FVector::ZeroVector, CameraScale3D);

	// Get the scene origin anchor, if any given
	const FMikanAnchorInfo* OriginAnchorInfo = GetMikanAnchorInfoByName(SceneOriginAnchorName);

	if (OriginAnchorInfo != nullptr)
	{
		// Undo the origin anchor transform, then apply camera scale
		MikanToSceneTransform = OriginAnchorInfo->MikanSpaceTransform.Inverse() * CameraScaleTransform;
	}
	else
	{
		// Just apply the camera scale
		MikanToSceneTransform = CameraScaleTransform;
	}
}

void AMikanScene::RebuildSceneAnchorList()
{
	SceneAnchors.Reset();

	for (USceneComponent* AttachedChild : SceneOrigin->GetAttachChildren())
	{
		UMikanAnchorComponent* AttachedAnchor= Cast<UMikanAnchorComponent>(AttachedChild);

		if (AttachedAnchor != nullptr)
		{
			SceneAnchors.Add(AttachedAnchor);
		}
	}
}

void AMikanScene::BindSceneCamera()
{
	SceneCamera= nullptr;

	for (USceneComponent* AttachedChild : SceneOrigin->GetAttachChildren())
	{
		UMikanCaptureComponent* AttachedCamera = Cast<UMikanCaptureComponent>(AttachedChild);

		if (AttachedCamera != nullptr)
		{
			SceneCamera= Cast<AMikanCamera>(AttachedCamera->GetOwner());
		}
	}
}

PRAGMA_ENABLE_OPTIMIZATION
