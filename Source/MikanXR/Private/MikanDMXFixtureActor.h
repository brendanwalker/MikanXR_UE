#pragma once

#include "GameFramework/Actor.h"
#include "MikanDataStore.h"
#include "MikanTransformActor.h"
#include "MikanDMXFixtureActor.generated.h"

UCLASS()
class UMikanDMXFixtureData : public UMikanTransformData
{
	GENERATED_BODY()

public:
	UMikanDMXFixtureData() = default;

	virtual void Initialize(const Serialization::PolymorphicObjectPtr& InValuesObject) override;
	virtual bool ApplyMikanValue(const FString& FieldName, const MikanVariant& FieldValue) override;
	virtual void Describe(TArray<FString>& OutLines) const override;

	inline int32 GetStageId() const { return StageId; }
	inline int32 GetDmxUniverse() const { return DmxUniverse; }
	inline int32 GetDmxStartChannel() const { return DmxStartChannel; }
	inline int32 GetDmxChannelCount() const { return DmxChannelCount; }
	inline bool IsDisabled() const { return bIsDisabled; }
	inline float GetMaxWattage() const { return MaxWattage; }
	inline float GetLumensPerWatt() const { return LumensPerWatt; }

private:
	int32 StageId = -1;
	int32 DmxUniverse = 1;
	int32 DmxStartChannel = 1;
	int32 DmxChannelCount = 3;
	bool bIsDisabled = false;
	float MaxWattage = 0.3f;
	float LumensPerWatt = 83.f;
};

UCLASS(BlueprintType)
class AMikanDMXFixtureActor : public AMikanTransformActor
{
	GENERATED_BODY()

public:
	AMikanDMXFixtureActor(const FObjectInitializer& ObjectInitializer);

	virtual void BindMikanComponentData(class UMikanComponentData* Data) override;
	virtual void Destroyed() override;

	UFUNCTION()
	virtual void OnDMXDataChanged() {}
};
