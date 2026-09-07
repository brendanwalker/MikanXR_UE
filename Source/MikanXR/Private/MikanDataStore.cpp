#include "MikanDataStore.h"
#include "MikanComponentTypes.h"
#include "MikanEngineSubsystem.h"
#include "MikanPropertyEvents.h"
#include "MikanPropertyRequests.h"
#include "MikanTransformTypes.h"
#include "MikanMath.h"
#include "IMikanXRModule.h"

#include "MikanAnchorTypes.h"
#include "MikanAnchorActor.h"
#include "MikanShapeTypes.h"
#include "MikanShapeActor.h"
#include "MikanQuadShapeActor.h"
#include "MikanBoxShapeActor.h"
#include "MikanModelShapeActor.h"
#include "MikanStencilTypes.h"
#include "MikanStencilActor.h"
#include "MikanQuadStencilActor.h"
#include "MikanBoxStencilActor.h"
#include "MikanModelStencilActor.h"
#include "MikanCameraTypes.h"
#include "MikanCameraActor.h"
#include "MikanLightTypes.h"
#include "MikanLightEnvironmentActor.h"
#include "MikanRGBSpotLightActor.h"
#include "MikanRGBPixelGridActor.h"
#include "MikanStageTypes.h"
#include "MikanStageActor.h"
#include "MikanSceneTypes.h"
#include "MikanSceneActor.h"

UE_DISABLE_OPTIMIZATION
namespace Mikan
{
	void ToUnrealString(const Serialization::String& MikanString, FString& OutUnrealString)
	{
		const char* MikanStringValue = MikanString.getUtf8Value();
		const auto UnrealStringTChar = StringCast<TCHAR>(MikanStringValue);

		OutUnrealString = FString(UnrealStringTChar.Get());
	}

	void ToUnrealString(const MikanVariant& InVariant, FString& OutUnrealString)
	{
		const char* MikanStringValue = InVariant.getUtf8Value();
		const auto UnrealStringTChar = StringCast<TCHAR>(MikanStringValue);

		OutUnrealString = FString(UnrealStringTChar.Get());
	}

	void AppendDescribeLine(TArray<FString>& OutLines, const TCHAR* FieldName, const FString& Value)
	{
		OutLines.Add(FString::Printf(TEXT("%s %s"), FieldName, *Value));
	}

	void AppendDescribeLine(TArray<FString>& OutLines, const TCHAR* FieldName, int32 Value)
	{
		OutLines.Add(FString::Printf(TEXT("%s %d"), FieldName, Value));
	}

	void AppendDescribeLine(TArray<FString>& OutLines, const TCHAR* FieldName, float Value)
	{
		OutLines.Add(FString::Printf(TEXT("%s %g"), FieldName, Value));
	}

	void AppendDescribeLine(TArray<FString>& OutLines, const TCHAR* FieldName, bool Value)
	{
		OutLines.Add(FString::Printf(TEXT("%s %s"), FieldName, Value ? TEXT("true") : TEXT("false")));
	}

	void AppendDescribeLine(TArray<FString>& OutLines, const TCHAR* FieldName, const FVector& Value)
	{
		OutLines.Add(FString::Printf(TEXT("%s %g %g %g"), FieldName, Value.X, Value.Y, Value.Z));
	}

	void AppendDescribeLine(TArray<FString>& OutLines, const TCHAR* FieldName, const FQuat& Value)
	{
		OutLines.Add(FString::Printf(TEXT("%s %g %g %g %g"), FieldName, Value.W, Value.X, Value.Y, Value.Z));
	}
}

// -- UMikanComponentData -----
UMikanComponentSystem* UMikanComponentData::GetOwnerSystem() const
{
	return CastChecked<UMikanComponentSystem>(GetOuter());
}

