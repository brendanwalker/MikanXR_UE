#include "MikanStencilActor.h"
#include "MikanCamera.h"
#include "Engine/Engine.h"
#include "MikanScene.h"
#include "MikanStencilComponent.h"
#include "Components/TextRenderComponent.h"

AMikanStencilActor::AMikanStencilActor(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = true;

	StencilComponent = CreateDefaultSubobject<UMikanStencilComponent>(TEXT("StencilRoot"));
	RootComponent = StencilComponent;

	LabelComponent = CreateDefaultSubobject<UTextRenderComponent>(TEXT("Label"));
	LabelComponent->SetupAttachment(RootComponent);
}

AMikanScene* AMikanStencilActor::GetParentScene() const
{
	USceneComponent* AttachParentComponent = RootComponent->GetAttachParent();

	if (AttachParentComponent != nullptr)
	{
		return Cast<AMikanScene>(AttachParentComponent->GetOwner());
	}

	return nullptr;
}

int32 AMikanStencilActor::GetStencilId() const
{
	return StencilComponent->StencilId;
}

void AMikanStencilActor::ApplyStencilName(const FString& InAnchorName)
{
	StencilComponent->ApplyStencilName(InAnchorName);
	UpdateLabelText();
}

void AMikanStencilActor::UpdateLabelText()
{
	LabelComponent->SetText(FText::FromString(StencilComponent->StencilName));
}