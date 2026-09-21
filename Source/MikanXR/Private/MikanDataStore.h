#pragma once

#include "CoreMinimal.h"
#include "MikanAPI.h"
#include "SerializableObjectPtr.h"
#include "SerializableString.h"
#include "MikanPropertyTypes.h"

#include "MikanDataStore.generated.h"

DECLARE_MULTICAST_DELEGATE_OneParam(
	FOnMikanComponentDataChanged,
	const FString& /*FieldName*/)

DECLARE_MULTICAST_DELEGATE_OneParam(
	FOnMikanComponentListChanged,
	const UMikanComponentSystem* /*System*/)

DECLARE_MULTICAST_DELEGATE_TwoParams(
	FOnMikanSystemDataChanged,
	const UMikanComponentSystem* /*System*/,
	const FString& /*FieldName*/)

DECLARE_MULTICAST_DELEGATE(FOnMikanComponentsBatchRefreshed)

namespace Mikan
{;
	void ToUnrealString(const Serialization::String& MikanString, FString& OutUnrealString);
	void ToUnrealString(const MikanVariant& InVariant, FString& OutUnrealString);

	// "<field> <value>" line builders for UMikanComponentData::Describe. Vectors print as
	// "x y z", quaternions as "w x y z", matching the Mikan automation value syntax.
	void AppendDescribeLine(TArray<FString>& OutLines, const TCHAR* FieldName, const FString& Value);
	void AppendDescribeLine(TArray<FString>& OutLines, const TCHAR* FieldName, int32 Value);
	void AppendDescribeLine(TArray<FString>& OutLines, const TCHAR* FieldName, float Value);
	void AppendDescribeLine(TArray<FString>& OutLines, const TCHAR* FieldName, bool Value);
	void AppendDescribeLine(TArray<FString>& OutLines, const TCHAR* FieldName, const FVector& Value);
	void AppendDescribeLine(TArray<FString>& OutLines, const TCHAR* FieldName, const FQuat& Value);
}

UCLASS()
class UMikanComponentData : public UObject
{
	GENERATED_BODY()

public:
	UMikanComponentData()= default;

	virtual void Initialize(const Serialization::PolymorphicObjectPtr& InValuesObject);
	virtual bool ApplyMikanValue(const FString& FieldName, const MikanVariant& FieldValue);

	UMikanComponentSystem* GetOwnerSystem() const;
	MikanComponentID GetComponentId() const { return ComponentId; }
	const FString& GetComponentName() const { return ComponentName; }

	// Appends one "<field> <value>" line per cached field, keyed by the Mikan property name the
	// value arrived under in ApplyMikanValue. Overrides call Super first.
	virtual void Describe(TArray<FString>& OutLines) const;

	// Fires after a single component property is updated in ApplyMikanValue.
	FOnMikanComponentDataChanged OnComponentDataChanged;

private:
	MikanComponentID ComponentId= -1;
	FString ComponentName;
};

// Values that belong to an object system rather than to any one component. Mikan sends these
// with a component id of -1, so they have no entry in the component table to land in.
UCLASS()
class UMikanSystemData : public UObject
{
	GENERATED_BODY()

public:
	UMikanSystemData()= default;

	virtual void Initialize(const Serialization::PolymorphicObjectPtr& InValuesObject) {}
	virtual bool ApplyMikanValue(const FString& FieldName, const MikanVariant& FieldValue) { return false; }
	virtual void Describe(TArray<FString>& OutLines) const {}
};

UCLASS()
class UMikanComponentSystem : public UObject
{
	GENERATED_BODY()

public:
	using ComponentDataFactory=	TFunction<UMikanComponentData* (UMikanComponentSystem*)>;
	using SystemDataFactory= TFunction<UMikanSystemData* (UMikanComponentSystem*)>;

	UMikanComponentSystem()= default;

	void Initialize(
		const char* InSystemName,
		const char* InComponentClassName,
		ComponentDataFactory Factory);

	// Opts this system into system-level values. Without a factory the system ignores them,
	// which is what every system but the scene system does today.
	void SetSystemDataFactory(SystemDataFactory Factory);
	inline UMikanSystemData* GetSystemData() const { return SystemData; }

	template<typename T>
	T* GetTypedSystemData() const { return Cast<T>(SystemData); }

	// Asynchronously fetch this system's own values. No-op without a system data factory.
	// Runs beside FetchAllComponents rather than inside its completion count, so subscribers
	// treat the system values and the component tables as two independent arrivals.
	void FetchSystemValues();

	// Asynchronously rebuild this system's component table. OnComplete runs on the game thread once
	// the list + every component's values have been fetched (or immediately if there's no API), and
	// reports whether every request was answered. A false result means the table is incomplete.
	void FetchAllComponents(TFunction<void(bool bSuccess)> OnComplete = nullptr);
	void FlushAllComponents();
	void HandleMikanConnected();
	void HandleMikanDisconnected();
	void HandleComponentListChanged();
	void ApplyMikanValue(
		MikanComponentID ComponentId,
		const FString& FieldName,
		const MikanVariant& FieldValue);

