#include "MikanCameraActor.h"
#include "Components/ArrowComponent.h"
#include "IMikanXRModule.h"
#include "Engine/Engine.h"
#include "MikanCaptureComponent.h"
#include "MikanAPI.h"
#include "MikanCameraEvents.h"
#include "MikanCameraRequests.h"
#include "MikanCameraTypes.h"
#include "MikanMath.h"
#include "MikanSceneActor.h"
#include "MikanRenderTargetRequests.h"
#include "MikanVideoSourceRequests.h"
#include "MikanVideoSourceTypes.h"
#include "MikanEngineSubsystem.h"
#include "MikanClient.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "DrawDebugHelpers.h"

UE_DISABLE_OPTIMIZATION

// -- UMikanCameraData -----
void UMikanCameraData::Initialize(const Serialization::PolymorphicObjectPtr& InValuesObject)
{
	UMikanTransformData::Initialize(InValuesObject);

	const auto* CameraValues = InValuesObject.getTypedPointer<MikanCameraComponentValues>();
	StageId = CameraValues->stage_id;
	TrackingMountId = CameraValues->tracking_mount_id;
	VideoSourceId = CameraValues->video_source_id;
	TrackingFrameDelay = CameraValues->tracking_frame_delay;
}

bool UMikanCameraData::ApplyMikanValue(const FString& FieldName, const MikanVariant& FieldValue)
{
	if (FieldName == "stage_id")
	{
		StageId = FieldValue.getIntValue();
		return true;
	}
	else if (FieldName == "tracking_mount_id")
	{
		TrackingMountId = FieldValue.getIntValue();
		return true;
	}
	else if (FieldName == "video_source_id")
	{
		VideoSourceId = FieldValue.getIntValue();
		return true; 
	}
	else if (FieldName == "tracking_frame_delay")
	{
		TrackingFrameDelay = FieldValue.getIntValue();
		return true;
	}
	else
	{
		return UMikanTransformData::ApplyMikanValue(FieldName, FieldValue);
	}
}

void UMikanCameraData::Describe(TArray<FString>& OutLines) const
{
	UMikanTransformData::Describe(OutLines);
	Mikan::AppendDescribeLine(OutLines, TEXT("stage_id"), StageId);
	Mikan::AppendDescribeLine(OutLines, TEXT("tracking_mount_id"), TrackingMountId);
	Mikan::AppendDescribeLine(OutLines, TEXT("video_source_id"), VideoSourceId);
	Mikan::AppendDescribeLine(OutLines, TEXT("tracking_frame_delay"), TrackingFrameDelay);
}

// -- AMikanFrameCaptureActor -----
AMikanFrameCaptureActor::AMikanFrameCaptureActor(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = true;

	CameraRoot = CreateDefaultSubobject<USceneComponent>(TEXT("CameraRoot"));
	RootComponent = CameraRoot;

	ColorCaptureComponent = CreateDefaultSubobject<UMikanCaptureComponent>(TEXT("ColorCaptureComponent"));
	ColorCaptureComponent->SetupAttachment(CameraRoot);
	ColorCaptureComponent->CaptureSource = ESceneCaptureSource::SCS_SceneColorHDR;
	ColorCaptureComponent->SetCaptureKind(EMikanCaptureKind::Color);

	// DepthCaptureComponent is created on demand in BeginPlay when bEnableDepthCapture is set.

	// Shadow catcher pass A (shadowed): white catcher lit with shadow casters present.
	ShadowCaptureComponent = CreateDefaultSubobject<UMikanCaptureComponent>(TEXT("ShadowCaptureComponent"));
	ShadowCaptureComponent->SetupAttachment(CameraRoot);
	ShadowCaptureComponent->CaptureSource = ESceneCaptureSource::SCS_SceneColorHDR;
	ShadowCaptureComponent->SetCaptureKind(EMikanCaptureKind::Shadow);
	// Keep unrendered pixels at the white clear (not opaque black scene background) so empty regions
	// read as "no shadow" once divided. Both shadow passes must agree here so A/B cancels to 1.
	ShadowCaptureComponent->bConsiderUnrenderedOpaquePixelAsFullyTranslucent = true;

	// Shadow catcher pass B (reference): same white catcher with no casters. A/B isolates the
	// per-channel shadow factor (cancels the dynamic colored lighting). Composited multiplicatively
	// in Mikan, so the identity is white (no shadow).
	ShadowReferenceCaptureComponent = CreateDefaultSubobject<UMikanCaptureComponent>(TEXT("ShadowReferenceCaptureComponent"));
	ShadowReferenceCaptureComponent->SetupAttachment(CameraRoot);
	ShadowReferenceCaptureComponent->CaptureSource = ESceneCaptureSource::SCS_SceneColorHDR;
	ShadowReferenceCaptureComponent->SetCaptureKind(EMikanCaptureKind::ShadowReference);
	// Match the shadowed pass so unrendered pixels cancel to 1 in the A/B divide.
	ShadowReferenceCaptureComponent->bConsiderUnrenderedOpaquePixelAsFullyTranslucent = true;

	ClientInfo = {};
	RenderTargetDesc = {};
}

