// Fill out your copyright notice in the Description page of Project Settings.

#include "MikanUtils.h"

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "MikanClient.h"
#include "MikanRenderableComponent.h"

bool UMikanUtils::AnyMikanRenderableOverlapsCone(
	AMikanClient* MikanClient,
	const FVector& Origin, 
	const FVector& Direction, 
	float HalfAngleDegrees)
{
	if (MikanClient)
	{
		auto& RegisteredRenderables = MikanClient->GetRegisteredMikanRenderables();

		for (UMikanRenderableComponent* Renderable : RegisteredRenderables)
		{
			if (Renderable && Renderable->OverlapsCone(Origin, Direction, HalfAngleDegrees))
			{
				return true;
			}
		}
	}

	return false;
}
