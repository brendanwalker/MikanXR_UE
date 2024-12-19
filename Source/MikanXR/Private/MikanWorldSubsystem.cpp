#include "MikanWorldSubsystem.h"
#include "MikanAnchorComponent.h"
#include "MikanCaptureComponent.h"
#include "MikanAPI.h"
#include "MikanClientEvents.h"
#include "MikanScriptEvents.h"
#include "MikanSpatialAnchorEvents.h"
#include "MikanStencilEvents.h"
#include "MikanVideoSourceEvents.h"
#include "MikanVRDeviceEvents.h" 
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
		IMikanXRModule& MikanXRModule= IMikanXRModule::Get();

		MikanAPI= MikanXRModule.GetMikanAPI();
		MikanXRModule.GetClientInfo(&ClientInfo);
		MikanXRModule.ConnectSubsystem(this);
	}
}

bool UMikanWorldSubsystem::DoesSupportWorldType(EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE || WorldType == EWorldType::Editor;
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

bool UMikanWorldSubsystem::RegisterMikanScene(class AMikanScene* MikanScene)
{
	if (MikanScene != nullptr)
	{
		if (!RegisteredScenes.Contains(MikanScene))
		{
			RegisteredScenes.Add(MikanScene);

			// Activate the scene if it is the first scene registered
			if (ActiveMikanScene == nullptr)
			{
				SetActiveMikanScene(MikanScene);
			}

			return true;
		}
	}

	return false;
}

void UMikanWorldSubsystem::UnregisterMikanScene(class AMikanScene* MikanScene)
{
	if (MikanScene != nullptr)
	{
		// Deactivate the scene if it is the active scene
		if (ActiveMikanScene == MikanScene)
		{
			SetActiveMikanScene(nullptr);
		}

		RegisteredScenes.Remove(MikanScene);
	}
}

void UMikanWorldSubsystem::SetActiveMikanScene(AMikanScene* DesiredScene)
{
	if (DesiredScene != ActiveMikanScene && RegisteredScenes.Contains(DesiredScene))
	{
		AMikanScene* OldScene = ActiveMikanScene;
		AMikanScene* NewScene = DesiredScene;

		ActiveMikanScene = DesiredScene;

		if (OldScene != nullptr)
		{
			OldScene->HandleSceneDeactivated();
		}

		if (NewScene != nullptr)
		{
			NewScene->HandleSceneActivated();
		}

		OnActiveSceneChanged.Broadcast(OldScene, NewScene);
	}
}

bool UMikanWorldSubsystem::RegisterMikanRenderable(class UMikanRenderableComponent* Renderable)
{
	if (Renderable != nullptr && !RegisteredRenderables.Contains(Renderable))
	{
		RegisteredRenderables.Add(Renderable);
		OnRenderableRegistered.Broadcast(Renderable);
		return true;
	}

	return false;
}
void UMikanWorldSubsystem::UnregisterMikanRenderable(class UMikanRenderableComponent* Renderable)
{
	if (Renderable != nullptr && RegisteredRenderables.Contains(Renderable))
	{
		RegisteredRenderables.Remove(Renderable);
		OnRenderableUnregistered.Broadcast(Renderable);
	}
}

void UMikanWorldSubsystem::HandleMikanEvent(MikanEventPtr mikanEvent)
{
	// App Connection Events
	if (typeid(*mikanEvent) == typeid(MikanConnectedEvent))
	{
		HandleMikanConnected();
	}
	else if (typeid(*mikanEvent) == typeid(MikanDisconnectedEvent))
	{
		HandleMikanDisconnected();
	}	
	// Video Source Events
	else if (typeid(*mikanEvent) == typeid(MikanVideoSourceOpenedEvent))
	{
		HandleVideoSourceOpened();
	}
	else if (typeid(*mikanEvent) == typeid(MikanVideoSourceClosedEvent))
	{
		HandleVideoSourceClosed();	
	}
	else if (typeid(*mikanEvent) == typeid(MikanVideoSourceModeChangedEvent))
	{
		HandleVideoSourceModeChanged();
	}
	else if (typeid(*mikanEvent) == typeid(MikanVideoSourceIntrinsicsChangedEvent))
	{
		HandleVideoSourceIntrinsicsChanged();
	}
	else if (typeid(*mikanEvent) == typeid(MikanVideoSourceAttachmentChangedEvent))
	{
		HandleVideoSourceAttachmentChanged();
	}
	else if (typeid(*mikanEvent) == typeid(MikanVideoSourceNewFrameEvent))
	{
		auto newFrameEvent= std::static_pointer_cast<MikanVideoSourceNewFrameEvent>(mikanEvent);

		HandleNewVideoSourceFrame(*newFrameEvent.get());
	}
	else if (typeid(*mikanEvent) == typeid(MikanVideoSourceAttachmentChangedEvent))
	{
		HandleVideoSourceAttachmentChanged();
	}
	// VR Device Events
	else if (typeid(*mikanEvent) == typeid(MikanVRDeviceListUpdateEvent))
	{
		HandleVRDeviceListChanged();
	}
	else if (typeid(*mikanEvent) == typeid(MikanVRDevicePoseUpdateEvent))
	{
		auto devicePoseEvent= std::static_pointer_cast<MikanVRDevicePoseUpdateEvent>(mikanEvent);

		HandleVRDevicePoseChanged(*devicePoseEvent.get());
	}
	// Spatial Anchor Events
	else if (typeid(*mikanEvent) == typeid(MikanAnchorNameUpdateEvent))
	{
		auto anchorNameEvent= std::static_pointer_cast<MikanAnchorNameUpdateEvent>(mikanEvent);

		HandleAnchorNameChanged(*anchorNameEvent.get());
	}
	else if (typeid(*mikanEvent) == typeid(MikanAnchorPoseUpdateEvent))
	{
		auto anchorPoseEvent= std::static_pointer_cast<MikanAnchorPoseUpdateEvent>(mikanEvent);

		HandleAnchorPoseChanged(*anchorPoseEvent.get());
	}
	else if (typeid(*mikanEvent) == typeid(MikanAnchorListUpdateEvent))
	{
		HandleAnchorListChanged();
	}
	// Stencil Events
	else if (typeid(*mikanEvent) == typeid(MikanStencilNameUpdateEvent))
	{
		auto stencilNameEvent= std::static_pointer_cast<MikanStencilNameUpdateEvent>(mikanEvent);

		HandleStencilNameChanged(*stencilNameEvent.get());
	}
	else if (typeid(*mikanEvent) == typeid(MikanStencilPoseUpdateEvent))
	{
		auto stencilPoseEvent= std::static_pointer_cast<MikanStencilPoseUpdateEvent>(mikanEvent);

		HandleStencilPoseChanged(*stencilPoseEvent.get());
	}
	else if (typeid(*mikanEvent) == typeid(MikanQuadStencilListUpdateEvent))
	{
		HandleQuadStencilListChanged();
	}
	else if (typeid(*mikanEvent) == typeid(MikanBoxStencilListUpdateEvent))
	{
		HandleBoxStencilListChanged();
	}
	else if (typeid(*mikanEvent) == typeid(MikanModelStencilListUpdateEvent))
	{
		HandleModelStencilListChanged();
	}
	// Script Message Events
	else if (typeid(*mikanEvent) == typeid(MikanScriptMessagePostedEvent))
	{
		auto scriptMessageEvent= std::static_pointer_cast<MikanScriptMessagePostedEvent>(mikanEvent);

		HandleScriptMessage(*scriptMessageEvent.get());
	}
}

// App Connection Events
void UMikanWorldSubsystem::HandleMikanConnected()
{
	if (OnMikanConnected.IsBound())
	{
		OnMikanConnected.Broadcast();
	}
}

void UMikanWorldSubsystem::HandleMikanDisconnected()
{
	if (OnMikanDisconnected.IsBound())
	{
		OnMikanDisconnected.Broadcast();
	}
}

// Video Source Events
void UMikanWorldSubsystem::HandleVideoSourceOpened()
{
	if (OnVideoSourceOpened.IsBound())
	{
		OnVideoSourceOpened.Broadcast();
	}
}

void UMikanWorldSubsystem::HandleVideoSourceClosed()
{
	if (OnVideoSourceClosed.IsBound())
	{
		OnVideoSourceClosed.Broadcast();
	}
}

void UMikanWorldSubsystem::HandleVideoSourceModeChanged()
{
	if (OnVideoSourceModeChanged.IsBound())
	{
		OnVideoSourceModeChanged.Broadcast();
	}
}

void UMikanWorldSubsystem::HandleVideoSourceIntrinsicsChanged()
{
	if (OnVideoSourceModeChanged.IsBound())
	{
		OnVideoSourceModeChanged.Broadcast();
	}
}

void UMikanWorldSubsystem::HandleNewVideoSourceFrame(const MikanVideoSourceNewFrameEvent& newFrameEvent)
{
	if (OnNewFrameEvent.IsBound())
	{
		const float MetersToUU = GetWorld()->GetWorldSettings()->WorldToMeters;
		FVector CameraPosition = FMikanMath::MikanVector3fToFVector(newFrameEvent.cameraPosition) * MetersToUU;
		FVector CameraForward = FMikanMath::MikanVector3fToFVector(newFrameEvent.cameraForward);
		FVector CameraUp = FMikanMath::MikanVector3fToFVector(newFrameEvent.cameraUp);
		FQuat CameraQuat = FRotationMatrix::MakeFromXZ(CameraForward, CameraUp).ToQuat();
		FTransform CameraTransform(CameraQuat, CameraPosition);

		OnNewFrameEvent.Broadcast(newFrameEvent.frame, CameraTransform);
	}
}

void UMikanWorldSubsystem::HandleVideoSourceAttachmentChanged()
{
	if (OnVideoSourceAttachmentChanged.IsBound())
	{
		OnVideoSourceAttachmentChanged.Broadcast();
	}
}

// VR Device Events
void UMikanWorldSubsystem::HandleVRDeviceListChanged()
{
	if (OnVRDeviceListChanged.IsBound())
	{
		OnVRDeviceListChanged.Broadcast();
	}
}

void UMikanWorldSubsystem::HandleVRDevicePoseChanged(const MikanVRDevicePoseUpdateEvent& DevicePoseEvent)
{
	if (OnVRDevicePoseChanged.IsBound())
	{
		FMatrix MikanMat = FMikanMath::MikanMatrix4fToFMatrix(DevicePoseEvent.transform);
		const float MetersToUU = GetWorld()->GetWorldSettings()->WorldToMeters;
		MikanMat.ApplyScale(MetersToUU);

		OnVRDevicePoseChanged.Broadcast(DevicePoseEvent.device_id, DevicePoseEvent.frame, FTransform(MikanMat));
	}
}

// Spatial Anchor Events
void UMikanWorldSubsystem::HandleAnchorListChanged()
{
	if (OnAnchorListChanged.IsBound())
	{
		OnAnchorListChanged.Broadcast();
	}
}

void UMikanWorldSubsystem::HandleAnchorNameChanged(const struct MikanAnchorNameUpdateEvent& AnchorNameEvent)
{
	if (OnAnchorNameChanged.IsBound())
	{
		auto AnchorNameTChar = StringCast<TCHAR>(AnchorNameEvent.anchor_name.getValue().c_str());

		OnAnchorNameChanged.Broadcast(AnchorNameEvent.anchor_id, AnchorNameTChar.Get());
	}
}

void UMikanWorldSubsystem::HandleAnchorPoseChanged(const MikanAnchorPoseUpdateEvent& AnchorPoseEvent)
{
	if (OnAnchorPoseChanged.IsBound())
	{
		const float MetersToUU = GetWorld()->GetWorldSettings()->WorldToMeters;
		FTransform AnchorTransform = FMikanMath::MikanTransformToFTransform(AnchorPoseEvent.transform, MetersToUU);

		OnAnchorPoseChanged.Broadcast(AnchorPoseEvent.anchor_id, AnchorTransform);
	}
}

// Stencil Events
void UMikanWorldSubsystem::HandleStencilNameChanged(const struct MikanStencilNameUpdateEvent& StencilNameEvent)
{
	if (OnStencilNameChanged.IsBound())
	{
		auto StencilNameTChar = StringCast<TCHAR>(StencilNameEvent.stencil_name.getValue().c_str());

		OnStencilNameChanged.Broadcast(StencilNameEvent.stencil_id, StencilNameTChar.Get());
	}
}

void UMikanWorldSubsystem::HandleQuadStencilListChanged()
{
	if (OnQuadStencilListChanged.IsBound())
	{
		OnQuadStencilListChanged.Broadcast();
	}
}

void UMikanWorldSubsystem::HandleBoxStencilListChanged()
{
	if (OnBoxStencilListChanged.IsBound())
	{
		OnBoxStencilListChanged.Broadcast();
	}
}

void UMikanWorldSubsystem::HandleModelStencilListChanged()
{
	if (OnModelStencilListChanged.IsBound())
	{
		OnModelStencilListChanged.Broadcast();
	}
}

void UMikanWorldSubsystem::HandleStencilPoseChanged(const MikanStencilPoseUpdateEvent& StencilPoseEvent)
{
	if (OnStencilPoseChanged.IsBound())
	{
		const float MetersToUU = GetWorld()->GetWorldSettings()->WorldToMeters;
		FTransform StencilTransform = FMikanMath::MikanTransformToFTransform(StencilPoseEvent.transform, MetersToUU);

		OnStencilPoseChanged.Broadcast(StencilPoseEvent.stencil_id, StencilTransform);
	}
}

// Script Message Events
void UMikanWorldSubsystem::HandleScriptMessage(const MikanScriptMessagePostedEvent& ScriptMessageEvent)
{
	if (OnScriptMessage.IsBound())
	{
		auto TCharMessage = StringCast<TCHAR>(ScriptMessageEvent.message.getValue().c_str());

		OnScriptMessage.Broadcast(TCharMessage.Get());
	}
}