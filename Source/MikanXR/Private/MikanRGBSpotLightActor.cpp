#include "MikanRGBSpotLightActor.h"
#include "MikanEngineSubsystem.h"
#include "MikanDMXDataStore.h"
#include "Components/ArrowComponent.h"
#include "Components/SpotLightComponent.h"
#include "Engine/World.h"
#include "MikanUtils.h"

// -- UMikanRGBSpotLightData -----
void UMikanRGBSpotLightData::Initialize(const Serialization::PolymorphicObjectPtr& InValuesObject)
{
	UMikanDMXFixtureData::Initialize(InValuesObject);

	const auto* SpotLightValues = InValuesObject.getTypedPointer<MikanRGBSpotLightComponentValues>();
	ConeAngleDegrees = SpotLightValues->cone_angle_degrees;
	ConeRangeMeters = SpotLightValues->cone_range_meters;
}

bool UMikanRGBSpotLightData::ApplyMikanValue(const FString& FieldName, const MikanVariant& FieldValue)
{
	if (FieldName == "cone_angle_degrees")
	{
		ConeAngleDegrees = FieldValue.getFloatValue();
		return true;
	}
	else if (FieldName == "cone_range_meters")
	{
		ConeRangeMeters = FieldValue.getFloatValue();
		return true;
	}
	else
	{
		return UMikanDMXFixtureData::ApplyMikanValue(FieldName, FieldValue);
	}
}

void UMikanRGBSpotLightData::Describe(TArray<FString>& OutLines) const
{
	UMikanDMXFixtureData::Describe(OutLines);
	Mikan::AppendDescribeLine(OutLines, TEXT("cone_angle_degrees"), ConeAngleDegrees);
	Mikan::AppendDescribeLine(OutLines, TEXT("cone_range_meters"), ConeRangeMeters);
}

// -- AMikanRGBSpotLightActor -----
AMikanRGBSpotLightActor::AMikanRGBSpotLightActor(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	LightForward = CreateDefaultSubobject<USceneComponent>(TEXT("CameraForward"));
	LightForward->SetupAttachment(RootComponent);
	// Light Forward direction is rotated -90 about the vertical
	// Mikan used the OpenGL convention where camera forward is down -Z
	// which becomes down -Y when converted to Unreal coordinates
	LightForward->SetRelativeRotation(FRotator(0.f, -90.f, 0.f));

#if WITH_EDITORONLY_DATA
	struct FConstructorStatics
	{
		FName ID_Lighting;
		FText NAME_Lighting;
		FConstructorStatics()
			: ID_Lighting(TEXT("Lighting"))
			, NAME_Lighting(NSLOCTEXT("SpriteCategory", "Lighting", "Lighting"))
		{
		}
	};
	static FConstructorStatics ConstructorStatics;

	ArrowComponent = CreateEditorOnlyDefaultSubobject<UArrowComponent>(TEXT("ArrowComponent0"));
	if (ArrowComponent)
	{
		ArrowComponent->ArrowColor = FColor::White;
		ArrowComponent->bTreatAsASprite = true;
		ArrowComponent->SpriteInfo.Category = ConstructorStatics.ID_Lighting;
		ArrowComponent->SpriteInfo.DisplayName = ConstructorStatics.NAME_Lighting;
		ArrowComponent->SetupAttachment(LightForward);
		ArrowComponent->bLightAttachment = true;
		ArrowComponent->bIsScreenSizeScaled = true;
	}
#endif // WITH_EDITORONLY_DATA

	SpotLightComponent = CreateDefaultSubobject<USpotLightComponent>(TEXT("Spot Light Component"));
	if (SpotLightComponent != nullptr)
	{
		SpotLightComponent->Mobility = EComponentMobility::Stationary;
		SpotLightComponent->IntensityUnits = ELightUnits::Lumens;
		SpotLightComponent->Intensity = 0.f;
		SpotLightComponent->LightColor = FColor::Black;
		SpotLightComponent->bUseTemperature = false;
		SpotLightComponent->SetupAttachment(LightForward);
	}
}

void AMikanRGBSpotLightActor::BindMikanComponentData(UMikanComponentData* Data)
{
	Super::BindMikanComponentData(Data);

	UpdateConeAngle();
	UpdateConeRange();
}

void AMikanRGBSpotLightActor::OnComponentDataChanged(const FString& FieldName)
{
	if (FieldName == "cone_angle_degrees")
	{
		UpdateConeAngle();
	}
	else if (FieldName == "cone_range_meters")
	{
		UpdateConeRange();
	}
	else
	{
		Super::OnComponentDataChanged(FieldName);
	}
}

void AMikanRGBSpotLightActor::OnDMXDataChanged()
{
	UpdateRGBFractions();
	UpdateSpotLightIntensity();
}

void AMikanRGBSpotLightActor::UpdateRGBFractions()
{
	auto* SpotLightData = Cast<UMikanRGBSpotLightData>(GetTransformData());
	if (SpotLightData)
	{
		UMikanEngineSubsystem* ES = UMikanEngineSubsystem::Get();
		UMikanDMXDataStore* DMXDataStore = ES ? ES->GetDMXDataStore() : nullptr;
		if (DMXDataStore)
		{
			const int32 DMXUniverseId = SpotLightData->GetDmxUniverse();
			const int32 DMXStartChannel = SpotLightData->GetDmxStartChannel();

			const UMikanDMXUniverse* DMXUniverse= DMXDataStore->GetDMXUniverse(DMXUniverseId);
			if (DMXUniverse)
			{
				uint8_t Red = DMXUniverse->ReadChannelValue(DMXStartChannel);
				uint8_t Green = DMXUniverse->ReadChannelValue(DMXStartChannel+1);
				uint8_t Blue = DMXUniverse->ReadChannelValue(DMXStartChannel+2);

				DMXRedFraction = (float)Red / 255.f;
				DMXGreenFraction = (float)Green / 255.f;
				DMXBlueFraction = (float)Blue / 255.f;
			}
		}
	}
}

