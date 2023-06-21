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

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaSeconds ) override;

	void RecreateRenderTarget(const MikanRenderTargetDescriptor& InRTDdesc);
	void DisposeRenderTarget();

	void SetCameraFOV(float FOVDegrees);

	UFUNCTION(BlueprintNativeEvent)
	void RebuildHiddenActorList();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Components")
	class UMikanCaptureComponent* MikanCaptureComponent;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Components")
	class UTextureRenderTarget2D* RenderTarget;

protected:
	MikanRenderTargetDescriptor RTDdesc;
};
