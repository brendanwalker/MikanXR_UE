#include "MikanWorldSubsystem.h"
#include "MikanAnchorComponent.h"
#include "MikanCaptureComponent.h"
#include "MikanClient_CAPI.h"
#include "MikanMath.h"
#include "MikanCamera.h"
#include "MikanScene.h"
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

void UMikanWorldSubsystem::BindMikanScene(class AMikanScene* InScene)
{
	MikanScene= InScene;
}

void UMikanWorldSubsystem::UnbindMikanScene(class AMikanScene* InScene)
{
	if (MikanScene == InScene)
	{
		MikanScene= InScene;
	}
}

void UMikanWorldSubsystem::HandleMikanConnected()
{
	ReallocateRenderBuffers();

	if (MikanScene != nullptr)
	{
		MikanScene->HandleMikanConnected();
	}

	OnMikanConnected.Broadcast();
}

void UMikanWorldSubsystem::HandleMikanDisconnected()
{
	FreeRenderBuffers();

	if (MikanScene != nullptr)
	{
		MikanScene->HandleMikanDisconnected();
	}

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
			HandleCameraIntrinsicsChanged();
			break;
		case MikanEvent_videoSourceClosed:
			break;
		case MikanEvent_videoSourceNewFrame:
			HandleNewVideoSourceFrame(event->event_payload.video_source_new_frame);
			break;
		case MikanEvent_videoSourceAttachmentChanged:
			HandleCameraAttachmentChanged();
			break;
		case MikanEvent_videoSourceModeChanged:
		case MikanEvent_videoSourceIntrinsicsChanged:
			ReallocateRenderBuffers();
			HandleCameraIntrinsicsChanged();
			break;

		// VR Device Events
		case MikanEvent_vrDevicePoseUpdated:
			break;
		case MikanEvent_vrDeviceListUpdated:
			break;

		// Spatial Anchor Events
		case MikanEvent_anchorPoseUpdated:
			HandleAnchorPoseChanged(event->event_payload.anchor_pose_updated);
			break;
		case MikanEvent_anchorListUpdated:
			HandleAnchorListChanged();
			break;
	}
}

void UMikanWorldSubsystem::FreeRenderBuffers()
{
	if (MikanScene != nullptr)
	{
		AMikanCamera* MikanCamera = MikanScene->GetMikanCamera();
		if (MikanCamera != nullptr)
		{
			MikanCamera->DisposeRenderTarget();
		}
	}

	Mikan_FreeRenderTargetBuffers();
	FMemory::Memset(&RenderTargetMemory, 0, sizeof(MikanRenderTargetMemory));
}

void UMikanWorldSubsystem::ReallocateRenderBuffers()
{
	// Clean up any previously allocated render targets
	FreeRenderBuffers();

	// Fetch the video source information from Mikan
	MikanVideoSourceMode VideoMode = {};
	if (Mikan_GetVideoSourceMode(&VideoMode) == MikanResult_Success)
	{
		MikanRenderTargetDescriptor RTDdesc = {};
		RTDdesc.width = (uint32_t)VideoMode.resolution_x;
		RTDdesc.height = (uint32_t)VideoMode.resolution_y;
		RTDdesc.color_key = {0, 0, 0};
		RTDdesc.color_buffer_type = MikanColorBuffer_BGRA32;
		RTDdesc.depth_buffer_type = MikanDepthBuffer_NODEPTH;
		RTDdesc.graphicsAPI = ClientInfo.graphicsAPI;

		// Allocate any behind the scenes shared memory
		Mikan_AllocateRenderTargetBuffers(&RTDdesc, &RenderTargetMemory);

		// Tell the active scene camera to recreate a matching render target
		if (MikanScene != nullptr)
		{
			AMikanCamera* MikanCamera = MikanScene->GetMikanCamera();

			if (MikanCamera != nullptr)
			{
				MikanCamera->RecreateRenderTarget(RTDdesc);
			}
		}
	}
}

void UMikanWorldSubsystem::HandleAnchorListChanged()
{
	if (MikanScene != nullptr)
	{
		MikanScene->HandleAnchorListChanged();
	}
}

void UMikanWorldSubsystem::HandleAnchorPoseChanged(const MikanAnchorPoseUpdateEvent& AnchorPoseEvent)
{
	if (MikanScene != nullptr)
	{
		MikanScene->HandleAnchorPoseChanged(AnchorPoseEvent);
	}
}

void UMikanWorldSubsystem::HandleNewVideoSourceFrame(const MikanVideoSourceNewFrameEvent& newFrameEvent)
{
	if (MikanScene)
	{
		MikanScene->HandleNewVideoSourceFrame(newFrameEvent);
	}
}

void UMikanWorldSubsystem::HandleCameraIntrinsicsChanged()
{
	if (MikanScene != nullptr)
	{
		MikanScene->HandleCameraIntrinsicsChanged();
	}
}

void UMikanWorldSubsystem::HandleCameraAttachmentChanged()
{
	if (MikanScene != nullptr)
	{
		MikanScene->HandleCameraAttachmentChanged();
	}
}