void UMikanComponentData::Initialize(
	const Serialization::PolymorphicObjectPtr& InValuesObject)
{
	const auto* ComponentValues= InValuesObject.getTypedPointer<MikanComponentValues>();

	ComponentId = ComponentValues->component_id;
	Mikan::ToUnrealString(ComponentValues->component_name, ComponentName);
}

bool UMikanComponentData::ApplyMikanValue(const FString& FieldName, const MikanVariant& FieldValue)
{
	if (FieldName == "component_id")
	{
		ComponentId = FieldValue.getIntValue();
		return true;
	}
	else if (FieldName == "component_name")
	{
		Mikan::ToUnrealString(FieldValue, ComponentName);
		return true;
	}

	return false;
}

void UMikanComponentData::Describe(TArray<FString>& OutLines) const
{
	Mikan::AppendDescribeLine(OutLines, TEXT("component_id"), (int32)ComponentId);
	Mikan::AppendDescribeLine(OutLines, TEXT("component_name"), ComponentName);
	Mikan::AppendDescribeLine(OutLines, TEXT("owner_system"), GetOwnerSystem()->GetSystemName());
	Mikan::AppendDescribeLine(OutLines, TEXT("component_class"), GetOwnerSystem()->GetComponentClassName());
}

// -- UMikanComponentSystem ------
void UMikanComponentSystem::Initialize(
	const char* InSystemName, 
	const char* InComponentClassName,
	ComponentDataFactory Factory)
{
	szSystemName = InSystemName;
	szComponentClassName = InComponentClassName;
	SystemName = FString(InSystemName);
	ComponentClassName = FString(InComponentClassName);
	DataObjectFactory = Factory;
}

UMikanDataStore* UMikanComponentSystem::GetOwnerDataStore() const 
{ 
	return CastChecked<UMikanDataStore>(GetOuter()); 
}

