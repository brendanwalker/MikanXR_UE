#pragma once

#include "MikanTransformActor.h"
#include "MikanDMXFixtureActor.h"
#include "MikanLightTypes.h"
#include "MikanRGBPixelGridActor.generated.h"

UCLASS()
class UMikanRGBPixelGridData : public UMikanDMXFixtureData
{
	GENERATED_BODY()

public:
	UMikanRGBPixelGridData() = default;

	virtual void Initialize(const Serialization::PolymorphicObjectPtr& InValuesObject) override;
	virtual bool ApplyMikanValue(const FString& FieldName, const MikanVariant& FieldValue) override;
	virtual void Describe(TArray<FString>& OutLines) const override;

	inline int32 GetGridColumns() const { return GridColumns; }
	inline int32 GetGridRows() const { return GridRows; }

private:
	int32 GridColumns = 8;
	int32 GridRows = 8;
};

UCLASS(BlueprintType, Blueprintable)
class AMikanRGBPixelGridActor : public AMikanDMXFixtureActor
{
	GENERATED_BODY()

public:
	AMikanRGBPixelGridActor(const FObjectInitializer& ObjectInitializer);

	virtual void Tick(float DeltaSeconds) override;
	virtual void BindMikanComponentData(UMikanComponentData* Data) override;

private:
	int32 GridColumns = 8;
	int32 GridRows = 8;
};