	UMikanComponentData* FindComponentDataById(int32 ComponentId);
	UMikanComponentData* FindComponentDataByName(const FString& ComponentName);

	class UMikanDataStore* GetOwnerDataStore() const;
	inline const FString& GetSystemName() const { return SystemName; }
	inline const FString& GetComponentClassName() const { return ComponentClassName; }
	inline const TMap<int32, UMikanComponentData*>& GetComponentDataTable() const { return ComponentDataTable; }

protected:
	UMikanDataStore* OwnerDataStore= nullptr;
	const char* szSystemName = nullptr;
	const char* szComponentClassName = nullptr;
	FString SystemName;
	FString ComponentClassName;
	ComponentDataFactory DataObjectFactory;
	SystemDataFactory SystemDataObjectFactory;

	UPROPERTY(Transient)
	TMap<int32, UMikanComponentData*> ComponentDataTable;

	UPROPERTY(Transient)
	UMikanSystemData* SystemData= nullptr;
};

UCLASS()
class UMikanDataStore : public UObject
{
	GENERATED_BODY()

public:
	using ComponentDataFactory = UMikanComponentSystem::ComponentDataFactory;
	using SystemDataFactory = UMikanComponentSystem::SystemDataFactory;

	UMikanDataStore()= default;

	void Initialize(IMikanAPI* InMikanAPI);
	inline IMikanAPI* GetMikanAPI() const { return MikanAPI; }

	template<typename MikanDataType, typename UnrealDataType>
	void AddTypedComponentSystem()
	{
		AddComponentSystem(
			MikanDataType::k_ownerSystemName,
			MikanDataType::k_componentClassName,
			[](UMikanComponentSystem* OwnerSystem) { return NewObject<UnrealDataType>(OwnerSystem); });
	}
	void AddComponentSystem(
		const char* OwnerSystemName,
		const char* ComponentClassName,
		ComponentDataFactory Factory);

	// Gives an already-registered system a place to keep its system-level values.
	template<typename MikanSystemValuesType, typename UnrealSystemDataType>
	void AddTypedSystemData()
	{
		AddSystemData(
			MikanSystemValuesType::k_systemName,
			[](UMikanComponentSystem* OwnerSystem) { return NewObject<UnrealSystemDataType>(OwnerSystem); });
	}
	void AddSystemData(const char* OwnerSystemName, SystemDataFactory Factory);

	inline const TMap<FString, UMikanComponentSystem*>& GetSystemsTable() const { return SystemsTable; }

	UMikanComponentSystem* GetComponentSystem(const char* SystemName);

	UMikanComponentData* GetComponentData(int32 ComponentId);
	UMikanComponentData* GetComponentData(const char* SystemName, int32 ComponentId);

	template<typename T>
	T* GetTypedComponentData(const char* SystemName, int32 ComponentId)
	{
		return Cast<T>(GetComponentData(SystemName, ComponentId));
	}

	// Asynchronously rebuild every system's component table. bIsBatchFetching stays true across the
	// (multi-frame) fetch; OnComponentsBatchRefreshed and OnComplete fire once all systems finish.
	// OnComplete reports false when any system's fetch went unanswered, leaving the store partial.
	void FetchAllComponents(TFunction<void(bool bSuccess)> OnComplete = nullptr);
	void FlushAllComponents();
	void HandleMikanConnected();
	void HandleMikanDisconnected();
	void HandleListChanged(const FString& SystemName);
	void HandlePropertyUpdateEvent(const struct MikanPropertyUpdateEvent& PropertyUpdateEvent);

	// Fires after the component list for a system is rebuilt in the DataStore.
	FOnMikanComponentListChanged OnComponentListChanged;

	// Fires when a system's own values arrive or change. The field name is empty for the
	// initial fetch, which delivers every field at once.
	FOnMikanSystemDataChanged OnSystemDataChanged;

	// Fires once after FetchAllComponents has rebuilt every system, so subscribers can
	// refresh cross-system state (e.g. actor attachments) against a fully consistent store.
	FOnMikanComponentsBatchRefreshed OnComponentsBatchRefreshed;

	// True while FetchAllComponents is rebuilding every system. Subscribers should defer
	// cross-system work until OnComponentsBatchRefreshed fires rather than reacting to each
	// per-system OnComponentListChanged broadcast against partially-synced state.
	inline bool IsBatchFetching() const { return bIsBatchFetching; }

protected:
	IMikanAPI* MikanAPI= nullptr;

	bool bIsBatchFetching= false;

	UPROPERTY(Transient)
	TMap<FString, UMikanComponentSystem*> SystemsTable;
};
