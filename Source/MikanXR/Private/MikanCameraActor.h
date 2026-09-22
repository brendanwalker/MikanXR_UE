#pragma once

#include "GameFramework/Actor.h"
#include "MikanTransformActor.h"
#include "MikanAPITypes.h"
#include "MikanClientTypes.h"
#include "MikanVideoSourceTypes.h"
#include "MikanCameraTypes.h"
#include "MikanClient.h"
#include "MikanDataStore.h"
#include "RHIFwd.h"
#include "MikanCameraActor.generated.h"

UCLASS()
class UMikanCameraData : public UMikanTransformData
{
	GENERATED_BODY()

public:
	UMikanCameraData() = default;

	virtual void Initialize(const Serialization::PolymorphicObjectPtr& InValuesObject) override;
	virtual bool ApplyMikanValue(const FString& FieldName, const MikanVariant& FieldValue) override;
	virtual void Describe(TArray<FString>& OutLines) const override;

	inline int32 GetStageId() const { return StageId; }
	inline int32 GetTrackingMountId() const { return TrackingMountId; }
	inline int32 GetVideoSourceId() const { return VideoSourceId; }
	inline int32 GetTrackingFrameDelay() const { return TrackingFrameDelay; }
	inline int32 GetFrameSyncMode() const { return FrameSyncMode; }
	inline bool GetIsPoseDrivenPerFrame() const { return bPoseDrivenPerFrame; }

	// Whether the client renders once per video frame event or on its own clock. Auto resolves
	// against the editor's pose-driven state, the same way the editor resolves it.
	inline bool IsVideoFrameSynced() const
	{
		return FrameSyncMode == (int32)MikanCameraFrameSyncMode_Auto
			? bPoseDrivenPerFrame
			: FrameSyncMode == (int32)MikanCameraFrameSyncMode_VideoFrame;
	}

private:
	int32 StageId = -1;
	int32 TrackingMountId = -1;
	int32 VideoSourceId = -1;
	int32 TrackingFrameDelay = 0;
	int32 FrameSyncMode = (int32)MikanCameraFrameSyncMode_Auto;
	bool bPoseDrivenPerFrame = false;
};

UCLASS(BlueprintType, Blueprintable)
class AMikanFrameCaptureActor : public AActor
{
	GENERATED_BODY()

public:
	AMikanFrameCaptureActor(const FObjectInitializer& ObjectInitializer);

	virtual void PostInitializeComponents() override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaSeconds ) override;

	void NotifyMikanComponentDataBound();
	void NotifyMikanComponentDataUnbound();

	class AMikanCameraActor* GetOwnerCameraActor() const;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class USceneComponent* CameraRoot;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Components")
	class UMikanCaptureComponent* ColorCaptureComponent;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Components")
	class UTextureRenderTarget2D* ColorRenderTarget;

	UFUNCTION(BlueprintImplementableEvent, meta = (DisplayName = "OnColorRenderTargetUpdated"))
	void ReceiveColorRenderTargetUpdated();

	// When true, a depth capture component + depth render target are created and the linearized
	// scene depth buffer is sent to Mikan. Off by default: this client composites shadows and bakes
	// environment occlusion of the character into the color pass client-side, so depth isn't needed.
	// Read at BeginPlay (the depth component is created there), so set it on the class default / a
	// Blueprint subclass rather than expecting a live runtime toggle.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mikan")
	bool bEnableDepthCapture = false;

	// Created at runtime in BeginPlay only when bEnableDepthCapture is set; otherwise null.
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Components")
	class UMikanCaptureComponent* DepthCaptureComponent;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Components")
	class UTextureRenderTarget2D* DepthRenderTarget;

	UFUNCTION(BlueprintImplementableEvent, meta = (DisplayName = "OnDepthRenderTargetUpdated"))
	void ReceiveDepthRenderTargetUpdated();

	// Shadowed pass (A): white catcher with shadow casters present.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Components")
	class UMikanCaptureComponent* ShadowCaptureComponent;

	// Reference pass (B): the same white catcher with no casters.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Components")
	class UMikanCaptureComponent* ShadowReferenceCaptureComponent;

	// A render target (ShadowCaptureComponent target).
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Components")
	class UTextureRenderTarget2D* ShadowShadowedRenderTarget;

	// B render target (ShadowReferenceCaptureComponent target).
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Components")
	class UTextureRenderTarget2D* ShadowReferenceRenderTarget;

	// Final A/B divide result. This is the render target published to Mikan as the shadow buffer.
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Components")
	class UTextureRenderTarget2D* ShadowRenderTarget;

	UFUNCTION(BlueprintImplementableEvent, meta = (DisplayName = "OnShadowRenderTargetUpdated"))
	void ReceiveShadowRenderTargetUpdated();

	UFUNCTION(BlueprintPure)
	inline float GetDepthRange() const { return DepthRangeUU; }

	UFUNCTION(BlueprintImplementableEvent, meta = (DisplayName = "OnDepthRangeUpdated"))
	void ReceiveDepthRangeUpdated();

	// Upper bound on frame events waiting to render in video-frame sync mode. Past it the oldest
	// are dropped, since the compositor has already moved on from them.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mikan")
	int32 MaxPendingFrameEvents = 2;

	// Mikan API Events
	void HandleNewCameraFrame(const FMikanCameraNewFrameEvent& NewFrameEvent);
	void HandleCameraProperties(const FMikanCameraNewFrameEvent& PropertiesEvent);
	void HandleCompositorStarted();
	void HandleCompositorStopped();
	void HandleFrameSyncModeChanged();
	void HandleVideoSourceChanged();
	void FreeRenderBuffers();

