#include "MikanLightEnvironmentActor.h"
#include "MikanMath.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

// The skydome material must evaluate the SH environment. Paste this into a
// Custom node with inputs (Dir = the world-space direction, and the nine SH
// parameters wired in as Vector parameters SH0..SH8):
//
//   // Unreal is left handed Z up; Mikan is right handed Y up. The conversion
//   // is a Y/Z swap, which is a HANDEDNESS FLIP and therefore not expressible
//   // as a rotation of the coefficients. So evaluate the basis at the swapped
//   // direction instead of trying to transform the environment.
//   float3 n = float3(Dir.x, Dir.z, Dir.y);
//   float3 c = 0.282095f * SH0
//            + 0.488603f * (n.y * SH1 + n.z * SH2 + n.x * SH3)
//            + 1.092548f * (n.x * n.y * SH4)
//            + 1.092548f * (n.y * n.z * SH5)
//            + 0.315392f * (3.0f * n.z * n.z - 1.0f) * SH6
//            + 1.092548f * (n.x * n.z * SH7)
//            + 0.546274f * (n.x * n.x - n.y * n.y) * SH8;
//   return max(c, 0.0f);   // order-2 SH rings negative around sharp lights
//
// Wire the result into Emissive Color on an Unlit, two-sided material.

// -- UMikanLightEnvironmentData -----
void UMikanLightEnvironmentData::Initialize(const Serialization::PolymorphicObjectPtr& InValuesObject)
{
	Super::Initialize(InValuesObject);

	const auto* Values = InValuesObject.getTypedPointer<MikanLightEnvironmentComponentValues>();
	if (Values == nullptr)
	{
		return;
	}

	SetCoefficientsFromFlatList(Values->sh_coefficients);
	ExposureScale = Values->exposure_scale;
	Directionality = Values->directionality;
	KeyLightDirection = FMikanMath::MikanVector3fToFVector(Values->key_light_direction);
}

void UMikanLightEnvironmentData::SetCoefficientsFromFlatList(const Serialization::List<float>& FlatList)
{
	// Mikan sends 27 floats laid out (coefficient * 3 + channel). A short list
	// would otherwise read past the end, so build to a fixed length and fill
	// only what actually arrived.
	SHCoefficients.Init(FLinearColor::Black, kMikanSHCoefficientCount);

	const int32 AvailableCoefficients =
		FMath::Min((int32)(FlatList.size() / 3), kMikanSHCoefficientCount);
	for (int32 Index = 0; Index < AvailableCoefficients; ++Index)
	{
		SHCoefficients[Index] = FLinearColor(
			FlatList[Index * 3 + 0], FlatList[Index * 3 + 1], FlatList[Index * 3 + 2], 1.f);
	}
}

bool UMikanLightEnvironmentData::ApplyMikanValue(const FString& FieldName, const MikanVariant& FieldValue)
{
	if (FieldName == "sh_coefficients")
	{
		SetCoefficientsFromFlatList(FieldValue.getFloatArrayValue());
		return true;
	}
	else if (FieldName == "exposure_scale")
	{
		ExposureScale = FieldValue.getFloatValue();
		return true;
	}
	else if (FieldName == "directionality")
	{
		Directionality = FieldValue.getFloatValue();
		return true;
	}
	else if (FieldName == "key_light_direction")
	{
		KeyLightDirection = FMikanMath::MikanVector3fToFVector(FieldValue.getVector3fValue());
		return true;
	}
	else
	{
		return Super::ApplyMikanValue(FieldName, FieldValue);
	}
}

