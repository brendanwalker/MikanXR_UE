#include "MikanDMXDataStore.h"
#include "IMikanXRModule.h"
#include "MikanLightTypes.h"
#include "MikanLightEvents.h"

UE_DISABLE_OPTIMIZATION

// -- UMikanDMXUniverse -----
void UMikanDMXUniverse::Initialize(int32 InUniverseId)
{
	UniverseId= InUniverseId;
	FMemory::Memset(ChannelData, 0, sizeof(ChannelData));
}

void UMikanDMXUniverse::ApplyMikanUniverseData(const MikanUniverseDMXData& UniverseData)
{
	check((int32)UniverseData.dmx_universe_id == UniverseId);
	mikanRLEDecodeDMXUniverseBuffer(&UniverseData, sizeof(ChannelData), ChannelData);
}

uint8_t UMikanDMXUniverse::ReadChannelValue(const int32 Channel) const
{
	return (Channel >= 1 && Channel  <= kDMXUniverseChannelCount) ? ChannelData[Channel - 1] : 0;
}

// -- UMikanDMXDataStore -----

void UMikanDMXDataStore::Initialize()
{
	LastUpdateServerTime= 0;
	DMXUniverses.Reset();
}

void UMikanDMXDataStore::ApplyMikanUniverseData(const MikanLightDMXDataChangedEvent& DataChangeEvent)
{
	LastUpdateServerTime = DataChangeEvent.dmx_data.server_time_seconds;

	for (const auto& MikanUniverseDMXData : DataChangeEvent.dmx_data.universes)
	{
		UMikanDMXUniverse* Universe= FindOrAddDMXUniverse((int32)MikanUniverseDMXData.dmx_universe_id);
		if (Universe)
		{
			Universe->ApplyMikanUniverseData(MikanUniverseDMXData);
		}
	}
}

UMikanDMXUniverse* UMikanDMXDataStore::GetDMXUniverse(int32 UniverseId)
{
	UMikanDMXUniverse** UniverseEntry= DMXUniverses.Find(UniverseId);
	if (UniverseEntry)
	{
		return *UniverseEntry;
	}

	return nullptr;
}

UMikanDMXUniverse* UMikanDMXDataStore::FindOrAddDMXUniverse(int32 UniverseId)
{
	UMikanDMXUniverse** UniverseEntry = DMXUniverses.Find(UniverseId);
	if (UniverseEntry)
	{
		return *UniverseEntry;
	}
	else
	{
		UMikanDMXUniverse* NewUniverse = NewObject<UMikanDMXUniverse>(this);
		NewUniverse->Initialize(UniverseId);

		DMXUniverses.Add(UniverseId, NewUniverse);

		return NewUniverse;
	}
}
UE_ENABLE_OPTIMIZATION