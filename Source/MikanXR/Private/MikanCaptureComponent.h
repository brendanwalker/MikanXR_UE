// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneCaptureComponent2D.h"
#include "RHIFwd.h"
#include "PixelFormat.h"
#include "MikanCoreTypes.h"
#include "MikanVideoSourceTypes.h"
#include "MikanCaptureComponent.generated.h"

// Which Mikan buffer this capture component feeds. Determines both the per-actor show-only tag
// it filters on and the shared-texture write request it issues on publish.
UENUM()
enum class EMikanCaptureKind : uint8
{
	Color,
	Depth,
	// Shadowed pass (A): white catcher lit with shadow casters present. Includes untagged
	// renderables (e.g. the character) so they cast shadows onto the catcher.
	Shadow,
	// Reference pass (B): the same white catcher with NO casters. Excludes untagged renderables.
	// Divided into Shadow (A/B) to isolate the per-channel shadow factor.
	ShadowReference,
};

UCLASS(hidecategories=(Collision, Object, Physics, SceneComponent), ClassGroup=Rendering, editinlinenew, meta=(BlueprintSpawnableComponent))
class UMikanCaptureComponent : public USceneCaptureComponent2D
{
	GENERATED_BODY()

public:
	static FName MikanRenderColor;
	static FName MikanRenderDepth;
	static FName MikanRenderShadow;

	UMikanCaptureComponent(const FObjectInitializer& ObjectInitializer);

	void SetCaptureKind(EMikanCaptureKind InCaptureKind) { CaptureKind = InCaptureKind; }
	EMikanCaptureKind GetCaptureKind() const { return CaptureKind; }

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	void SetRenderTargetDesc(const MikanRenderTargetDescriptor& InRTDdesc);
	void SetCustomProjectionMatrix(const FMatrix& InProjectionMatrix);
	void SetDepthClippingPlanes(float InNearClippingPlaneUU, float InFarClippingPlaneUU);

	// Kicks off the scene capture for this frame's render target. The actual GPU work is
	// enqueued on the render thread and is not complete when this returns.
	void CaptureFrame();

	// Copies the most recently captured render target into the Mikan shared (Spout) texture.
	// Must be called on a later tick than the matching CaptureFrame() so the GPU has finished
	// rendering before Spout reads the texture on its own device.
	void PublishCapturedFrame(int32 CameraId);

	// Publishes an arbitrary render target as this component's buffer kind. Used for the shadow
	// pass, where the published texture is the A/B divide result rather than this component's
	// own capture target.
	void PublishRenderTarget(int32 CameraId, class UTextureRenderTarget2D* SourceTarget);

	// Mikan UE4 Events
	UFUNCTION()
	void HandleMikanRenderableRegistered(class UMikanRenderableComponent* Renderable);
	UFUNCTION()
	void HandleMikanRenderableUnregistered(class UMikanRenderableComponent* Renderable);

protected:
	class AMikanClient* GetOwnerMikanClient() const;

	// Copies the captured render target into the shared (Spout-bound) staging texture on the
	// render thread and blocks until the GPU has finished, so the texture is fully rendered and
	// in a known resource state before Mikan wraps and reads it. Returns the native API texture
	// pointer (e.g. ID3D12Resource*) of the staging texture, or nullptr on failure.
	void* UpdateSharedStagingTexture(class UTextureRenderTarget2D* SourceTarget);

protected:
	EMikanCaptureKind CaptureKind= EMikanCaptureKind::Color;
	class IMikanAPI* MikanAPI= nullptr;
	MikanRenderTargetDescriptor RenderTargetDesc;
	MikanMonoIntrinsics VideoSourceIntrinsics;
	float NearClippingPlaneUU;
	float FarClippingPlaneUU;

	// Dedicated staging texture that Mikan wraps and copies to the Spout shared texture. We copy
	// the scene-capture render target into this each frame instead of handing Mikan the live
	// render target directly, so Unreal's renderer never re-renders into the resource Mikan has
	// wrapped (which caused D3D12 resource-state conflicts and flicker). Created with the Shared
	// flag and left in SRVMask so its rest state matches Mikan's GENERIC_READ wrap.
	FTextureRHIRef SharedStagingTexture;
	int32 SharedStagingWidth = 0;
	int32 SharedStagingHeight = 0;
	EPixelFormat SharedStagingFormat = PF_Unknown;
};
