#pragma once

#include "MikanTransformActor.h"
#include "MikanLightTypes.h"
#include "MikanLightEnvironmentActor.generated.h"

/// Number of order-2 spherical harmonic coefficients (9), each RGB.
static constexpr int32 kMikanSHCoefficientCount = 9;

UCLASS()
class UMikanLightEnvironmentData : public UMikanTransformData
{
	GENERATED_BODY()

public:
	UMikanLightEnvironmentData() = default;

	virtual void Initialize(const Serialization::PolymorphicObjectPtr& InValuesObject) override;
	virtual bool ApplyMikanValue(const FString& FieldName, const MikanVariant& FieldValue) override;
	virtual void Describe(TArray<FString>& OutLines) const override;

	/// Coefficients as RGB, already converted out of Mikan's flat 27-float list.
	/// These are RADIANCE in MIKAN space - see AMikanLightEnvironmentActor for
	/// why they are not converted to Unreal space here.
	const TArray<FLinearColor>& GetSHCoefficients() const { return SHCoefficients; }

	float GetExposureScale() const { return ExposureScale; }

	/// l=1 over l=0 band energy. Below ~0.25 the estimate is effectively uniform
	/// ambient and GetKeyLightDirection() is not meaningful.
	float GetDirectionality() const { return Directionality; }

	/// Key light direction, already converted to Unreal space.
	const FVector& GetKeyLightDirection() const { return KeyLightDirection; }

private:
	void SetCoefficientsFromFlatList(const Serialization::List<float>& FlatList);

	TArray<FLinearColor> SHCoefficients;
	float ExposureScale = 1.f;
	float Directionality = 0.f;
	FVector KeyLightDirection = FVector::ForwardVector;
};

/**
 * Applies a Mikan scene lighting probe to the Unreal scene.
 *
 * The probe carries the real scene's low-frequency lighting recovered from a
 * captured video frame as order-2 spherical harmonics. That is enough to light
 * a character with the correct soft/ambient environment, and - because the
 * existing Mikan shadow-catcher passes render against whatever lights the level
 * has - it also makes the composited shadow reflect the real scene.
 *
 * How it reaches the renderer: the coefficients drive an unlit emissive skydome
 * material, and a Movable SkyLight in Real Time Capture mode re-reads that dome.
 * Order-2 SH has no high-frequency detail to lose, so this needs no cubemap
 * asset and no render target - updating the lighting is nine vector parameter
 * writes.
 *
 * Two limits inherited from the representation (docs/reference/scene-lighting.md
 * in the MikanXR repo):
 *   - It cannot represent a sharp light. Enable bCreateKeyLight to add an
 *     explicit directional light if the shot needs a crisp shadow.
 *   - Radiance can evaluate negative. The material must clamp; Unreal cannot
 *     emit negative light regardless.
 */
UCLASS(BlueprintType, Blueprintable)
class AMikanLightEnvironmentActor : public AMikanTransformActor
{
	GENERATED_BODY()

public:
	AMikanLightEnvironmentActor(const FObjectInitializer& ObjectInitializer);

	virtual void BeginPlay() override;
	virtual void BindMikanComponentData(class UMikanComponentData* Data) override;

	/** The Sky Light that carries the recovered environment. Movable + Real Time Capture. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mikan Light Environment")
	class USkyLightComponent* SkyLightComponent;

	/** Inverted sphere the Sky Light captures. Unlit, emissive, shadow casting off. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mikan Light Environment")
	class UStaticMeshComponent* SkydomeComponent;

	/**
	 * Optional explicit key light, driven by the estimate's dominant direction.
	 * Off by default: it is only meaningful when Directionality is high, and a
	 * confidently-placed wrong key light looks worse than none.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mikan Light Environment")
	class UDirectionalLightComponent* KeyLightComponent;

	/**
	 * Parent material for the skydome. Must sample the nine SH vector
	 * parameters named SH0..SH8 against the world-space view direction and
	 * clamp the result to non-negative. See MikanLightEnvironmentActor.cpp for
	 * the exact HLSL to paste into a Custom node.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mikan Light Environment")
	class UMaterialInterface* SkydomeMaterial;

	/** Enable the explicit key light. Ignored when Directionality is below the threshold. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mikan Light Environment")
	bool bCreateKeyLight = false;

	/**
	 * Directionality (l1/l0) below which the estimate is treated as uniform
	 * ambient and the key light is suppressed. Measured office interiors land
	 * around 0.23-0.31, hard directional scenes above 1.0.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mikan Light Environment")
	float KeyLightDirectionalityThreshold = 0.25f;

	/** Extra multiplier on top of the probe's own exposure calibration. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mikan Light Environment")
	float IntensityScale = 1.f;

	/** Pushes the recovered environment into the material and lights. */
	UFUNCTION(BlueprintCallable, Category = "Mikan Light Environment")
	void UpdateLightEnvironment();

	UFUNCTION(BlueprintCallable, Category = "Mikan Light Environment")
	float GetDirectionality() const;

	inline UMikanLightEnvironmentData* GetLightEnvironmentData() const { return LightEnvironmentData; }

protected:
	virtual void OnComponentDataChanged(const FString& FieldName) override;

	void UpdateSkydomeMaterial();
	void UpdateKeyLight();

private:
	UPROPERTY(Transient)
	class UMaterialInstanceDynamic* SkydomeMaterialInstance = nullptr;

	UPROPERTY(Transient)
	UMikanLightEnvironmentData* LightEnvironmentData = nullptr;
};
