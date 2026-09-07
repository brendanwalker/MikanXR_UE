#pragma once

#include "GameFramework/Actor.h"
#include "MikanTransformActor.h"
#include "MikanAPITypes.h"
#include "MikanClientTypes.h"
#include "MikanVideoSourceTypes.h"
#include "MikanDataStore.h"
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

private:
	int32 StageId = -1;
	int32 TrackingMountId = -1;
	int32 VideoSourceId = -1;
	int32 TrackingFrameDelay = 0;
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

	// Mikan API Events
	void HandleNewCameraFrame(const FMikanCameraNewFrameEvent& NewFrameEvent);
	void FreeRenderBuffers();

protected:
	void SetDepthClippingPlanes(float InNearClippingPlaneUU, float InFarClippingPlaneUU);
	bool EnsureRenderBuffers(int FrameBufferWidth, int FrameBufferHeight);
	bool RecreateRenderTargets(const MikanRenderTargetDescriptor& DesiredRenderTargetDesc);
	void DisposeRenderTargets();
	
protected:
	MikanCameraID OwnerCameraId= -1;
	class IMikanAPI* MikanAPI= nullptr;
	MikanClientInfo ClientInfo;
	MikanRenderTargetDescriptor RenderTargetDesc;
	uint64 LastRenderedFrameIndex = 0;
	float DepthRangeUU= 0.f;

	// Dynamic instance of AMikanClient::ShadowDivideMaterial, wired to the A/B render targets.
	UPROPERTY(Transient)
	class UMaterialInstanceDynamic* ShadowDivideMID= nullptr;

	// Deferred publish state: the scene capture is kicked off on one camera frame event and
	// copied to the Mikan shared texture on the next, so the GPU is guaranteed finished first.
	bool bHasPendingPublish = false;
	uint64 PendingPublishFrameIndex = 0;

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

protected:
#if WITH_EDITORONLY_DATA
	UPROPERTY()
	TObjectPtr<class UArrowComponent> ArrowComponent;
#endif

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
	TObjectPtr<AMikanFrameCaptureActor> FrameCaptureActor;

	class IMikanAPI* MikanAPI= nullptr;
};