class AMikanCameraActor* AMikanFrameCaptureActor::GetOwnerCameraActor() const
{
	return Cast<AMikanCameraActor>(GetOwner());
}

void AMikanFrameCaptureActor::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	// Set a default depth range
	DepthRangeUU= GetWorld()->GetWorldSettings()->WorldToMeters * 1.f;
}

void AMikanFrameCaptureActor::BeginPlay()
{
	Super::BeginPlay();

	ClientInfo = {};
	RenderTargetDesc = {};

	auto* EngineSubsystem = UMikanEngineSubsystem::Get();
	if (EngineSubsystem)
	{
		MikanAPI = EngineSubsystem->GetMikanAPI();
		ClientInfo = EngineSubsystem->GetClientInfo();
	}

	// Spawn the depth capture only when requested. The actor has already begun play (Super::BeginPlay
	// above), so RegisterComponent triggers the capture component's BeginPlay -> renderable binding.
	if (bEnableDepthCapture && DepthCaptureComponent == nullptr)
	{
		DepthCaptureComponent = NewObject<UMikanCaptureComponent>(this, TEXT("DepthCaptureComponent"));
		DepthCaptureComponent->SetupAttachment(CameraRoot);
		DepthCaptureComponent->CaptureSource = ESceneCaptureSource::SCS_SceneDepth; // Scene Z as float in R channel
		DepthCaptureComponent->SetCaptureKind(EMikanCaptureKind::Depth);
		DepthCaptureComponent->RegisterComponent();
	}
}

void AMikanFrameCaptureActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UMikanEngineSubsystem* Subsystem = UMikanEngineSubsystem::Get())
	{
		Subsystem->CancelRequest(PendingAllocateRequestHandle);
	}
	PendingAllocateRequestHandle = 0;
	bAllocatePending = false;

	DisposeRenderTargets();

	Super::EndPlay(EndPlayReason);
}

void AMikanFrameCaptureActor::NotifyMikanComponentDataBound()
{
	OwnerCameraId = GetOwnerCameraActor()->GetCameraID();
}

void AMikanFrameCaptureActor::NotifyMikanComponentDataUnbound()
{
	OwnerCameraId= -1;
}

void AMikanFrameCaptureActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// Publish the frame we captured on the previous event. Deferring the shared-texture
	// copy by one frame guarantees the GPU has finished the scene capture before Spout
	// reads it on its own device.
	// If you try to publishing immediately after CaptureScene() you will get
	// a read-before-write flicker.
	if (bHasPendingPublish && OwnerCameraId != -1)
	{
		ColorCaptureComponent->PublishCapturedFrame(OwnerCameraId);
		if (DepthCaptureComponent)
		{
			DepthCaptureComponent->PublishCapturedFrame(OwnerCameraId);
		}

		// Shadow buffer: divide the now-complete shadowed pass (A) by the reference pass (B) into
		// the published shadow render target, then publish that. The captures were kicked off on the
		// previous frame, so the GPU has finished A and B; the divide draw and the staging copy in
		// PublishRenderTarget are enqueued in order on the render thread.
		if (ShadowRenderTarget != nullptr)
		{
			if (ShadowDivideMID != nullptr)
			{
				UKismetRenderingLibrary::DrawMaterialToRenderTarget(this, ShadowRenderTarget, ShadowDivideMID);
				ShadowCaptureComponent->PublishRenderTarget(OwnerCameraId, ShadowRenderTarget);
			}
			else
			{
				// No divide material configured: fall back to publishing the raw shadowed pass (A).
				ShadowCaptureComponent->PublishRenderTarget(OwnerCameraId, ShadowShadowedRenderTarget);
			}
		}

		PublishCameraRenderTargetTextures frameInfo;
		frameInfo.camera_id = OwnerCameraId;
		frameInfo.frame_index = PendingPublishFrameIndex;

		MikanAPI->sendRequest(frameInfo);

		bHasPendingPublish= false;
	}
}

// Mikan API Events
void AMikanFrameCaptureActor::HandleNewCameraFrame(const FMikanCameraNewFrameEvent& NewFrameEvent)
{
	// Update the capture actor transform
	SetActorRelativeTransform(NewFrameEvent.CameraTransform);

	// Update the capture components with the new projection matrix
	ColorCaptureComponent->SetCustomProjectionMatrix(NewFrameEvent.ProjectionMatrix);
	if (DepthCaptureComponent)
	{
		DepthCaptureComponent->SetCustomProjectionMatrix(NewFrameEvent.ProjectionMatrix);
	}
	ShadowCaptureComponent->SetCustomProjectionMatrix(NewFrameEvent.ProjectionMatrix);
	ShadowReferenceCaptureComponent->SetCustomProjectionMatrix(NewFrameEvent.ProjectionMatrix);
	SetDepthClippingPlanes(NewFrameEvent.NearClippingPlaneUU, NewFrameEvent.FarClippingPlaneUU);

	DrawDebugCamera(
		GetWorld(), 
		GetActorLocation(), 
		GetActorRotation(), 
		NewFrameEvent.HorizontalFOVDegrees,
		1.f, 
		FColor::Red);

	// Attempt to render a new frame
	if (EnsureRenderBuffers(NewFrameEvent.FrameWidth, NewFrameEvent.FrameHeight))
	{
		// Warn if the last frame we rendered hasn't been publish yet
		if (bHasPendingPublish)
		{
			UE_LOG(MikanXRLog, Warning, 
				TEXT("HandleNewCameraFrame: New Frame %I64d stomping Pending Publish Frame %I64d"), 
				NewFrameEvent.FrameIndex, PendingPublishFrameIndex);
		}

		// Kick off the capture for this frame; it will be published on the next camera frame
		// event once the GPU has finished rendering into the render targets.
		ColorCaptureComponent->CaptureFrame();
		if (DepthCaptureComponent)
		{
			DepthCaptureComponent->CaptureFrame();
		}
		if (ShadowCaptureComponent)
		{
			ShadowCaptureComponent->CaptureFrame();
		}
		if (ShadowReferenceCaptureComponent)
		{
			ShadowReferenceCaptureComponent->CaptureFrame();
		}
		LastRenderedFrameIndex = NewFrameEvent.FrameIndex;
		PendingPublishFrameIndex = NewFrameEvent.FrameIndex;
		bHasPendingPublish = true;
	}
}