protected:
	void SetDepthClippingPlanes(float InNearClippingPlaneUU, float InFarClippingPlaneUU);
	bool EnsureRenderBuffers(int FrameBufferWidth, int FrameBufferHeight);
	bool RecreateRenderTargets(const MikanRenderTargetDescriptor& DesiredRenderTargetDesc);
	void DisposeRenderTargets();

	bool IsVideoFrameSynced() const;
	void ApplyCameraState(const FMikanCameraNewFrameEvent& CameraState);
	bool CanBeginCapture() const;
	void BeginCapture(int64 FrameIndex);
	void TryPublishCaptures();
	void ResetCaptureState();
	void RequestCameraProperties();
	void RequestVideoSourceFrameRate();

protected:
	MikanCameraID OwnerCameraId= -1;
	class IMikanAPI* MikanAPI= nullptr;
	MikanClientInfo ClientInfo;
	MikanRenderTargetDescriptor RenderTargetDesc;
	float DepthRangeUU= 0.f;

	// Dynamic instance of AMikanClient::ShadowDivideMaterial, wired to the A/B render targets.
	UPROPERTY(Transient)
	class UMaterialInstanceDynamic* ShadowDivideMID= nullptr;

	// Frame events waiting to render, oldest first (video-frame sync mode only)
	TArray<FMikanCameraNewFrameEvent> PendingFrameEvents;
	uint64 DroppedFrameEventCount = 0;

	// The newest camera state received, from either a frame event or a properties event. In
	// free-running mode this is what every capture renders from.
	FMikanCameraNewFrameEvent LatestCameraState;
	bool bHasLatestCameraState = false;

	// Whether a compositor is consuming frames for this camera. Set by the compositor started
	// event and by any camera event (a client that joined mid-run sees no started event),
	// cleared by the compositor stopped event and on disconnect. Gates free-running captures.
	bool bCompositorRunning = false;

	// Captures in flight, oldest first. Each capture's scene captures and staging copies are
	// enqueued together with a GPU fence behind them, and it publishes once its fence signals.
	// The engine runs the GPU a frame or two behind the game thread, so waiting for one capture
	// before starting the next would cap the rate at a third of the tick rate; instead captures
	// pipeline through the staging ring, one slot and one fence each.
	struct FInFlightCapture
	{
		int64 FrameIndex = -1;
		int32 SlotIndex = 0;
		uint64 BeginFrame = 0;
	};
	TArray<FInFlightCapture> InFlightCaptures;
	TArray<FGPUFenceRHIRef> SlotFences;
	int32 NextStagingSlot = 0;

	// Free-running pacing: captures are spaced to the video source frame rate when it is known,
	// otherwise one per engine tick
	float VideoSourceFrameRate = 0.f;
	float CaptureAccumulatorSeconds = 0.f;
	uint64 PendingFrameRateRequestHandle = 0;

	// In-flight pull of the camera state, sent when the camera binds. A free-running camera
	// only pushes its state on change, so a client that starts between changes has to ask.
	uint64 PendingPropertiesRequestHandle = 0;

	// Capture statistics, reported through the MikanXR stat group and a log line every few
	// seconds, so the delivered rate can be read instead of judged from the composite
	void UpdateCaptureStats(float DeltaSeconds);
	int32 StatCapturesThisWindow = 0;
	int32 StatPublishesThisWindow = 0;
	int32 StatFenceWaitFramesTotal = 0;
	int32 StatFenceWaitFramesMax = 0;
	float StatWindowSeconds = 0.f;
	float StatCapturesPerSecond = 0.f;
	float StatPublishesPerSecond = 0.f;
	float StatFenceWaitFramesAverage = 0.f;

	// In-flight async render-target allocation. While one is pending we skip both capture and
	// re-issuing another allocate, so a resolution change fires exactly one allocate request.
	uint64 PendingAllocateRequestHandle = 0;
	bool bAllocatePending = false;
};

UCLASS(BlueprintType, Blueprintable)
class AMikanCameraActor : public AMikanTransformActor
{
	GENERATED_BODY()

public:
	AMikanCameraActor(const FObjectInitializer& ObjectInitializer);

	virtual void PostInitializeComponents() override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	virtual void BindMikanComponentData(class UMikanComponentData* Data) override;
	virtual void UnbindMikanComponentData(class UMikanComponentData* Data) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class USceneComponent* CameraForward;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mikan")
	TSubclassOf<class AMikanFrameCaptureActor> FrameCaptureActorClass;

	const UMikanCameraData* GetCameraData() const;
	MikanCameraID GetCameraID() const;

	// Mikan API Events
	UFUNCTION()
	void HandleMikanDisconnected();
	UFUNCTION()
	void HandleNewCameraFrame(const FMikanCameraNewFrameEvent& NewFrameEvent);
	UFUNCTION()
	void HandleCameraProperties(const FMikanCameraNewFrameEvent& PropertiesEvent);
	UFUNCTION()
	void HandleCompositorStarted(int32 CompositorId, int32 CameraId);
	UFUNCTION()
	void HandleCompositorStopped(int32 CompositorId, int32 CameraId);
	void HandleCameraDataChanged(const FString& FieldName);

protected:
#if WITH_EDITORONLY_DATA
	UPROPERTY()
	TObjectPtr<class UArrowComponent> ArrowComponent;
#endif

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
	TObjectPtr<AMikanFrameCaptureActor> FrameCaptureActor;

	class IMikanAPI* MikanAPI= nullptr;
};