void UMikanComponentSystem::FetchAllComponents(TFunction<void(bool bSuccess)> OnComplete)
{
	UMikanEngineSubsystem* Subsystem = UMikanEngineSubsystem::Get();
	if (!Subsystem)
	{
		if (OnComplete) { OnComplete(false); }
		return;
	}

	GetComponentListRequest ListRequest;
	ListRequest.ownerSystem.setUtf8Value(szSystemName);
	ListRequest.componentClassName.setUtf8Value(szComponentClassName);

	// Step 1: fetch the component id list. The value fetches are fired from the list response
	// callback (below), all on the game thread, so no locking is needed on the shared counter.
	TWeakObjectPtr<UMikanComponentSystem> WeakThis(this);
	Subsystem->SendRequestAsync(
		ListRequest,
		[WeakThis, OnComplete](const MikanResponsePtr& ListResponse)
		{
			UMikanComponentSystem* Self = WeakThis.Get();
			if (!Self)
			{
				if (OnComplete) { OnComplete(false); }
				return;
			}

			if (ListResponse->resultCode != MikanAPIResult::Success)
			{
				UE_LOG(MikanXRLog, Error,
					TEXT("Failed to fetch Components of class %s from System %s (Error Code: %d)"),
					*Self->ComponentClassName, *Self->SystemName, ListResponse->resultCode);
				if (OnComplete) { OnComplete(false); }
				return;
			}

			UMikanEngineSubsystem* InnerSubsystem = UMikanEngineSubsystem::Get();
			if (!InnerSubsystem)
			{
				if (OnComplete) { OnComplete(false); }
				return;
			}

			auto ComponentList = std::static_pointer_cast<ComponentListResponse>(ListResponse);
			const auto& NewIdList = ComponentList->componentIdList;

			UE_LOG(MikanXRLog, Log,
				TEXT("Fetch Components of class %s from System %s"),
				*Self->ComponentClassName, *Self->SystemName);

			// Drop components that are no longer present in the fetched list.
			TArray<int32> ExistingIDs;
			Self->ComponentDataTable.GetKeys(ExistingIDs);
			for (int32 ExistingID : ExistingIDs)
			{
				if (std::find(NewIdList.begin(), NewIdList.end(), ExistingID) == NewIdList.end())
				{
					UE_LOG(MikanXRLog, Log,
						TEXT("Removed ComponentID %d from System %s"),
						ExistingID, *Self->SystemName);

					Self->ComponentDataTable.Remove(ExistingID);
				}
			}
			Self->ComponentDataTable.Compact();

			// Nothing to fetch values for: broadcast the (possibly emptied) list and finish.
			if (NewIdList.empty())
			{
				Self->GetOwnerDataStore()->OnComponentListChanged.Broadcast(Self);
				if (OnComplete) { OnComplete(true); }
				return;
			}

			// Step 2: fire every value request up front (SendRequestAsync is non-blocking) so the
			// Mikan server can answer them all in one tick, then apply each as its response arrives.
			// A shared counter, decremented on the game thread by each response, drives completion.
			TSharedRef<int32> Remaining = MakeShared<int32>((int32)NewIdList.size());
			TSharedRef<bool> bAnyValueFailed = MakeShared<bool>(false);

			for (int32 ComponentId : NewIdList)
			{
				ComponentGetValuesRequest ValuesRequest;
				ValuesRequest.ownerSystem.setUtf8Value(Self->szSystemName);
				ValuesRequest.componentId = ComponentId;

				InnerSubsystem->SendRequestAsync(
					ValuesRequest,
					[WeakThis, OnComplete, Remaining, bAnyValueFailed, ComponentId](const MikanResponsePtr& ValuesResponse)
					{
						UMikanComponentSystem* System = WeakThis.Get();
						if (System)
						{
							if (ValuesResponse->resultCode == MikanAPIResult::Success)
							{
								auto ComponentValuesResponse =
									std::static_pointer_cast<ComponentGetValuesResponse>(ValuesResponse);

								UE_LOG(MikanXRLog, Log,
									TEXT("Added new ComponentID %d from System %s"),
									ComponentId, *System->SystemName);

								UMikanComponentData* NewComponentData = System->DataObjectFactory(System);
								check(NewComponentData);
								NewComponentData->Initialize(ComponentValuesResponse->valuesObject);

								System->ComponentDataTable.Emplace(ComponentId, NewComponentData);
							}
							else
							{
								*bAnyValueFailed = true;

								UE_LOG(MikanXRLog, Error,
									TEXT("Failed to add new ComponentID %d from System %s (ErrorCode %d)"),
									ComponentId, *System->SystemName, ValuesResponse->resultCode);
							}
						}

						// Last value response for this system: broadcast list-changed, then complete.
						if (--(*Remaining) == 0)
						{
							if (System)
							{
								System->GetOwnerDataStore()->OnComponentListChanged.Broadcast(System);
							}
							if (OnComplete) { OnComplete(!*bAnyValueFailed); }
						}
					});
			}
		});
}

void UMikanComponentSystem::FlushAllComponents()
{
	ComponentDataTable.Reset();
}

void UMikanComponentSystem::HandleMikanConnected()
{
	FetchAllComponents();
}

void UMikanComponentSystem::HandleMikanDisconnected()
{
	FlushAllComponents();
}

void UMikanComponentSystem::HandleComponentListChanged()
{
	FetchAllComponents();
}

UMikanComponentData* UMikanComponentSystem::FindComponentDataById(int32 ComponentId)
{
	if (UMikanComponentData** ComponentDataPtr = ComponentDataTable.Find(ComponentId))
	{
		return *ComponentDataPtr;
	}
	return nullptr;
}

UMikanComponentData* UMikanComponentSystem::FindComponentDataByName(const FString& ComponentName)
{
	for (const auto& Pair : ComponentDataTable)
	{
		UMikanComponentData* ComponentData = Pair.Value;

		if (ComponentData->GetComponentName() == ComponentName)
		{
			return ComponentData;
		}
	}

	return nullptr;
}

