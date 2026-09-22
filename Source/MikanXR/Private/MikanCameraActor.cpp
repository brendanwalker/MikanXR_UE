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
#include "Engine/TextureRenderTarget2D.h"
#include "GameFramework/WorldSettings.h"
#include "RenderingThread.h"
#include "RHI.h"
#include "RHICommandList.h"
#include "RHIResources.h"
#include "TextureResource.h"

DECLARE_STATS_GROUP(TEXT("MikanXR"), STATGROUP_MikanXR, STATCAT_Advanced);
DECLARE_FLOAT_COUNTER_STAT(TEXT("Captures / s"), STAT_MikanCapturesPerSecond, STATGROUP_MikanXR);
DECLARE_FLOAT_COUNTER_STAT(TEXT("Publishes / s"), STAT_MikanPublishesPerSecond, STATGROUP_MikanXR);
DECLARE_FLOAT_COUNTER_STAT(TEXT("Fence wait (frames, avg)"), STAT_MikanFenceWaitFrames, STATGROUP_MikanXR);
DECLARE_DWORD_COUNTER_STAT(TEXT("Pending frame events"), STAT_MikanPendingFrameEvents, STATGROUP_MikanXR);
DECLARE_DWORD_COUNTER_STAT(TEXT("Captures in flight"), STAT_MikanCapturesInFlight, STATGROUP_MikanXR);

// How often the capture statistics are logged
static constexpr float kMikanCaptureStatsWindowSeconds = 5.f;

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
	FrameSyncMode = (int32)CameraValues->frame_sync_mode;
	bPoseDrivenPerFrame = CameraValues->pose_driven_per_frame;
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
	else if (FieldName == "frame_sync_mode")
	{
		FrameSyncMode = FieldValue.getIntValue();
		return true;
	}
	else if (FieldName == "pose_driven_per_frame")
	{
		bPoseDrivenPerFrame = FieldValue.getBoolValue();
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
	Mikan::AppendDescribeLine(OutLines, TEXT("frame_sync_mode"), FrameSyncMode);
	Mikan::AppendDescribeLine(OutLines, TEXT("pose_driven_per_frame"), bPoseDrivenPerFrame);
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
		Subsystem->CancelRequest(PendingFrameRateRequestHandle);
		Subsystem->CancelRequest(PendingPropertiesRequestHandle);
	}
	PendingAllocateRequestHandle = 0;
	bAllocatePending = false;
	PendingFrameRateRequestHandle = 0;
	PendingPropertiesRequestHandle = 0;

	// Tell the editor the render targets are gone before they are. The SDK keeps wraps of the
	// staging textures it was handed, keyed by pointer, and a later session could be handed a
	// recycled pointer that still matched a stale wrap.
	if (OwnerCameraId != -1 && MikanAPI != nullptr && RenderTargetDesc.width > 0)
	{
		FreeRenderBuffers();
	}

	ResetCaptureState();
	DisposeRenderTargets();

	Super::EndPlay(EndPlayReason);
}

void AMikanFrameCaptureActor::NotifyMikanComponentDataBound()
{
	OwnerCameraId = GetOwnerCameraActor()->GetCameraID();
	RequestCameraProperties();
	RequestVideoSourceFrameRate();
}

void AMikanFrameCaptureActor::NotifyMikanComponentDataUnbound()
{
	OwnerCameraId= -1;
}

void AMikanFrameCaptureActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	UpdateCaptureStats(DeltaSeconds);

	// Publish every capture whose GPU work has finished, oldest first
	TryPublishCaptures();

	if (OwnerCameraId == -1 || !bHasLatestCameraState)
	{
		return;
	}

	if (IsVideoFrameSynced())
	{
		// Render the oldest frame the editor asked for, at the pose it was asked for. One capture
		// per tick: the engine renders once per tick, so a second one would only queue GPU work.
		if (PendingFrameEvents.Num() == 0 || !CanBeginCapture())
		{
			return;
		}

		const FMikanCameraNewFrameEvent& FrameEvent = PendingFrameEvents[0];
		if (!EnsureRenderBuffers(FrameEvent.FrameWidth, FrameEvent.FrameHeight))
		{
			return;
		}

		ApplyCameraState(FrameEvent);
		BeginCapture(FrameEvent.FrameIndex);
		PendingFrameEvents.RemoveAt(0);
	}
	else
	{
		// Render on our own clock from the latest camera state, while a compositor is consuming.
		// Time accumulates whether or not a capture is in flight, so the cap is a rate rather
		// than a gap after each publish.
		if (!bCompositorRunning)
		{
			CaptureAccumulatorSeconds = 0.f;
			return;
		}

		const float CaptureIntervalSeconds = VideoSourceFrameRate > 0.f ? 1.f / VideoSourceFrameRate : 0.f;
		CaptureAccumulatorSeconds = FMath::Min(CaptureAccumulatorSeconds + DeltaSeconds, 2.f * CaptureIntervalSeconds);
		if (CaptureAccumulatorSeconds < CaptureIntervalSeconds || !CanBeginCapture())
		{
			return;
		}

		if (!EnsureRenderBuffers(LatestCameraState.FrameWidth, LatestCameraState.FrameHeight))
		{
			return;
		}

		CaptureAccumulatorSeconds = FMath::Max(CaptureAccumulatorSeconds - CaptureIntervalSeconds, 0.f);
		ApplyCameraState(LatestCameraState);
		BeginCapture(-1);
	}
}

