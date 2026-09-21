#include "MikanDMXFixtureActor.h"
#include "MikanClient.h"
#include "MikanLightTypes.h"
#include "MikanTransformTypes.h"
#include "MikanSceneActor.h"

// -- UMikanDMXFixtureData -----
void UMikanDMXFixtureData::Initialize(const Serialization::PolymorphicObjectPtr& InValuesObject)
{
	UMikanTransformData::Initialize(InValuesObject);

	const auto* FixtureValues = InValuesObject.getTypedPointer<MikanDMXFixtureComponentValues>();
	StageId = FixtureValues->stage_id;
	DmxUniverse = static_cast<int32>(FixtureValues->dmx_universe);
	DmxStartChannel = static_cast<int32>(FixtureValues->dmx_start_channel);
	DmxChannelCount = static_cast<int32>(FixtureValues->dmx_channel_count);
	bIsDisabled = FixtureValues->is_disabled;
	MaxWattage = FixtureValues->max_wattage;
	LumensPerWatt = FixtureValues->lumens_per_watt;
}

bool UMikanDMXFixtureData::ApplyMikanValue(const FString& FieldName, const MikanVariant& FieldValue)
{
	if (FieldName == "stage_id")
	{
		StageId = FieldValue.getIntValue();
		return true;
	}
	else if (FieldName == "dmx_universe")
	{
		DmxUniverse = FieldValue.getIntValue();
		return true;
	}
	else if (FieldName == "dmx_start_channel")
	{
		DmxStartChannel = FieldValue.getIntValue();
		return true;
	}
	else if (FieldName == "dmx_channel_count")
	{
		DmxChannelCount = FieldValue.getIntValue();
		return true;
	}
	else if (FieldName == "is_disabled")
	{
		bIsDisabled = FieldValue.getBoolValue();
		return true;
	}
	else if (FieldName == "max_wattage")
	{
		MaxWattage = FieldValue.getFloatValue();
		return true;
	}
	else if (FieldName == "lumens_per_watt")
	{
		LumensPerWatt = FieldValue.getFloatValue();
		return true;
	}
	else
	{
		return UMikanTransformData::ApplyMikanValue(FieldName, FieldValue);
	}
}

void UMikanDMXFixtureData::Describe(TArray<FString>& OutLines) const
{
	UMikanTransformData::Describe(OutLines);
	Mikan::AppendDescribeLine(OutLines, TEXT("stage_id"), StageId);
	Mikan::AppendDescribeLine(OutLines, TEXT("dmx_universe"), DmxUniverse);
	Mikan::AppendDescribeLine(OutLines, TEXT("dmx_start_channel"), DmxStartChannel);
	Mikan::AppendDescribeLine(OutLines, TEXT("dmx_channel_count"), DmxChannelCount);
	Mikan::AppendDescribeLine(OutLines, TEXT("is_disabled"), bIsDisabled);
	Mikan::AppendDescribeLine(OutLines, TEXT("max_wattage"), MaxWattage);
	Mikan::AppendDescribeLine(OutLines, TEXT("lumens_per_watt"), LumensPerWatt);
}

// -- AMikanDMXFixtureActor -----
AMikanDMXFixtureActor::AMikanDMXFixtureActor(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void AMikanDMXFixtureActor::BindMikanComponentData(UMikanComponentData* Data)
{
	Super::BindMikanComponentData(Data);

	// Bind here rather than in PostInitializeComponents/BeginPlay: actors spawned into the
	// editor world are never "initialized" and play never begins, so those callbacks don't fire.
	// BindMikanComponentData runs explicitly from AMikanClient::SpawnMikanActor in all worlds.
	AMikanClient* OwnerMikanClient= GetOwnerMikanClient();
	if (OwnerMikanClient)
	{
		OwnerMikanClient->OnDMXDataChanged.AddUniqueDynamic(this, &AMikanDMXFixtureActor::OnDMXDataChanged);
	}

	// Seed from whatever the DMX store already holds. Subscribing only catches the next change,
	// and the store is usually filled before this actor exists: Mikan answers the subscription
	// with a snapshot while the component fetch is still spawning actors. Without this a fixture
	// stays dark until something happens to move its channels.
	OnDMXDataChanged();
}

void AMikanDMXFixtureActor::Destroyed()
{
	// Unbind in Destroyed rather than EndPlay: EndPlay only fires if BeginPlay ran, which it
	// doesn't for editor-world actors. Destroyed() is called from Destroy() regardless of play state.
	AMikanClient* OwnerMikanClient = GetOwnerMikanClient();
	if (OwnerMikanClient)
	{
		OwnerMikanClient->OnDMXDataChanged.RemoveDynamic(this, &AMikanDMXFixtureActor::OnDMXDataChanged);
	}

	Super::Destroyed();
}
