#include "MikanAnchorComponent.h"
#include "MikanScene.h"

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

void UMikanAnchorComponent::FetchAnchorInfo()
{
	AMikanScene* OwnerScene= GetParentScene();

	if (OwnerScene != nullptr)
	{
		const FMikanAnchorInfo* MikanAnchorInfo= OwnerScene->GetMikanAnchorInfoByName(AnchorName);

		if (MikanAnchorInfo != nullptr)
		{
			AnchorId= MikanAnchorInfo->AnchorID;

			// Update our scene transform now that we have an assigned anchor id
			UpdateSceneTransform();
		}
	}
}

void UMikanAnchorComponent::UpdateSceneTransform()
{
	if (AnchorId != INVALID_MIKAN_ID)
	{
		AMikanScene* OwnerScene = GetParentScene();

		if (OwnerScene != nullptr)
		{
			// Get the corresponding mikan anchor info from the scene
			const FMikanAnchorInfo* MikanAnchorInfo = OwnerScene->GetMikanAnchorInfoById(AnchorId);

			if (MikanAnchorInfo != nullptr)
			{
				// Get the anchor transform in Mikan Space
				const FTransform& MikanSpaceTransform= MikanAnchorInfo->MikanSpaceTransform;

				// Get the conversion from the scene to go from Mikan to Scene space
				const FTransform& MikanToSceneXform = OwnerScene->GetMikanToSceneTransform();

				// Compute the Scene space transform
				const FTransform SceneSpaceTransform= MikanSpaceTransform * MikanToSceneXform;

				// Update the anchor scene componetn transform
				SetRelativeTransform(SceneSpaceTransform);
			}
		}
	}
}