// Mikan API Events
void AMikanFrameCaptureActor::HandleNewCameraFrame(const FMikanCameraNewFrameEvent& NewFrameEvent)
{
	LatestCameraState = NewFrameEvent;
	bHasLatestCameraState = true;
	bCompositorRunning = true;

	if (!IsVideoFrameSynced())
	{
		return;
	}

	// Queue the frame for Tick. Several events can arrive in one engine tick when the client is
	// behind, and rendering each of them would only make it later, so the queue is bounded and
	// the oldest go: the compositor has already composited past them.
	PendingFrameEvents.Add(NewFrameEvent);

	const int32 MaxPending = FMath::Max(MaxPendingFrameEvents, 1);
	while (PendingFrameEvents.Num() > MaxPending)
	{
		DroppedFrameEventCount++;
		if (DroppedFrameEventCount == 1 || (DroppedFrameEventCount % 100) == 0)
		{
			UE_LOG(MikanXRLog, Warning,
				TEXT("HandleNewCameraFrame: Dropping frame %lld, client is behind the video frame rate (%llu dropped so far)"),
				PendingFrameEvents[0].FrameIndex, DroppedFrameEventCount);
		}
		PendingFrameEvents.RemoveAt(0);
	}
}

void AMikanFrameCaptureActor::HandleCameraProperties(const FMikanCameraNewFrameEvent& PropertiesEvent)
{
	LatestCameraState = PropertiesEvent;
	bHasLatestCameraState = true;
	bCompositorRunning = true;
}

void AMikanFrameCaptureActor::HandleCompositorStarted()
{
	bCompositorRunning = true;
}

void AMikanFrameCaptureActor::HandleCompositorStopped()
{
	bCompositorRunning = false;
	PendingFrameEvents.Empty();
}

void AMikanFrameCaptureActor::HandleFrameSyncModeChanged()
{
	// Frames queued for the old mode are meaningless to the new one
	PendingFrameEvents.Empty();
	CaptureAccumulatorSeconds = 0.f;
}

void AMikanFrameCaptureActor::HandleVideoSourceChanged()
{
	RequestVideoSourceFrameRate();
}

bool AMikanFrameCaptureActor::IsVideoFrameSynced() const
{
	const AMikanCameraActor* CameraActor = GetOwnerCameraActor();
	const UMikanCameraData* CameraData = CameraActor ? CameraActor->GetCameraData() : nullptr;

	// Without camera data the safe assumption is the original behavior: one render per frame event
	return CameraData ? CameraData->IsVideoFrameSynced() : true;
}

void AMikanFrameCaptureActor::ApplyCameraState(const FMikanCameraNewFrameEvent& CameraState)
{
	// Update the capture actor transform
	SetActorRelativeTransform(CameraState.CameraTransform);

	// Update the capture components with the new projection matrix
	ColorCaptureComponent->SetCustomProjectionMatrix(CameraState.ProjectionMatrix);
	if (DepthCaptureComponent)
	{
		DepthCaptureComponent->SetCustomProjectionMatrix(CameraState.ProjectionMatrix);
	}
	ShadowCaptureComponent->SetCustomProjectionMatrix(CameraState.ProjectionMatrix);
	ShadowReferenceCaptureComponent->SetCustomProjectionMatrix(CameraState.ProjectionMatrix);
	SetDepthClippingPlanes(CameraState.NearClippingPlaneUU, CameraState.FarClippingPlaneUU);

	DrawDebugCamera(
		GetWorld(),
		GetActorLocation(),
		GetActorRotation(),
		CameraState.HorizontalFOVDegrees,
		1.f,
		FColor::Red);
}