void AMikanFrameCaptureActor::SetDepthClippingPlanes(
	float InNearClippingPlaneUU, 
	float InFarClippingPlaneUU)
{
	ColorCaptureComponent->SetDepthClippingPlanes(InNearClippingPlaneUU, InFarClippingPlaneUU);
	if (DepthCaptureComponent)
	{
		DepthCaptureComponent->SetDepthClippingPlanes(InNearClippingPlaneUU, InFarClippingPlaneUU);
	}
	ShadowCaptureComponent->SetDepthClippingPlanes(InNearClippingPlaneUU, InFarClippingPlaneUU);
	ShadowReferenceCaptureComponent->SetDepthClippingPlanes(InNearClippingPlaneUU, InFarClippingPlaneUU);

	const float NewDepthRangeUU= FMath::Abs(InFarClippingPlaneUU - InNearClippingPlaneUU);
	if (!FMath::IsNearlyEqual(DepthRangeUU, NewDepthRangeUU))
	{
		DepthRangeUU= NewDepthRangeUU;
		ReceiveDepthRangeUpdated();
	}
}

// Render Buffer Management
void AMikanFrameCaptureActor::FreeRenderBuffers()
{
	// Drop any deferred capture so we don't try to publish into render targets that are
	// being freed or recreated.
	bHasPendingPublish = false;

	FreeCameraRenderTargetTextures request;
	request.camera_id = OwnerCameraId;

	MikanAPI->sendRequest(request);

	// Nuke the render targets after telling Mikan we are done with them
	DisposeRenderTargets();
}

bool AMikanFrameCaptureActor::EnsureRenderBuffers(int FrameBufferWidth, int FrameBufferHeight)
{
	MikanClientGraphicsApi api = ClientInfo.graphicsAPI;
	if (api != MikanClientGraphicsApi_Direct3D9 &&
		api != MikanClientGraphicsApi_Direct3D11 &&
		api != MikanClientGraphicsApi_Direct3D12 &&
		api != MikanClientGraphicsApi_OpenGL)
	{
		return false;
	}

	if (RenderTargetDesc.width == FrameBufferWidth && RenderTargetDesc.height == FrameBufferHeight)
	{
		// Buffers already match the requested size — ready to capture this frame.
		return true;
	}

	// Size mismatch. If an allocate for the new size is already in flight, just wait for it: don't
	// free the buffers again or fire a duplicate request every frame until the response lands.
	if (bAllocatePending)
	{
		return false;
	}

	UMikanEngineSubsystem* Subsystem = UMikanEngineSubsystem::Get();
	if (!Subsystem)
	{
		return false;
	}

	// Release the old buffers before requesting new ones. The render targets stay null until the
	// async allocate response arrives, so we keep returning false (skipping capture) until then.
	FreeRenderBuffers();

	MikanRenderTargetDescriptor DesiredRenderTargetDesc= {};
	DesiredRenderTargetDesc.width = (uint32_t)FrameBufferWidth;
	DesiredRenderTargetDesc.height = (uint32_t)FrameBufferHeight;
	// RGBA16F half-float preserves the HDR (SCS_SceneColorHDR) values that RTF_RGBA8 truncated.
	DesiredRenderTargetDesc.color_buffer_type = MikanColorBuffer_RGBA16F;
	// Depth is opt-in (bEnableDepthCapture). When off, the color capture bakes environment
	// occlusion of the character client-side and the shadow buffer carries the cast-shadow factor.
	DesiredRenderTargetDesc.depth_buffer_type =
		bEnableDepthCapture ? MikanDepthBuffer_FLOAT_SCENE_DEPTH : MikanDepthBuffer_NODEPTH;
	DesiredRenderTargetDesc.shadow_buffer_type = MikanShadowBuffer_RGBA16F;
	DesiredRenderTargetDesc.graphicsAPI = ClientInfo.graphicsAPI;

	// Allocate shared render textures using the render target descriptor
	AllocateCameraRenderTargetTextures allocateRequest;
	allocateRequest.camera_id = OwnerCameraId;
	allocateRequest.descriptor = DesiredRenderTargetDesc;

	bAllocatePending = true;

	// Weak capture: the response may arrive after this actor is torn down. Capture the descriptor
	// by value so the callback recreates targets for exactly the size it requested.
	TWeakObjectPtr<AMikanFrameCaptureActor> WeakThis(this);
	PendingAllocateRequestHandle = Subsystem->SendRequestAsync(
		allocateRequest,
		[WeakThis, DesiredRenderTargetDesc](const MikanResponsePtr& Response)
		{
			AMikanFrameCaptureActor* Self = WeakThis.Get();
			if (!Self)
			{
				return;
			}

			Self->bAllocatePending = false;
			Self->PendingAllocateRequestHandle = 0;

			if (Response->resultCode == MikanAPIResult::Success)
			{
				// Tell the active scene camera to recreate a matching render target
				if (Self->RecreateRenderTargets(DesiredRenderTargetDesc))
				{
					// Remember the render target desc we setup
					Self->RenderTargetDesc = DesiredRenderTargetDesc;
				}
			}
		});

	return false;
}

