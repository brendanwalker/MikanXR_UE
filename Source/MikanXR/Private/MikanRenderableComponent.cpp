#include "MikanRenderableComponent.h"
#include "MikanWorldSubsystem.h"

UMikanRenderableComponent::UMikanRenderableComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{}


void UMikanRenderableComponent::BeginPlay()
{
	Super::BeginPlay();

	UMikanWorldSubsystem::GetInstance(GetWorld())->RegisterMikanRenderable(this);
}

void UMikanRenderableComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UMikanWorldSubsystem::GetInstance(GetWorld())->UnregisterMikanRenderable(this);

	Super::EndPlay(EndPlayReason);
}