bool AMikanFrameCaptureActor::CanBeginCapture() const
{
	// A slot frees when its capture publishes, so a full ring means the GPU is behind
	return InFlightCaptures.Num() < UMikanCaptureComponent::kStagingRingDepth;
}

void AMikanFrameCaptureActor::BeginCapture(int64 FrameIndex)
{
	const int32 SlotIndex = NextStagingSlot;
	NextStagingSlot = (NextStagingSlot + 1) % UMikanCaptureComponent::kStagingRingDepth;

	// Kick off the scene captures. The GPU work is enqueued on the render thread.
	ColorCaptureComponent->CaptureFrame();
	if (DepthCaptureComponent)
	{
		DepthCaptureComponent->CaptureFrame();
	}
	ShadowCaptureComponent->CaptureFrame();
	ShadowReferenceCaptureComponent->CaptureFrame();

	// Shadow buffer: divide the shadowed pass (A) by the reference pass (B) into the published
	// shadow render target. Enqueued behind the two captures, so it reads their finished results.
	UTextureRenderTarget2D* PublishedShadowTarget = ShadowShadowedRenderTarget;
	if (ShadowRenderTarget != nullptr && ShadowDivideMID != nullptr)
	{
		UKismetRenderingLibrary::DrawMaterialToRenderTarget(this, ShadowRenderTarget, ShadowDivideMID);
		PublishedShadowTarget = ShadowRenderTarget;
	}

	// Copy every published target into its component's shared staging texture behind the
	// captures, then write a fence so Tick can tell when the staging textures are settled
	// without stalling the game thread on the render thread or the GPU.
	TArray<UMikanCaptureComponent::FStagingCopy> StagingCopies;
	UMikanCaptureComponent::FStagingCopy Copy;
	if (ColorCaptureComponent->PrepareStagingCopy(ColorRenderTarget, SlotIndex, Copy))
	{
		StagingCopies.Add(Copy);
	}
	if (DepthCaptureComponent && DepthCaptureComponent->PrepareStagingCopy(DepthRenderTarget, SlotIndex, Copy))
	{
		StagingCopies.Add(Copy);
	}
	if (ShadowCaptureComponent->PrepareStagingCopy(PublishedShadowTarget, SlotIndex, Copy))
	{
		StagingCopies.Add(Copy);
	}

	// One fence per slot. The slot's previous capture has published by the time it is reused,
	// so clearing here cannot race a poll.
	if (SlotFences.Num() < UMikanCaptureComponent::kStagingRingDepth)
	{
		SlotFences.SetNum(UMikanCaptureComponent::kStagingRingDepth);
	}
	if (!SlotFences[SlotIndex].IsValid())
	{
		SlotFences[SlotIndex] = RHICreateGPUFence(TEXT("MikanCaptureFence"));
	}
	SlotFences[SlotIndex]->Clear();

	FGPUFenceRHIRef Fence = SlotFences[SlotIndex];
	ENQUEUE_RENDER_COMMAND(MikanCopyCapturesToStaging)(
		[StagingCopies, Fence](FRHICommandListImmediate& RHICmdList)
		{
			for (const UMikanCaptureComponent::FStagingCopy& StagingCopy : StagingCopies)
			{
				UMikanCaptureComponent::ExecuteStagingCopy_RenderThread(RHICmdList, StagingCopy);
			}
			RHICmdList.WriteGPUFence(Fence);
		});

	FInFlightCapture InFlight;
	InFlight.FrameIndex = FrameIndex;
	InFlight.SlotIndex = SlotIndex;
	InFlight.BeginFrame = GFrameCounter;
	InFlightCaptures.Add(InFlight);
	StatCapturesThisWindow++;
}

void AMikanFrameCaptureActor::TryPublishCaptures()
{
	// Captures complete in order on the GPU, so stop at the first fence still pending
	while (InFlightCaptures.Num() > 0)
	{
		const FInFlightCapture& InFlight = InFlightCaptures[0];
		const FGPUFenceRHIRef& Fence = SlotFences.IsValidIndex(InFlight.SlotIndex) ? SlotFences[InFlight.SlotIndex] : FGPUFenceRHIRef();
		if (!Fence.IsValid() || !Fence->Poll())
		{
			return;
		}

		if (OwnerCameraId != -1)
		{
			ColorCaptureComponent->PublishSharedStaging(OwnerCameraId, InFlight.SlotIndex);
			if (DepthCaptureComponent)
			{
				DepthCaptureComponent->PublishSharedStaging(OwnerCameraId, InFlight.SlotIndex);
			}
			ShadowCaptureComponent->PublishSharedStaging(OwnerCameraId, InFlight.SlotIndex);

			// A free-running capture carries no frame index; the editor serves its newest texture
			PublishCameraRenderTargetTextures frameInfo;
			frameInfo.camera_id = OwnerCameraId;
			frameInfo.frame_index = InFlight.FrameIndex;

			MikanAPI->sendRequest(frameInfo);
		}

		const int32 FenceWaitFrames = (int32)(GFrameCounter - InFlight.BeginFrame);
		StatPublishesThisWindow++;
		StatFenceWaitFramesTotal += FenceWaitFrames;
		StatFenceWaitFramesMax = FMath::Max(StatFenceWaitFramesMax, FenceWaitFrames);

		InFlightCaptures.RemoveAt(0);
	}
}