bool AMikanFrameCaptureActor::RecreateRenderTargets(const MikanRenderTargetDescriptor& DesiredRenderTargetDesc)
{
	DisposeRenderTargets();

	check(DesiredRenderTargetDesc.color_buffer_type == MikanColorBuffer_RGBA16F);

	ColorRenderTarget = UKismetRenderingLibrary::CreateRenderTarget2D(
		this,
		DesiredRenderTargetDesc.width,
		DesiredRenderTargetDesc.height,
		RTF_RGBA16f,
		FLinearColor::Transparent,
		false);
	if (ColorRenderTarget != nullptr)
	{
		ColorRenderTarget->TargetGamma = GEngine->DisplayGamma;
		ColorCaptureComponent->TextureTarget = ColorRenderTarget;
		ColorCaptureComponent->SetRenderTargetDesc(DesiredRenderTargetDesc);
		ReceiveColorRenderTargetUpdated();
	}
	else
	{
		return false;
	}

	if (DesiredRenderTargetDesc.depth_buffer_type == MikanDepthBuffer_FLOAT_SCENE_DEPTH && DepthCaptureComponent != nullptr)
	{
		DepthRenderTarget = UKismetRenderingLibrary::CreateRenderTarget2D(
			this, 
			DesiredRenderTargetDesc.width,
			DesiredRenderTargetDesc.height,
			RTF_R32f, 
			FLinearColor::Transparent, 
			false);
		if (DepthRenderTarget != nullptr)
		{
			DepthCaptureComponent->TextureTarget = DepthRenderTarget;
			DepthCaptureComponent->SetRenderTargetDesc(DesiredRenderTargetDesc);
			ReceiveDepthRenderTargetUpdated();
		}
		else
		{
			return false;
		}
	}

	if (DesiredRenderTargetDesc.shadow_buffer_type != MikanShadowBuffer_NOSHADOW)
	{
		const uint32_t W= DesiredRenderTargetDesc.width;
		const uint32_t H= DesiredRenderTargetDesc.height;

		// A and B intermediates capture the white catcher (shadowed / reference). They are lit
		// scene captures, so HDR float gives the divide clean headroom before it is normalized.
		ShadowShadowedRenderTarget = UKismetRenderingLibrary::CreateRenderTarget2D(
			this, W, H, RTF_RGBA16f, FLinearColor::White, false);
		ShadowReferenceRenderTarget = UKismetRenderingLibrary::CreateRenderTarget2D(
			this, W, H, RTF_RGBA16f, FLinearColor::White, false);

		// Final divide result that is published to Mikan. Clear to white (multiply identity).
		// RGBA16F half-float preserves shadow/light-survival values that can exceed 1.0.
		ShadowRenderTarget = UKismetRenderingLibrary::CreateRenderTarget2D(
			this, W, H, RTF_RGBA16f, FLinearColor::White, false);

		if (ShadowShadowedRenderTarget == nullptr || ShadowReferenceRenderTarget == nullptr
			|| ShadowRenderTarget == nullptr)
		{
			return false;
		}

		ShadowShadowedRenderTarget->TargetGamma = GEngine->DisplayGamma;
		ShadowReferenceRenderTarget->TargetGamma = GEngine->DisplayGamma;
		ShadowRenderTarget->TargetGamma = GEngine->DisplayGamma;

		ShadowCaptureComponent->TextureTarget = ShadowShadowedRenderTarget;
		ShadowCaptureComponent->SetRenderTargetDesc(DesiredRenderTargetDesc);
		ShadowReferenceCaptureComponent->TextureTarget = ShadowReferenceRenderTarget;
		ShadowReferenceCaptureComponent->SetRenderTargetDesc(DesiredRenderTargetDesc);

		// Build the dynamic divide material instance and bind the A/B inputs.
		AMikanClient* MikanClient= GetOwnerCameraActor() ? GetOwnerCameraActor()->GetOwnerMikanClient() : nullptr;
		UMaterialInterface* DivideMaterial= MikanClient ? MikanClient->ShadowDivideMaterial : nullptr;
		if (DivideMaterial != nullptr)
		{
			ShadowDivideMID= UMaterialInstanceDynamic::Create(DivideMaterial, this);
			ShadowDivideMID->SetTextureParameterValue(FName(TEXT("A")), ShadowShadowedRenderTarget);
			ShadowDivideMID->SetTextureParameterValue(FName(TEXT("B")), ShadowReferenceRenderTarget);
		}
		else
		{
			ShadowDivideMID= nullptr;
			UE_LOG(MikanXRLog, Warning,
				TEXT("RecreateRenderTargets: AMikanClient::ShadowDivideMaterial is unset; shadow buffer will be the raw shadowed pass (A), not A/B."));
		}

		ReceiveShadowRenderTargetUpdated();
	}

	return true;
}

