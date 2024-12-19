#include "MikanBoxStencilActor.h"
#include "MikanCamera.h"
#include "Engine/Engine.h"
#include "MikanMath.h"
#include "MikanScene.h"
#include "MikanStencilTypes.h"
#include "MikanStencilComponent.h"

AMikanBoxStencilActor::AMikanBoxStencilActor(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	Extents= FVector::Zero();
}

void AMikanBoxStencilActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	FVector Center = GetActorLocation();
	FRotator Rot = GetActorRotation();
	DrawDebugBox(GetWorld(), Center, Extents, Rot.Quaternion(), FColor::Yellow);
}

AMikanBoxStencilActor* AMikanBoxStencilActor::SpawnStencil(
	AMikanScene* OwnerScene,
	const MikanStencilBoxInfo& BoxStencilInfo)
{
	// Spawn a new stencil
	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Name = *FString::Printf(TEXT("BoxStencil_%d"), BoxStencilInfo.stencil_id);
	SpawnParameters.Owner = OwnerScene;

	// Spawn the stencil actor
	UWorld* World = OwnerScene->GetWorld();
	AMikanBoxStencilActor* NewStencil = 
		World->SpawnActor<AMikanBoxStencilActor>(
			AMikanBoxStencilActor::StaticClass(), 
			FTransform::Identity, 
			SpawnParameters);

	NewStencil->AttachToActor(OwnerScene, FAttachmentTransformRules::SnapToTargetIncludingScale);
	NewStencil->ApplyBoxStencilInfo(BoxStencilInfo);

	return NewStencil;
}

void AMikanBoxStencilActor::ApplyBoxStencilInfo(const MikanStencilBoxInfo& InStencilInfo)
{
	StencilComponent->ApplyStencilInfo(
		InStencilInfo.stencil_id,
		InStencilInfo.stencil_name.getValue(),
		InStencilInfo.relative_transform);

	const float MetersToUU = GetWorld()->GetWorldSettings()->WorldToMeters;
	Extents= FVector(
		InStencilInfo.box_x_size * 0.5f * MetersToUU,
		InStencilInfo.box_y_size * 0.5f * MetersToUU,
		InStencilInfo.box_z_size * 0.5f * MetersToUU);
}