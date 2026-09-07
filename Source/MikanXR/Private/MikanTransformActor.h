#pragma once

#include "GameFramework/Actor.h"
#include "MikanDataStore.h"
#include "MikanTransformActor.generated.h"

UCLASS()
class UMikanTransformData : public UMikanComponentData
{
	GENERATED_BODY()

public:
	UMikanTransformData()= default;

	virtual void Initialize(const Serialization::PolymorphicObjectPtr& InValuesObject) override;
	virtual bool ApplyMikanValue(const FString& FieldName, const MikanVariant& FieldValue) override;
	virtual void Describe(TArray<FString>& OutLines) const override;

	inline MikanTransformID GetParentTransformId() const { return ParentTransformId; }
	inline const FVector& GetRelativePosition() const { return RelativePosition; }
	inline const FQuat& GetRelativeRotation() const { return RelativeRotation; }
	inline const FVector& GetRelativeScale() const { return RelativeScale; }

	FTransform ComputeRelativeTransform(UWorld* World) const;

private:
	MikanTransformID ParentTransformId= -1;
	FVector RelativePosition;
	FQuat RelativeRotation;
	FVector RelativeScale;
};

UCLASS(BlueprintType, Blueprintable)
class MIKANXR_API AMikanTransformActor : public AActor
{
	GENERATED_BODY()

public:
	AMikanTransformActor(const FObjectInitializer& ObjectInitializer);

	virtual void Destroyed() override;

	inline class AMikanClient* GetOwnerMikanClient() const;
	inline FString GetTransformName() const { return TransformData ? TransformData->GetComponentName() : FString(); }
	inline MikanTransformID GetTransformId() const { return TransformData ? TransformData->GetComponentId() : -1; }
	inline UMikanTransformData* GetTransformData() const { return TransformData; }
	inline AMikanTransformActor* GetParentTransformActor() const { return ParentTransformActor; }

	virtual void BindMikanComponentData(class UMikanComponentData* Data);
	virtual void UnbindMikanComponentData(class UMikanComponentData* Data);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class USceneComponent* MikanTransform;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class UTextRenderComponent* LabelComponent;

	virtual void OnMikanComponentNameChanged();
	virtual void OnMikanTransformDataChanged();
	virtual void OnMikanAttachmentChanged();

#if WITH_EDITOR
	virtual bool ShouldTickIfViewportsOnly() const override;
	virtual void PostEditMove(bool bFinished) override;
#endif

protected:
	UFUNCTION()
	virtual void OnComponentDataChanged(const FString& FieldName);

private:
	UPROPERTY(Transient)
	AMikanTransformActor* ParentTransformActor = nullptr;

	UPROPERTY(Transient)
	UMikanTransformData* TransformData = nullptr;
};