void AMikanFrameCaptureActor::UpdateCaptureStats(float DeltaSeconds)
{
	StatWindowSeconds += DeltaSeconds;
	if (StatWindowSeconds >= kMikanCaptureStatsWindowSeconds)
	{
		StatCapturesPerSecond = StatCapturesThisWindow / StatWindowSeconds;
		StatPublishesPerSecond = StatPublishesThisWindow / StatWindowSeconds;
		StatFenceWaitFramesAverage =
			StatPublishesThisWindow > 0 ? (float)StatFenceWaitFramesTotal / StatPublishesThisWindow : 0.f;

		// Only worth a line while something is happening or was expected to
		if (StatCapturesThisWindow > 0 || bCompositorRunning)
		{
			UE_LOG(MikanXRLog, Log,
				TEXT("MikanFrameCapture camera %d: %.1f captures/s, %.1f publishes/s, fence wait avg %.1f max %d frames, %d in flight, %d pending, %llu dropped, %s"),
				OwnerCameraId, StatCapturesPerSecond, StatPublishesPerSecond, StatFenceWaitFramesAverage,
				StatFenceWaitFramesMax, InFlightCaptures.Num(), PendingFrameEvents.Num(), DroppedFrameEventCount,
				IsVideoFrameSynced() ? TEXT("video frame sync") : TEXT("free running"));
		}

		StatWindowSeconds = 0.f;
		StatCapturesThisWindow = 0;
		StatPublishesThisWindow = 0;
		StatFenceWaitFramesTotal = 0;
		StatFenceWaitFramesMax = 0;
	}

	// Every camera in the project has a capture actor ticking, so the stats are summed across
	// them rather than set, or whichever idle actor ticks last would report zero for all
	INC_FLOAT_STAT_BY(STAT_MikanCapturesPerSecond, StatCapturesPerSecond);
	INC_FLOAT_STAT_BY(STAT_MikanPublishesPerSecond, StatPublishesPerSecond);
	INC_FLOAT_STAT_BY(STAT_MikanFenceWaitFrames, StatFenceWaitFramesAverage);
	INC_DWORD_STAT_BY(STAT_MikanPendingFrameEvents, PendingFrameEvents.Num());
	INC_DWORD_STAT_BY(STAT_MikanCapturesInFlight, InFlightCaptures.Num());
}

void AMikanFrameCaptureActor::ResetCaptureState()
{
	PendingFrameEvents.Empty();
	InFlightCaptures.Empty();
	NextStagingSlot = 0;
	CaptureAccumulatorSeconds = 0.f;
}

void AMikanFrameCaptureActor::RequestCameraProperties()
{
	UMikanEngineSubsystem* Subsystem = UMikanEngineSubsystem::Get();
	if (!Subsystem || OwnerCameraId == -1)
	{
		return;
	}

	Subsystem->CancelRequest(PendingPropertiesRequestHandle);
	PendingPropertiesRequestHandle = 0;

	GetCameraProperties propertiesRequest;
	propertiesRequest.camera_id = OwnerCameraId;

	TWeakObjectPtr<AMikanFrameCaptureActor> WeakThis(this);
	PendingPropertiesRequestHandle = Subsystem->SendRequestAsync(
		propertiesRequest,
		[WeakThis](const MikanResponsePtr& Response)
		{
			AMikanFrameCaptureActor* Self = WeakThis.Get();
			if (!Self)
			{
				return;
			}

			Self->PendingPropertiesRequestHandle = 0;

			if (Response->resultCode != MikanAPIResult::Success)
			{
				return;
			}

			// The pulled state is treated exactly like a pushed properties event. A frame or
			// properties event that arrived in the meantime is newer, so it wins.
			auto PropertiesResponse = std::static_pointer_cast<MikanCameraPropertiesResponse>(Response);
			const float MetersToUU = Self->GetWorld()->GetWorldSettings()->WorldToMeters;
			if (!Self->bHasLatestCameraState)
			{
				Self->LatestCameraState = AMikanClient::MakeUnrealCameraEvent(*PropertiesResponse, MetersToUU, -1);
				Self->bHasLatestCameraState = true;
			}
			Self->bCompositorRunning = Self->bCompositorRunning || PropertiesResponse->compositor_running;
		});
}

