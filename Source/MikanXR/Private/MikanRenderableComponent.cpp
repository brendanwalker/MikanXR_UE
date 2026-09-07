#include "MikanRenderableComponent.h"
#include "MikanTransformActor.h"
#include "MikanClient.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"

UMikanRenderableComponent::UMikanRenderableComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{}


void UMikanRenderableComponent::BeginPlay()
{
	Super::BeginPlay();

	OwnerMikanClient= 
		CastChecked<AMikanClient>(
			UGameplayStatics::GetActorOfClass(this, AMikanClient::StaticClass()));
	if (OwnerMikanClient)
	{
		OwnerMikanClient->RegisterMikanRenderable(this);
	}
}

void UMikanRenderableComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (OwnerMikanClient)
	{
		OwnerMikanClient->UnregisterMikanRenderable(this);
	}

	Super::EndPlay(EndPlayReason);
}

bool UMikanRenderableComponent::OverlapsCone(
	const FVector& ConeOrigin, 
	const FVector& ConeDirection, 
	float ConeHalfAngleDegrees) const
{
	const FVector ConeUnitDirection= ConeDirection.GetSafeNormal();

	FVector ActorOrigin, ActorBoxExtents;
	GetOwner()->GetActorBounds(true, ActorOrigin, ActorBoxExtents);

	const FVector ToActorOrigin= ActorOrigin - ConeOrigin;
	const float SqrdDistToActor= ToActorOrigin.SquaredLength();
	const float ActorProjDistAlongConeDir= FVector::DotProduct(ConeUnitDirection, ToActorOrigin);

	// Is the actor in front of the light
	if (ActorProjDistAlongConeDir > 0.f)
	{
		const float ConeRadiusAtProjDist= 
			ActorProjDistAlongConeDir * FMath::Sin(FMath::DegreesToRadians(ConeHalfAngleDegrees));

		const float ActorPerpDistFromConeDir = 
			FMath::Sqrt(SqrdDistToActor - ActorProjDistAlongConeDir * ActorProjDistAlongConeDir);

		const float ActorSphereRadius= ActorBoxExtents.Length();
		const bool bSphereOverlapsCone= 
			(ActorPerpDistFromConeDir - ConeRadiusAtProjDist) < ActorSphereRadius;

		return bSphereOverlapsCone;
	}

	return false;
}