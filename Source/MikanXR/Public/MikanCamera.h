#pragma once

#include "GameFramework/Actor.h"
#include "MikanClientTypes.h"
#include "MikanCamera.generated.h"

UCLASS(Blueprintable)
class MIKANXR_API AMikanCamera : public AActor
{
	GENERATED_BODY()

public:
	AMikanCamera(const FObjectInitializer& ObjectInitializer);

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaSeconds ) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Components")
	class UMikanCaptureComponent* MikanCaptureComponent;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Components")
	class UTextureRenderTarget2D* RenderTarget;

	void HandleCameraIntrinsicsChanged();

	void RecreateRenderTarget(const MikanRenderTargetDescriptor& InRTDdesc);
	void DisposeRenderTarget();

protected:
	MikanSpatialAnchorID CameraParentAnchorId = INVALID_MIKAN_ID;
	float MikanCameraScale = 1.f;
	MikanRenderTargetDescriptor RTDdesc;
	uint64 LastReceivedVideoSourceFrame;
};
