#include "MikanStencilComponent.h"
#include "MikanScene.h"
#include "MikanMath.h"

UMikanStencilComponent::UMikanStencilComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{}

AMikanScene* UMikanStencilComponent::GetParentScene() const
{
	USceneComponent* AttachParentComponent = GetAttachParent();

	if (AttachParentComponent != nullptr)
	{
		return Cast<AMikanScene>(AttachParentComponent->GetOwner());
	}

	return nullptr;
}

void UMikanStencilComponent::ApplyStencilInfo(
	MikanStencilID InStencilId,
	const std::string& InStencilName,
	const MikanTransform& InStencilTransform)
{
	// Set the stencil ID
	StencilId = InStencilId;

	// Copy Stencil Name (convert to UTF-16 from ansi string)
	auto StencilNameTChar = StringCast<TCHAR>(InStencilName.c_str());
	ApplyStencilName(StencilNameTChar.Get());

	// Apply the stencil transform
	ApplyStencilTransform(InStencilTransform);
}

void UMikanStencilComponent::ApplyStencilName(const FString& InStencilName)
{
	StencilName = InStencilName;
}

void UMikanStencilComponent::ApplyStencilTransform(const MikanTransform& InStencilTransform)
{
	const float MetersToUU = GetWorld()->GetWorldSettings()->WorldToMeters;
	FTransform SceneTransform =
		FMikanMath::MikanTransformToFTransform(
			InStencilTransform,
			MetersToUU);

	SetRelativeTransform(SceneTransform);
}