void AMikanFrameCaptureActor::RequestVideoSourceFrameRate()
{
	UMikanEngineSubsystem* Subsystem = UMikanEngineSubsystem::Get();
	const AMikanCameraActor* CameraActor = GetOwnerCameraActor();
	const UMikanCameraData* CameraData = CameraActor ? CameraActor->GetCameraData() : nullptr;
	if (!Subsystem || !CameraData)
	{
		return;
	}

	Subsystem->CancelRequest(PendingFrameRateRequestHandle);
	PendingFrameRateRequestHandle = 0;
	VideoSourceFrameRate = 0.f;

	if (CameraData->GetVideoSourceId() == -1)
	{
		return;
	}

	GetVideoSourceMode modeRequest;
	modeRequest.video_source_id = CameraData->GetVideoSourceId();

	TWeakObjectPtr<AMikanFrameCaptureActor> WeakThis(this);
	PendingFrameRateRequestHandle = Subsystem->SendRequestAsync(
		modeRequest,
		[WeakThis](const MikanResponsePtr& Response)
		{
			AMikanFrameCaptureActor* Self = WeakThis.Get();
			if (!Self)
			{
				return;
			}

			Self->PendingFrameRateRequestHandle = 0;

			if (Response->resultCode == MikanAPIResult::Success)
			{
				auto ModeResponse = std::static_pointer_cast<MikanVideoSourceModeResponse>(Response);
				Self->VideoSourceFrameRate = ModeResponse->frame_rate;
			}
		});
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
	// Drop any pending or in-flight capture so we don't publish render targets that are
	// being freed or recreated.
	ResetCaptureState();

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
		MikanClient->OnCameraPropertiesEvent.AddDynamic(this, &AMikanCameraActor::HandleCameraProperties);
		MikanClient->OnCompositorStarted.AddDynamic(this, &AMikanCameraActor::HandleCompositorStarted);
		MikanClient->OnCompositorStopped.AddDynamic(this, &AMikanCameraActor::HandleCompositorStopped);
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

	// The base class unbinds every subscription of ours on unbind
	if (Data)
	{
		Data->OnComponentDataChanged.AddUObject(this, &AMikanCameraActor::HandleCameraDataChanged);
	}

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
		MikanClient->OnCameraPropertiesEvent.RemoveDynamic(this, &AMikanCameraActor::HandleCameraProperties);
		MikanClient->OnCompositorStarted.RemoveDynamic(this, &AMikanCameraActor::HandleCompositorStarted);
		MikanClient->OnCompositorStopped.RemoveDynamic(this, &AMikanCameraActor::HandleCompositorStopped);
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
		FrameCaptureActor->HandleCompositorStopped();
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

void AMikanCameraActor::HandleCameraProperties(const FMikanCameraNewFrameEvent& PropertiesEvent)
{
	if (PropertiesEvent.CameraID == GetCameraID() && FrameCaptureActor)
	{
		FrameCaptureActor->HandleCameraProperties(PropertiesEvent);
	}
}

void AMikanCameraActor::HandleCompositorStarted(int32 CompositorId, int32 CameraId)
{
	if (CameraId == GetCameraID() && FrameCaptureActor)
	{
		FrameCaptureActor->HandleCompositorStarted();
	}
}

void AMikanCameraActor::HandleCompositorStopped(int32 CompositorId, int32 CameraId)
{
	if (CameraId == GetCameraID() && FrameCaptureActor)
	{
		FrameCaptureActor->HandleCompositorStopped();
	}
}

void AMikanCameraActor::HandleCameraDataChanged(const FString& FieldName)
{
	if (!FrameCaptureActor)
	{
		return;
	}

	if (FieldName == "frame_sync_mode" || FieldName == "pose_driven_per_frame")
	{
		FrameCaptureActor->HandleFrameSyncModeChanged();
	}
	else if (FieldName == "video_source_id")
	{
		FrameCaptureActor->HandleVideoSourceChanged();
	}
}

UE_ENABLE_OPTIMIZATION