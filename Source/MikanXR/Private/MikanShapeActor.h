#pragma once

#include "GameFramework/Actor.h"
#include "MikanDataStore.h"
#include "MikanTransformActor.h"
#include "MikanShapeActor.generated.h"

UCLASS()
class UMikanShapeData : public UMikanTransformData
{
	GENERATED_BODY()

public:
	UMikanShapeData() = default;

	virtual void Initialize(const Serialization::PolymorphicObjectPtr& InValuesObject) override;
	virtual bool ApplyMikanValue(const FString& FieldName, const MikanVariant& FieldValue) override;
	virtual void Describe(TArray<FString>& OutLines) const override;

	inline const FString& GetShapeGraphPath() const { return ShapeGraphPath; }

private:
	FString ShapeGraphPath;
};

UCLASS(BlueprintType)
class AMikanShapeActor : public AMikanTransformActor
{
	GENERATED_BODY()

public:
	AMikanShapeActor(const FObjectInitializer& ObjectInitializer);
};
