#pragma once

#include "MikanTransformActor.h"
#include "MikanShapeActor.h"
#include "MikanShapeTypes.h"
#include "MikanBoxShapeActor.generated.h"

UCLASS()
class UMikanBoxShapeData : public UMikanShapeData
{
	GENERATED_BODY()

public:
	UMikanBoxShapeData() = default;

	virtual void Initialize(const Serialization::PolymorphicObjectPtr& InValuesObject) override;
	virtual bool ApplyMikanValue(const FString& FieldName, const MikanVariant& FieldValue) override;
	virtual void Describe(TArray<FString>& OutLines) const override;

	inline float GetBoxXSize() const { return BoxXSize; }
	inline float GetBoxYSize() const { return BoxYSize; }
	inline float GetBoxZSize() const { return BoxZSize; }

private:
	float BoxXSize = 0.f;
	float BoxYSize = 0.f;
	float BoxZSize = 0.f;
};

UCLASS(BlueprintType, Blueprintable)
class AMikanBoxShapeActor : public AMikanShapeActor
{
	GENERATED_BODY()

public:
	AMikanBoxShapeActor(const FObjectInitializer& ObjectInitializer);

	virtual void Tick(float DeltaSeconds) override;
	virtual void BindMikanComponentData(class UMikanComponentData* Data) override;

	UFUNCTION(BlueprintPure)
	FVector GetExtents() const { return Extents; }

private:
	FVector Extents;
};
