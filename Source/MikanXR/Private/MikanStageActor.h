#pragma once

#include "GameFramework/Actor.h"
#include "MikanTransformActor.h"
#include "MikanDataStore.h"
#include "MikanStageActor.generated.h"

UCLASS()
class UMikanStageData : public UMikanTransformData
{
	GENERATED_BODY()

public:
	UMikanStageData() = default;

	virtual void Initialize(const Serialization::PolymorphicObjectPtr& InValuesObject) override;
	virtual bool ApplyMikanValue(const FString& FieldName, const MikanVariant& FieldValue) override;
	virtual void Describe(TArray<FString>& OutLines) const override;

	inline int32 GetTrackingVolumeId() const { return TrackingVolumeId; }
	inline const FVector& GetStageBoundsMinMM() const { return StageBoundsMinCM; }
	inline const FVector& GetStageBoundsMaxMM() const { return StageBoundsMaxCM; }

private:
	int32 TrackingVolumeId = -1;
	FVector StageBoundsMinCM = FVector::ZeroVector;
	FVector StageBoundsMaxCM = FVector::ZeroVector;
};

UCLASS(BlueprintType, Blueprintable)
class MIKANXR_API AMikanStageActor : public AMikanTransformActor
{
	GENERATED_BODY()

public:
	AMikanStageActor(const FObjectInitializer& ObjectInitializer);

	virtual void PostRegisterAllComponents() override;
	virtual void Destroyed() override;
	virtual void Tick(float DeltaSeconds) override;

	// Script Message Events
	UFUNCTION()
	void HandleScriptMessage(const FString& Message);
	UFUNCTION(BlueprintImplementableEvent)
	void OnMikanScriptMessage(const FString& Message);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FVector StageBoundsMin = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FVector StageBoundsMax = FVector::ZeroVector;

	virtual void BindMikanComponentData(UMikanComponentData* Data) override;
	virtual void OnMikanTransformDataChanged() override;

protected:
	virtual void OnComponentDataChanged(const FString& FieldName) override;
	void OnMikanStageBoundsChanged();

#if WITH_EDITOR
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual void PostEditMove(bool bFinished) override;
#endif
};