void UMikanLightEnvironmentData::Describe(TArray<FString>& OutLines) const
{
	UMikanTransformData::Describe(OutLines);
	Mikan::AppendDescribeLine(OutLines, TEXT("exposure_scale"), ExposureScale);
	Mikan::AppendDescribeLine(OutLines, TEXT("directionality"), Directionality);
	Mikan::AppendDescribeLine(OutLines, TEXT("key_light_direction"), KeyLightDirection);
	Mikan::AppendDescribeLine(OutLines, TEXT("sh_coefficient_count"), SHCoefficients.Num());
	for (int32 Index = 0; Index < SHCoefficients.Num(); ++Index)
	{
		const FLinearColor& Coefficient = SHCoefficients[Index];
		OutLines.Add(FString::Printf(TEXT("sh_coefficient_%d %g %g %g"), Index, Coefficient.R, Coefficient.G, Coefficient.B));
	}
}

// -- AMikanLightEnvironmentActor -----
AMikanLightEnvironmentActor::AMikanLightEnvironmentActor(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SkydomeComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Skydome"));
	SkydomeComponent->SetupAttachment(RootComponent);
	// The dome exists only to be captured by the Sky Light. It must not cast or
	// receive shadows, and it must not occlude anything.
	SkydomeComponent->SetCastShadow(false);
	SkydomeComponent->bCastDynamicShadow = false;
	SkydomeComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	// Uniform positive scale: the material is two-sided, so the inside faces are
	// visible without mirroring the mesh. A negative scale would invert the
	// transform determinant and flip winding/normals for no benefit.
	SkydomeComponent->SetRelativeScale3D(FVector(1000.f));
	SkydomeComponent->SetHiddenInGame(false);
	// Second of the two gates the real-time sky capture applies. Defaulted on by
	// the engine, but set explicitly because the whole actor is useless without
	// it and a future default change would be silent.
	SkydomeComponent->bVisibleInRealTimeSkyCaptures = true;
	// Bounds must reach past the level or the dome gets culled while the Sky
	// Light is still trying to capture it.
	SkydomeComponent->SetBoundsScale(100.f);

	SkyLightComponent = CreateDefaultSubobject<USkyLightComponent>(TEXT("SkyLight"));
	SkyLightComponent->SetupAttachment(RootComponent);
	SkyLightComponent->SetMobility(EComponentMobility::Movable);
	SkyLightComponent->bRealTimeCapture = true;
	SkyLightComponent->SourceType = ESkyLightSourceType::SLS_CapturedScene;
	SkyLightComponent->bLowerHemisphereIsBlack = false;

	KeyLightComponent = CreateDefaultSubobject<UDirectionalLightComponent>(TEXT("KeyLight"));
	KeyLightComponent->SetupAttachment(RootComponent);
	KeyLightComponent->SetMobility(EComponentMobility::Movable);
	KeyLightComponent->SetVisibility(false);

	// A unit sphere is enough; the negative scale above turns it inside out.
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(
		TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (SphereMesh.Succeeded())
	{
		SkydomeComponent->SetStaticMesh(SphereMesh.Object);
	}
}

void AMikanLightEnvironmentActor::BeginPlay()
{
	Super::BeginPlay();

	// Deliberately not defaulted to a /Game/ asset: this plugin is shared across
	// projects, so a hard reference to project content would fail to resolve
	// elsewhere. Without it the dome renders black and silently contributes no
	// light, so say so plainly rather than letting it look like a bad estimate.
	if (SkydomeMaterial == nullptr)
	{
		UE_LOG(LogTemp, Warning,
			   TEXT("AMikanLightEnvironmentActor '%s' has no SkydomeMaterial assigned - the sky light will "
					"capture a black dome and contribute no lighting. Assign a material that evaluates the "
					"SH0..SH8 vector parameters (e.g. M_MikanSkydome)."),
			   *GetName());
	}
	else if (const UMaterial* BaseMaterial = SkydomeMaterial->GetMaterial())
	{
		// A sky light in Real Time Capture mode does NOT capture arbitrary scene
		// meshes. It renders SkyMeshBatches through FSkyPassMeshProcessor, which
		// only accepts materials with bIsSky set. Without it the dome still draws
		// in the main view - so it looks like everything is working - while the
		// capture sees nothing and the sky light contributes no light at all.
		if (!BaseMaterial->bIsSky)
		{
			UE_LOG(LogTemp, Warning,
				   TEXT("AMikanLightEnvironmentActor '%s': skydome material '%s' does not have Is Sky enabled, "
						"so the Real Time Capture sky light will not see it and will contribute no lighting. "
						"Enable 'Is Sky' (Details > Material > Advanced) on that material."),
				   *GetName(), *BaseMaterial->GetName());
		}
	}

	UpdateLightEnvironment();
}

void AMikanLightEnvironmentActor::BindMikanComponentData(UMikanComponentData* Data)
{
	Super::BindMikanComponentData(Data);

	LightEnvironmentData = Cast<UMikanLightEnvironmentData>(Data);

	UpdateLightEnvironment();
}

void AMikanLightEnvironmentActor::OnComponentDataChanged(const FString& FieldName)
{
	if (FieldName == "sh_coefficients" || FieldName == "exposure_scale" ||
		FieldName == "directionality" || FieldName == "key_light_direction")
	{
		UpdateLightEnvironment();
	}
	else
	{
		Super::OnComponentDataChanged(FieldName);
	}
}

float AMikanLightEnvironmentActor::GetDirectionality() const
{
	return LightEnvironmentData ? LightEnvironmentData->GetDirectionality() : 0.f;
}

void AMikanLightEnvironmentActor::UpdateLightEnvironment()
{
	UpdateSkydomeMaterial();
	UpdateKeyLight();

	if (SkyLightComponent != nullptr)
	{
		// Re-read the dome now that its material changed. With Real Time
		// Capture this also happens automatically, but an explicit recapture
		// makes a one-shot update land immediately instead of a frame or two
		// later.
		SkyLightComponent->RecaptureSky();
	}
}

void AMikanLightEnvironmentActor::UpdateSkydomeMaterial()
{
	if (SkydomeComponent == nullptr || SkydomeMaterial == nullptr)
	{
		return;
	}

	if (SkydomeMaterialInstance == nullptr)
	{
		SkydomeMaterialInstance = UMaterialInstanceDynamic::Create(SkydomeMaterial, this);
		SkydomeComponent->SetMaterial(0, SkydomeMaterialInstance);
	}

	if (SkydomeMaterialInstance == nullptr || LightEnvironmentData == nullptr)
	{
		return;
	}

	const TArray<FLinearColor>& Coefficients = LightEnvironmentData->GetSHCoefficients();
	const float Scale = LightEnvironmentData->GetExposureScale() * IntensityScale;

	for (int32 Index = 0; Index < kMikanSHCoefficientCount; ++Index)
	{
		const FLinearColor Coefficient =
			Coefficients.IsValidIndex(Index) ? Coefficients[Index] * Scale : FLinearColor::Black;

		SkydomeMaterialInstance->SetVectorParameterValue(
			FName(*FString::Printf(TEXT("SH%d"), Index)), Coefficient);
	}
}

void AMikanLightEnvironmentActor::UpdateKeyLight()
{
	if (KeyLightComponent == nullptr)
	{
		return;
	}

	if (!bCreateKeyLight || LightEnvironmentData == nullptr)
	{
		KeyLightComponent->SetVisibility(false);
		return;
	}

	// Suppress the key light when the estimate is near-ambient. The dominant
	// direction is arbitrary in that regime - it has been observed flipping
	// sign under small changes to the solve - so pointing a confident light
	// along it is worse than leaving it off.
	const float Directionality = LightEnvironmentData->GetDirectionality();
	if (Directionality < KeyLightDirectionalityThreshold)
	{
		KeyLightComponent->SetVisibility(false);
		return;
	}

	const FVector KeyDirection = LightEnvironmentData->GetKeyLightDirection();
	if (!KeyDirection.IsNearlyZero())
	{
		// The probe reports the direction light arrives FROM, so the light
		// actor must point the opposite way.
		KeyLightComponent->SetWorldRotation((-KeyDirection).GetSafeNormal().Rotation());
	}

	KeyLightComponent->SetVisibility(true);
}