void UMikanComponentSystem::ApplyMikanValue(
	MikanComponentID ComponentId,
	const FString& FieldName,
	const MikanVariant& FieldValue)
{
	if (UMikanComponentData** ComponentDataPtr = ComponentDataTable.Find(ComponentId))
	{
		UMikanComponentData* ComponentData = *ComponentDataPtr;

		// Try and apply the new value to the component data.
		if (ComponentData->ApplyMikanValue(FieldName, FieldValue))
		{
			//UE_LOG(MikanXRLog, Log,
			//	TEXT("Applied update to Field %s on Component ID %d in System %s"),
			//	*FieldName, ComponentId, *SystemName);

			// Notify subscribers that component data has changed
			ComponentData->OnComponentDataChanged.Broadcast(FieldName);
		}
	}
}

// -- UMikanDataStore -----
void UMikanDataStore::Initialize(IMikanAPI* InMikanAPI)
{
	MikanAPI = InMikanAPI;

	// Anchor
	AddTypedComponentSystem<MikanAnchorComponentValues, UMikanAnchorData>();
	// Shapes
	AddTypedComponentSystem<MikanQuadShapeComponentValues, UMikanQuadShapeData>();
	AddTypedComponentSystem<MikanBoxShapeComponentValues, UMikanBoxShapeData>();
	AddTypedComponentSystem<MikanModelShapeComponentValues, UMikanModelShapeData>();
	// Stencils
	AddTypedComponentSystem<MikanQuadStencilComponentValues, UMikanQuadStencilData>();
	AddTypedComponentSystem<MikanBoxStencilComponentValues, UMikanBoxStencilData>();
	AddTypedComponentSystem<MikanModelStencilComponentValues, UMikanModelStencilData>();
	// Camera
	AddTypedComponentSystem<MikanCameraComponentValues, UMikanCameraData>();
	// Lights
	AddTypedComponentSystem<MikanRGBSpotLightComponentValues, UMikanRGBSpotLightData>();
	AddTypedComponentSystem<MikanRGBPixelGridComponentValues, UMikanRGBPixelGridData>();
	AddTypedComponentSystem<MikanLightEnvironmentComponentValues, UMikanLightEnvironmentData>();
	// Stage & Scene
	AddTypedComponentSystem<MikanStageComponentValues, UMikanStageData>();
	AddTypedComponentSystem<MikanSceneComponentValues, UMikanSceneData>();
}

void UMikanDataStore::AddComponentSystem(
	const char* OwnerSystemName,
	const char* ComponentClassName,
	ComponentDataFactory Factory)
{
	auto* ComponentSystem= NewObject<UMikanComponentSystem>(this);
	ComponentSystem->Initialize(OwnerSystemName, ComponentClassName, Factory);

	SystemsTable.Emplace(OwnerSystemName, ComponentSystem);
}

UMikanComponentSystem* UMikanDataStore::GetComponentSystem(const char* SystemName)
{
	FString Key(SystemName);
	if (UMikanComponentSystem** SystemPtr = SystemsTable.Find(Key))
	{
		return *SystemPtr;
	}

	return nullptr;
}

UMikanComponentData* UMikanDataStore::GetComponentData(int32 ComponentId)
{
	for (const auto& Pair : SystemsTable)
	{
		UMikanComponentSystem* System = Pair.Value;

		if (System)
		{
			UMikanComponentData* ComponentData = System->FindComponentDataById(ComponentId);

			if (ComponentData)
			{
				return ComponentData;
			}
		}
	}

	return nullptr;
}

UMikanComponentData* UMikanDataStore::GetComponentData(const char* SystemName, int32 ComponentId)
{
	UMikanComponentSystem* System = GetComponentSystem(SystemName);
	if (System)
	{
		return System->FindComponentDataById(ComponentId);
	}

	return nullptr;
}

