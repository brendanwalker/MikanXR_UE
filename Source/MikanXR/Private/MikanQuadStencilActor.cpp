#include "MikanQuadStencilActor.h"
#include "MikanCamera.h"
#include "Engine/Engine.h"
#include "MikanMath.h"
#include "MikanScene.h"
#include "MikanStencilTypes.h"
#include "MikanStencilComponent.h"

AMikanQuadStencilActor::AMikanQuadStencilActor(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	QuadSize= FVector2D::Zero();
}

void AMikanQuadStencilActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	FVector Center = GetActorLocation();
	FRotator Rot = GetActorRotation();
	FVector Extent = FVector(QuadSize.X, QuadSize.Y, 1.0f);
	DrawDebugBox(GetWorld(), Center, Extent, Rot.Quaternion(), FColor::Yellow);
}

AMikanQuadStencilActor* AMikanQuadStencilActor::SpawnStencil(
	AMikanScene* OwnerScene,
	const MikanStencilQuadInfo& QuadStencilInfo)
{
	// Spawn a new stencil
	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Name = *FString::Printf(TEXT("QuadStencil_%d"), QuadStencilInfo.stencil_id);
	SpawnParameters.Owner = OwnerScene;

	// Spawn the stencil actor
	UWorld* World = OwnerScene->GetWorld();
	AMikanQuadStencilActor* NewStencil = 
		World->SpawnActor<AMikanQuadStencilActor>(
			AMikanQuadStencilActor::StaticClass(), 
			FTransform::Identity, 
			SpawnParameters);

	NewStencil->AttachToActor(OwnerScene, FAttachmentTransformRules::SnapToTargetIncludingScale);
	NewStencil->ApplyQuadStencilInfo(QuadStencilInfo);

	return NewStencil;
}

void AMikanQuadStencilActor::ApplyQuadStencilInfo(const MikanStencilQuadInfo& InStencilInfo)
{
	StencilComponent->ApplyStencilInfo(
		InStencilInfo.stencil_id,
		InStencilInfo.stencil_name.getValue(),
		InStencilInfo.relative_transform);

	const float MetersToUU = GetWorld()->GetWorldSettings()->WorldToMeters;
	QuadSize= FVector2D(
		InStencilInfo.quad_width * 0.5f * MetersToUU,
		InStencilInfo.quad_height * 0.5f * MetersToUU);
}