void AMikanFrameCaptureActor::DisposeRenderTargets()
{
	if (ColorRenderTarget != nullptr)
	{
		ColorCaptureComponent->TextureTarget = nullptr;
		UKismetRenderingLibrary::ReleaseRenderTarget2D(ColorRenderTarget);
		ColorRenderTarget = nullptr;
		ReceiveColorRenderTargetUpdated();
	}

	if (DepthRenderTarget != nullptr)
	{
		if (DepthCaptureComponent)
		{
			DepthCaptureComponent->TextureTarget = nullptr;
		}
		UKismetRenderingLibrary::ReleaseRenderTarget2D(DepthRenderTarget);
		DepthRenderTarget = nullptr;
		ReceiveDepthRenderTargetUpdated();
	}

	if (ShadowShadowedRenderTarget != nullptr)
	{
		ShadowCaptureComponent->TextureTarget = nullptr;
		UKismetRenderingLibrary::ReleaseRenderTarget2D(ShadowShadowedRenderTarget);
		ShadowShadowedRenderTarget = nullptr;
	}

	if (ShadowReferenceRenderTarget != nullptr)
	{
		ShadowReferenceCaptureComponent->TextureTarget = nullptr;
		UKismetRenderingLibrary::ReleaseRenderTarget2D(ShadowReferenceRenderTarget);
		ShadowReferenceRenderTarget = nullptr;
	}

	if (ShadowRenderTarget != nullptr)
	{
		UKismetRenderingLibrary::ReleaseRenderTarget2D(ShadowRenderTarget);
		ShadowRenderTarget = nullptr;
		ReceiveShadowRenderTargetUpdated();
	}

	ShadowDivideMID= nullptr;
}

// -- AMikanCameraActor -----
AMikanCameraActor::AMikanCameraActor(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = false;

	CameraForward = CreateDefaultSubobject<USceneComponent>(TEXT("CameraForward"));
	CameraForward->SetupAttachment(RootComponent);
	// Camera capture direction is rotated -90 about the vertical
	// Mikan used the OpenGL convention where render forward is down -Z
	// which becomes down -Y when converted to Unreal coordinates
	CameraForward->SetRelativeRotation(FRotator(0.f, -90.f, 0.f));

#if WITH_EDITORONLY_DATA
	ArrowComponent = CreateEditorOnlyDefaultSubobject<UArrowComponent>(TEXT("ArrowComponent0"));
	if (ArrowComponent)
	{
		ArrowComponent->ArrowColor = FColor::White;
		ArrowComponent->bTreatAsASprite = true;
		ArrowComponent->SpriteInfo.Category = TEXT("Cameras");
		ArrowComponent->SpriteInfo.DisplayName = NSLOCTEXT("SpriteCategory", "Cameras", "Cameras");
		ArrowComponent->SetupAttachment(CameraForward);
		ArrowComponent->bLightAttachment = false;
		ArrowComponent->bIsScreenSizeScaled = true;
	}
#endif // WITH_EDITORONLY_DATA

	FrameCaptureActorClass= AMikanFrameCaptureActor::StaticClass();
}

