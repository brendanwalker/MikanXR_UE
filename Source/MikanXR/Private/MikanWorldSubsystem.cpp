#include "MikanWorldSubsystem.h"
#include "MikanAnchorComponent.h"
#include "MikanCaptureComponent.h"
#include "MikanClient_CAPI.h"
#include "MikanMath.h"
#include "MikanCamera.h"
#include "IMikanXRModule.h"

// -- UMikanWorldSubsystem -----
UMikanWorldSubsystem::UMikanWorldSubsystem()
	: UWorldSubsystem()
{
}

UMikanWorldSubsystem* UMikanWorldSubsystem::GetInstance(class UWorld* CurrentWorld)
{
	return CurrentWorld->GetSubsystem<UMikanWorldSubsystem>();
}

void UMikanWorldSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	if (IMikanXRModule::IsAvailable())
	{
		IMikanXRModule::Get().GetClientInfo(&ClientInfo);
		IMikanXRModule::Get().ConnectSubsystem(this);
	}
}

bool UMikanWorldSubsystem::DoesSupportWorldType(EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

void UMikanWorldSubsystem::Deinitialize()
{
	FlushRenderingCommands();

	if (IMikanXRModule::IsAvailable())
	{
		IMikanXRModule::Get().DisconnectSubsystem(this);
	}

	Super::Deinitialize();
}

void UMikanWorldSubsystem::RegisterAnchor(UMikanAnchorComponent* InAnchor)
{
	Anchors.AddUnique(InAnchor);
	UpdateCameraAttachment();
}

void UMikanWorldSubsystem::UnregisterAnchor(UMikanAnchorComponent* InAnchor)
{
	Anchors.Remove(InAnchor);
	UpdateCameraAttachment();
}

void UMikanWorldSubsystem::BindMikanCamera(class AMikanCamera* InCamera)
{
	if (InCamera != nullptr && MikanCamera != InCamera)
	{
		MikanCamera = InCamera;
		ReallocateRenderBuffers();
		UpdateCameraProperties();
	}
}

void UMikanWorldSubsystem::UnbindCamera(class AMikanCamera* InCamera)
{
	if (InCamera == MikanCamera)
	{
		MikanCamera= nullptr;
	}
}

void UMikanWorldSubsystem::HandleMikanConnected()
{
	ReallocateRenderBuffers();
	UpdateCameraProperties();
	HandleVideoSourceAttachmentChanged();

	OnMikanConnected.Broadcast();
}

void UMikanWorldSubsystem::HandleMikanDisconnected()
{
	OnMikanDisconnected.Broadcast();
}

void UMikanWorldSubsystem::HandleMikanEvent(MikanEvent* event)
{
	switch (event->event_type)
	{
		// App Connection Events
		case MikanEvent_connected:
			HandleMikanConnected();
			break;
		case MikanEvent_disconnected:
			HandleMikanDisconnected();
			break;

		// Video Source Events
		case MikanEvent_videoSourceOpened:
			ReallocateRenderBuffers();
			UpdateCameraProperties();
			break;
		case MikanEvent_videoSourceClosed:
			break;
		case MikanEvent_videoSourceNewFrame:
			ProcessNewVideoSourceFrame(event->event_payload.video_source_new_frame);
			break;
		case MikanEvent_videoSourceAttachmentChanged:
			HandleVideoSourceAttachmentChanged();
			break;
		case MikanEvent_videoSourceModeChanged:
		case MikanEvent_videoSourceIntrinsicsChanged:
			ReallocateRenderBuffers();
			UpdateCameraProperties();
			break;

		// VR Device Events
		case MikanEvent_vrDevicePoseUpdated:
			break;
		case MikanEvent_vrDeviceListUpdated:
			break;

		// Spatial Anchor Events
		case MikanEvent_anchorPoseUpdated:
			UpdateAnchorPose(event->event_payload.anchor_pose_updated);
			break;
		case MikanEvent_anchorListUpdated:
			break;
	}
}

void UMikanWorldSubsystem::UpdateAnchorPose(const MikanAnchorPoseUpdateEvent& AnchorPoseEvent)
{
	const float MetersToUU = GetWorld()->GetWorldSettings()->WorldToMeters;

	for (UMikanAnchorComponent* Anchor : Anchors)
	{
		if (Anchor->GetAnchorId() == AnchorPoseEvent.anchor_id)
		{
			FTransform RelativeTransform=
				FMikanMath::MikanTransformToFTransform(AnchorPoseEvent.transform, MetersToUU);

			Anchor->SetRelativeTransform(RelativeTransform);
		}
	}
}

void UMikanWorldSubsystem::ProcessNewVideoSourceFrame(const MikanVideoSourceNewFrameEvent& newFrameEvent)
{
	if (MikanCamera)
	{
		// Apply the camera pose received
		const float MetersToUU = GetWorld()->GetWorldSettings()->WorldToMeters;
		FVector cameraForward = FMikanMath::MikanVector3fToFVector(newFrameEvent.cameraForward);
		FVector cameraUp = FMikanMath::MikanVector3fToFVector(newFrameEvent.cameraUp);
		FQuat cameraQuat = FRotationMatrix::MakeFromXZ(cameraForward, cameraUp).ToQuat();
		FVector cameraPosition = FMikanMath::MikanVector3fToFVector(newFrameEvent.cameraPosition) * MikanCameraScale * MetersToUU;
		FTransform cameraTransform(cameraQuat, cameraPosition);

		MikanCamera->SetActorRelativeTransform(cameraTransform);

		// Render out a new frame
		MikanCamera->MikanCaptureComponent->CaptureFrame(newFrameEvent.frame);

		// Remember the frame index of the last frame we published
		LastReceivedVideoSourceFrame = newFrameEvent.frame;
	}

}

void UMikanWorldSubsystem::FreeFrameBuffer()
{
	if (MikanCamera != nullptr)
	{
		MikanCamera->DisposeRenderTarget();
	}

	Mikan_FreeRenderTargetBuffers();
	FMemory::Memset(&RenderTargetMemory, 0, sizeof(MikanRenderTargetMemory));
}

void UMikanWorldSubsystem::ReallocateRenderBuffers()
{
	FreeFrameBuffer();

	MikanVideoSourceMode VideoMode= {};
	if (Mikan_GetVideoSourceMode(&VideoMode) == MikanResult_Success)
	{
		MikanRenderTargetDescriptor RTDdesc = {};
		RTDdesc.width = (uint32_t)VideoMode.resolution_x;
		RTDdesc.height = (uint32_t)VideoMode.resolution_y;
		RTDdesc.color_key = {0, 0, 0};
		RTDdesc.color_buffer_type = MikanColorBuffer_BGRA32;
		RTDdesc.depth_buffer_type = MikanDepthBuffer_NODEPTH;
		RTDdesc.graphicsAPI = ClientInfo.graphicsAPI;

		Mikan_AllocateRenderTargetBuffers(&RTDdesc, &RenderTargetMemory);

		if (MikanCamera != nullptr)
		{
			MikanCamera->RecreateRenderTarget(RTDdesc);
		}
	}
}

void UMikanWorldSubsystem::UpdateCameraProperties()
{
	MikanVideoSourceIntrinsics VideoIntrinsics;
	if (MikanCamera != nullptr &&
		Mikan_GetVideoSourceIntrinsics(&VideoIntrinsics) == MikanResult_Success)
	{
		MikanMonoIntrinsics MonoIntrinsics = VideoIntrinsics.intrinsics.mono;

		MikanCamera->SetCameraFOV((float)MonoIntrinsics.vfov);
	}
}

void UMikanWorldSubsystem::HandleVideoSourceAttachmentChanged()
{
	MikanVideoSourceAttachmentInfo AttachInfo;
	if (Mikan_GetVideoSourceAttachment(&AttachInfo) == MikanResult_Success)
	{
		CameraParentAnchorId= AttachInfo.parent_anchor_id;
		MikanCameraScale= AttachInfo.camera_scale;
		UpdateCameraAttachment();
	}
}

void UMikanWorldSubsystem::UpdateCameraAttachment()
{
	if (MikanCamera != nullptr)
	{
		AActor* CameraParentAnchor= MikanCamera->GetAttachParentActor();

		for (UMikanAnchorComponent* Anchor : Anchors)
		{
			if (Anchor->GetAnchorId() == CameraParentAnchorId)
			{
				AActor* NewCameraParentActor = Anchor->GetOwner();

				if (NewCameraParentActor != CameraParentAnchor)
				{
					MikanCamera->AttachToActor(
						NewCameraParentActor,
						FAttachmentTransformRules(EAttachmentRule::SnapToTarget, false));
					break;
				}
			}
		}
	}
}