void UMikanDataStore::FetchAllComponents(TFunction<void(bool bSuccess)> OnComplete)
{
	// Mark a batch fetch so per-system OnComponentListChanged subscribers defer cross-system work
	// (like actor attachment refresh) until the whole store is consistent. Unlike the old blocking
	// path this stays true across several frames; it is cleared once the last system finishes.
	bIsBatchFetching = true;

	int32 SystemCount = 0;
	for (auto& Pair : SystemsTable)
	{
		if (Pair.Value)
		{
			++SystemCount;
		}
	}

	if (SystemCount == 0)
	{
		bIsBatchFetching = false;
		OnComponentsBatchRefreshed.Broadcast();
		if (OnComplete) { OnComplete(true); }
		return;
	}

	// Shared completion counter, decremented on the game thread as each system finishes.
	TSharedRef<int32> Remaining = MakeShared<int32>(SystemCount);
	TSharedRef<bool> bAnySystemFailed = MakeShared<bool>(false);
	TWeakObjectPtr<UMikanDataStore> WeakThis(this);

	for (auto& Pair : SystemsTable)
	{
		UMikanComponentSystem* ComponentSystem= Pair.Value;
		if (!ComponentSystem)
		{
			continue;
		}

		ComponentSystem->FetchAllComponents(
			[WeakThis, Remaining, bAnySystemFailed, OnComplete](bool bSystemSucceeded)
			{
				if (!bSystemSucceeded)
				{
					*bAnySystemFailed = true;
				}

				if (--(*Remaining) == 0)
				{
					if (UMikanDataStore* Self = WeakThis.Get())
					{
						// Every system is rebuilt: let subscribers refresh cross-system state once.
						Self->bIsBatchFetching = false;
						Self->OnComponentsBatchRefreshed.Broadcast();
					}
					if (OnComplete) { OnComplete(!*bAnySystemFailed); }
				}
			});
	}
}

void UMikanDataStore::FlushAllComponents()
{
	// An in-flight batch fetch (if any) has been abandoned; make sure the flag doesn't stay stuck
	// true when its per-system completions never arrive (e.g. flushed on disconnect).
	bIsBatchFetching = false;

	for (auto& Pair : SystemsTable)
	{
		UMikanComponentSystem* ComponentSystem = Pair.Value;

		if (ComponentSystem)
		{
			ComponentSystem->FlushAllComponents();
		}
	}
}

void UMikanDataStore::HandleMikanConnected()
{
	FetchAllComponents();
}

void UMikanDataStore::HandleMikanDisconnected()
{
	FlushAllComponents();
}

void UMikanDataStore::HandleListChanged(const FString& SystemName)
{
	if (UMikanComponentSystem** SystemPtr = SystemsTable.Find(SystemName))
	{
		(*SystemPtr)->HandleComponentListChanged();
	}
}

void UMikanDataStore::HandlePropertyUpdateEvent(
	const MikanPropertyUpdateEvent& PropertyUpdateEvent)
{
	FString SystemName;
	Mikan::ToUnrealString(PropertyUpdateEvent.propertyValue.ownerSystem, SystemName);
	if (UMikanComponentSystem** ComponentSystemPtr = SystemsTable.Find(SystemName))
	{
		UMikanComponentSystem* ComponentSystem = *ComponentSystemPtr;

		FString FieldName;
		Mikan::ToUnrealString(PropertyUpdateEvent.propertyValue.fieldName, FieldName);
		MikanComponentID ComponentId = PropertyUpdateEvent.propertyValue.componentId;
		const MikanVariant& FieldValue = PropertyUpdateEvent.propertyValue.fieldValue;

		ComponentSystem->ApplyMikanValue(ComponentId, FieldName, FieldValue);
	}
	else
	{
		UE_LOG(MikanXRLog, Error,
			TEXT("Failed to update property. Unknown System %s"),
			*SystemName);
	}
}

UE_ENABLE_OPTIMIZATION