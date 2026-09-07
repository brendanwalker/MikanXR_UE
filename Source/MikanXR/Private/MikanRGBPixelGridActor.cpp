#include "MikanRGBPixelGridActor.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"

// -- UMikanRGBPixelGridData -----
void UMikanRGBPixelGridData::Initialize(const Serialization::PolymorphicObjectPtr& InValuesObject)
{
	UMikanDMXFixtureData::Initialize(InValuesObject);

	const auto* PixelGridValues = InValuesObject.getTypedPointer<MikanRGBPixelGridComponentValues>();
	GridColumns = PixelGridValues->grid_columns;
	GridRows = PixelGridValues->grid_rows;
}

bool UMikanRGBPixelGridData::ApplyMikanValue(const FString& FieldName, const MikanVariant& FieldValue)
{
	if (FieldName == "grid_columns")
	{
		GridColumns = FieldValue.getIntValue();
		return true;
	}
	else if (FieldName == "grid_rows")
	{
		GridRows = FieldValue.getIntValue();
		return true;
	}
	else
	{
		return UMikanDMXFixtureData::ApplyMikanValue(FieldName, FieldValue);
	}
}

void UMikanRGBPixelGridData::Describe(TArray<FString>& OutLines) const
{
	UMikanDMXFixtureData::Describe(OutLines);
	Mikan::AppendDescribeLine(OutLines, TEXT("grid_columns"), GridColumns);
	Mikan::AppendDescribeLine(OutLines, TEXT("grid_rows"), GridRows);
}

// -- AMikanRGBPixelGridActor -----
AMikanRGBPixelGridActor::AMikanRGBPixelGridActor(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void AMikanRGBPixelGridActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (GridColumns <= 0 || GridRows <= 0)
		return;

	// Draw a grid pattern to visualize the pixel grid
	const FVector Center = GetActorLocation();
	const FRotator Rot = GetActorRotation();
	const float CellSize = 10.f;
	const float GridWidth = GridColumns * CellSize;
	const float GridHeight = GridRows * CellSize;

	// Calculate grid corner positions
	const FVector Right = Rot.RotateVector(FVector(GridWidth * 0.5f, 0.f, 0.f));
	const FVector Up = Rot.RotateVector(FVector(0.f, 0.f, GridHeight * 0.5f));
	const FVector TopLeft = Center - Right + Up;

	const FColor GridColor = FColor::Magenta;

	// Draw vertical lines
	for (int32 Col = 0; Col <= GridColumns; ++Col)
	{
		const FVector Offset = Rot.RotateVector(FVector(Col * CellSize, 0.f, 0.f));
		const FVector Start = TopLeft + Offset;
		const FVector End = Start - Up * 2.f;
		DrawDebugLine(GetWorld(), Start, End, GridColor);
	}

	// Draw horizontal lines
	for (int32 Row = 0; Row <= GridRows; ++Row)
	{
		const FVector Offset = Rot.RotateVector(FVector(0.f, 0.f, -Row * CellSize));
		const FVector Start = TopLeft + Offset;
		const FVector End = Start + Right * 2.f;
		DrawDebugLine(GetWorld(), Start, End, GridColor);
	}
}

void AMikanRGBPixelGridActor::BindMikanComponentData(UMikanComponentData* Data)
{
	Super::BindMikanComponentData(Data);

	if (auto* PixelGridData = Cast<UMikanRGBPixelGridData>(Data))
	{
		GridColumns = PixelGridData->GetGridColumns();
		GridRows = PixelGridData->GetGridRows();
	}
}