void AMikanRGBSpotLightActor::UpdateConeAngle()
{
	auto* SpotLightData = Cast<UMikanRGBSpotLightData>(GetTransformData());
	if (SpotLightData)
	{
		const float HalfAngle= SpotLightData->GetConeAngleDegrees() * 0.5f;

		SpotLightComponent->SetOuterConeAngle(HalfAngle);
	}
}

void AMikanRGBSpotLightActor::UpdateConeRange()
{
	auto* SpotLightData = Cast<UMikanRGBSpotLightData>(GetTransformData());
	if (SpotLightData)
	{
		const float MetersToUU = GetWorld()->GetWorldSettings()->WorldToMeters;
		const float ConeRangeUU= SpotLightData->GetConeRangeMeters() * MetersToUU;

		SpotLightComponent->SetAttenuationRadius(ConeRangeUU);
	}
}

float AMikanRGBSpotLightActor::GetBrightness() const
{
	// Calculate the power draw fraction based on the DMX [0,1] parameters
	const float MaxWattFraction = (DMXRedFraction + DMXGreenFraction + DMXBlueFraction) / 3.f;

	// Compute the light output in lumens based on the wattage
	const float Watts = MaxWattage * MaxWattFraction;
	const float LumensOutput = Watts * LumensPerWatt;

	return LumensOutput;
}

FLinearColor AMikanRGBSpotLightActor::GetLightColor() const
{
	return FLinearColor(DMXRedFraction, DMXGreenFraction, DMXBlueFraction);
}

void AMikanRGBSpotLightActor::UpdateSpotLightIntensity()
{
	if (SpotLightComponent != nullptr && SpotLightComponent->GetVisibleFlag())
	{
		{
			QUICK_SCOPE_CYCLE_COUNTER(RGBFixtureActorSetIntensity);
			SpotLightComponent->SetIntensity(GetBrightness());
		}

		{
			QUICK_SCOPE_CYCLE_COUNTER(RGBFixtureActorSetLightColor);

			SpotLightComponent->SetLightColor(GetLightColor());
		}
	}
}

void AMikanRGBSpotLightActor::UpdateVisibility()
{
	// Bail early if there is no light
	if (!SpotLightComponent)
		return;

	// Bail if there is no spot light data
	auto* SpotLightData = Cast<UMikanRGBSpotLightData>(GetTransformData());
	if (!SpotLightData)
		return;


	// Want to be visible if lights aren't disabled...
	bool bDesiredVisible = !SpotLightData->IsDisabled();

	// ... and the DMX Channels have a non-zero value ...
	if (bDesiredVisible)
	{

		bDesiredVisible =
			DMXRedFraction > 0.f ||
			DMXGreenFraction > 0.f ||
			DMXBlueFraction > 0.f;
	}

	// ... and if the light can illuminate a mikan renderable component
	if (bDesiredVisible)
	{
		bDesiredVisible =
			UMikanUtils::AnyMikanRenderableOverlapsCone(
				GetOwnerMikanClient(),
				SpotLightComponent->GetComponentLocation(),
				SpotLightComponent->GetComponentQuat().GetForwardVector(),
				SpotLightComponent->OuterConeAngle);
	}

	const bool bPreviousVisible = SpotLightComponent->GetVisibleFlag();
	if (bPreviousVisible != bDesiredVisible)
	{
		// Set intensity back to 0 if we are setting visibility back off
		if (!bDesiredVisible)
		{
			SpotLightComponent->SetIntensity(0.f);
		}

		SpotLightComponent->SetVisibility(bDesiredVisible);
	}
}

#if WITH_EDITOR
void AMikanRGBSpotLightActor::EditorApplyScale(const FVector& DeltaScale, const FVector* PivotLocation, bool bAltDown, bool bShiftDown, bool bCtrlDown)
{
	// Bail early if there is no light
	if (SpotLightComponent == nullptr)
	{
		return;
	}

	const FVector ModifiedScale = DeltaScale * (AActor::bUsePercentageBasedScaling ? 10000.0f : 100.0f);

	if (bCtrlDown)
	{
		FMath::ApplyScaleToFloat(SpotLightComponent->OuterConeAngle, ModifiedScale, 0.01f);
		SpotLightComponent->OuterConeAngle = FMath::Min(89.0f, SpotLightComponent->OuterConeAngle);
		SpotLightComponent->InnerConeAngle = FMath::Min(SpotLightComponent->OuterConeAngle, SpotLightComponent->InnerConeAngle);
	}
	else if (bAltDown)
	{
		FMath::ApplyScaleToFloat(SpotLightComponent->InnerConeAngle, ModifiedScale, 0.01f);
		SpotLightComponent->InnerConeAngle = FMath::Min(89.0f, SpotLightComponent->InnerConeAngle);
		SpotLightComponent->OuterConeAngle = FMath::Max(SpotLightComponent->OuterConeAngle, SpotLightComponent->InnerConeAngle);
	}
	else
	{
		FMath::ApplyScaleToFloat(SpotLightComponent->AttenuationRadius, ModifiedScale);
	}

	PostEditChange();
}
#endif
