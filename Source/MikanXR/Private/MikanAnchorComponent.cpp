#include "MikanAnchorComponent.h"
#include "MikanAnchorActor.h"
#include "MikanScene.h"
#include "MikanMath.h"

UE_DISABLE_OPTIMIZATION

UMikanAnchorComponent::UMikanAnchorComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

AMikanScene* UMikanAnchorComponent::GetParentScene() const
{
	USceneComponent* AttachParentComponent= GetAttachParent();

	if (AttachParentComponent != nullptr)
	{
		return Cast<AMikanScene>(AttachParentComponent->GetOwner());
	}

	return nullptr;
}

void UMikanAnchorComponent::ApplyAnchorInfo(
	MikanSpatialAnchorID InAnchorId,
	const MikanTransform& InAnchorTransform)
{
	// Set the anchor ID
	AnchorId = InAnchorId;

	// Apply the anchor transform
	ApplyAnchorTransform(InAnchorTransform);
}

void UMikanAnchorComponent::ApplyAnchorTransform(const MikanTransform& InAnchorTransform)
{
	const float MetersToUU = GetWorld()->GetWorldSettings()->WorldToMeters;
	FTransform SceneTransform =
		FMikanMath::MikanTransformToFTransform(
			InAnchorTransform,
			MetersToUU);

	SetRelativeTransform(SceneTransform);
}

#if WITH_EDITOR  
void UMikanAnchorComponent::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	const FName PropertyName = (PropertyChangedEvent.Property != NULL) ? PropertyChangedEvent.Property->GetFName() : NAME_None;
	if (PropertyName == GET_MEMBER_NAME_CHECKED(UMikanAnchorComponent, AnchorName))
	{
		Cast<AMikanAnchorActor>(GetOwner())->UpdateLabelText();
	}	
}
#endif

UE_ENABLE_OPTIMIZATION