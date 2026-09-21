#pragma once

#include "MikanTransformActor.h"
#include "MikanDMXFixtureActor.h"
#include "MikanLightTypes.h"
#include "MikanRGBSpotLightActor.generated.h"

UCLASS()
class UMikanRGBSpotLightData : public UMikanDMXFixtureData
{
	GENERATED_BODY()

public:
	UMikanRGBSpotLightData() = default;

	virtual void Initialize(const Serialization::PolymorphicObjectPtr& InValuesObject) override;
	virtual bool ApplyMikanValue(const FString& FieldName, const MikanVariant& FieldValue) override;
	virtual void Describe(TArray<FString>& OutLines) const override;

	inline float GetConeAngleDegrees() const { return ConeAngleDegrees; }
	inline float GetConeRangeMeters() const { return ConeRangeMeters; }

private:
	float ConeAngleDegrees= 45.f;
	float ConeRangeMeters= 1.f;
};

UCLASS(BlueprintType, Blueprintable)
class AMikanRGBSpotLightActor : public AMikanDMXFixtureActor
{
	GENERATED_BODY()

public:
	AMikanRGBSpotLightActor(const FObjectInitializer& ObjectInitializer);

	// The emitter spec Mikan sends for this fixture wins once its component data binds.
	// These are what the actor uses before that, and if it is placed without a Mikan fixture.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DMX Light Fixture")
	float MaxWattage = 0.3f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DMX Light Fixture")
	float LumensPerWatt= 83.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class USceneComponent* LightForward;

#if WITH_EDITORONLY_DATA
	// Reference to editor arrow component visualization 
private:
	UPROPERTY()
	TObjectPtr<class UArrowComponent> ArrowComponent;
public:
#endif

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<class USpotLightComponent> SpotLightComponent;

	// FUNCTIONS---------------------------------
#if WITH_EDITOR
	//~ Begin AActor Interface.
	virtual void EditorApplyScale(const FVector& DeltaScale, const FVector* PivotLocation, bool bAltDown, bool bShiftDown, bool bCtrlDown) override;
	//~ End AActor Interface.
#endif

	/** 
	 * Updates the spotlight intensity. 
	 * Considers light intensity max, the spotlight intensity scale and the cone angle to compute the intensity. 
	 * Should be used instead of setting the intensity of the spotlight directly.
	 */
	UFUNCTION(BlueprintCallable, Category = "DMX Fixture")
	void UpdateSpotLightIntensity();

	UFUNCTION(BlueprintCallable, Category = "DMX Fixture")
	void UpdateVisibility();

	UFUNCTION(BlueprintCallable, Category = "DMX Fixture")
	void UpdateConeAngle();

	UFUNCTION(BlueprintCallable, Category = "DMX Fixture")
	void UpdateConeRange();

	UFUNCTION(BlueprintCallable, Category = "Rendering|Lighting")
	float GetBrightness() const;

	UFUNCTION(BlueprintCallable, Category = "Rendering|Lighting")
	FLinearColor GetLightColor() const;

#if WITH_EDITORONLY_DATA
	/** Returns ArrowComponent subobject **/
	class UArrowComponent* GetArrowComponent() const { return ArrowComponent; }
#endif

	virtual void BindMikanComponentData(class UMikanComponentData* Data) override;

protected:
	virtual void OnComponentDataChanged(const FString& FieldName) override;
	virtual void OnDMXDataChanged() override;
	void UpdateRGBFractions();

protected:
	float DMXRedFraction = 0.f;
	float DMXGreenFraction = 0.f;
	float DMXBlueFraction = 0.f;
};