void AMikanCameraActor::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	auto* EngineSubsystem = UMikanEngineSubsystem::Get();
	AMikanClient* MikanClient = GetOwnerMikanClient();
	if (EngineSubsystem && MikanClient)
	{
		MikanAPI = EngineSubsystem->GetMikanAPI();
		//ClientInfo = EngineSubsystem->GetClientInfo();

		MikanClient->OnMikanDisconnected.AddDynamic(this, &AMikanCameraActor::HandleMikanDisconnected);
		MikanClient->OnNewFrameEvent.AddDynamic(this, &AMikanCameraActor::HandleNewCameraFrame);
	}
}

void AMikanCameraActor::BeginPlay()
{
	Super::BeginPlay();

	if (FrameCaptureActorClass)
	{
		UWorld* World = GetWorld();
		FActorSpawnParameters SpawnParameters;
		SpawnParameters.Owner= this;
		FrameCaptureActor =
			World->SpawnActor<AMikanFrameCaptureActor>(
				FrameCaptureActorClass, 
				this->GetRootComponent()->GetRelativeTransform(),
				SpawnParameters);

		if (FrameCaptureActor)
		{
			AMikanTransformActor* StageActor= GetParentTransformActor();
			check(StageActor);

			FrameCaptureActor->AttachToActor(StageActor, FAttachmentTransformRules::KeepWorldTransform);

			// If we already have transform data bound, let the FrameCaptureActor know it's valid
			if (GetTransformData())
			{
				FrameCaptureActor->NotifyMikanComponentDataBound();
			}
		}
	}
}

void AMikanCameraActor::BindMikanComponentData(class UMikanComponentData* Data)
{
	Super::BindMikanComponentData(Data);

	if (FrameCaptureActor)
	{
		FrameCaptureActor->NotifyMikanComponentDataBound();
	}
}

void AMikanCameraActor::UnbindMikanComponentData(class UMikanComponentData* Data)
{
	if (FrameCaptureActor)
	{
		FrameCaptureActor->NotifyMikanComponentDataUnbound();
	}

	Super::UnbindMikanComponentData(Data);
}

void AMikanCameraActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (FrameCaptureActor)
	{
		FrameCaptureActor->Destroy();
		FrameCaptureActor= nullptr;
	}

	AMikanClient* MikanClient = GetOwnerMikanClient();
	if (MikanClient)
	{
		MikanClient->OnMikanDisconnected.RemoveDynamic(this, &AMikanCameraActor::HandleMikanDisconnected);
		MikanClient->OnNewFrameEvent.RemoveDynamic(this, &AMikanCameraActor::HandleNewCameraFrame);
	}

	Super::EndPlay(EndPlayReason);
}

const UMikanCameraData* AMikanCameraActor::GetCameraData() const
{
	return Cast<UMikanCameraData>(GetTransformData());
}

MikanCameraID AMikanCameraActor::GetCameraID() const
{
	return GetTransformId();
}

// Mikan API Events
void AMikanCameraActor::HandleMikanDisconnected()
{
	if (FrameCaptureActor)
	{
		FrameCaptureActor->FreeRenderBuffers();
	}
}

void AMikanCameraActor::HandleNewCameraFrame(const FMikanCameraNewFrameEvent& NewFrameEvent)
{
	if (NewFrameEvent.CameraID == GetCameraID() && FrameCaptureActor)
	{
		FrameCaptureActor->HandleNewCameraFrame(NewFrameEvent);
	}
}

UE_ENABLE_OPTIMIZATION