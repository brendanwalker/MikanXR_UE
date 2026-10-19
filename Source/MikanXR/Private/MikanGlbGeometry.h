#pragma once

#include "CoreMinimal.h"
#include "Interfaces/IHttpRequest.h"

// One triangle list from a Mikan geometry GLB, still in Mikan's world convention (meters,
// right-handed, Y up). UVs are V-down as glTF stores them, which is also how Unreal samples.
struct FMikanGlbMesh
{
	TArray<FVector3f> Positions;
	TArray<FVector3f> Normals;
	TArray<FVector2f> UVs;
	TArray<uint32> Indices;
	// Index into FMikanGlbGeometry::Images of the mesh's base colour texture, or INDEX_NONE
	int32 ImageIndex = INDEX_NONE;
};

struct FMikanGlbGeometry
{
	TArray<FMikanGlbMesh> Meshes;
	// Encoded PNG or JPEG bytes, one per glTF image
	TArray<TArray<uint8>> Images;
};

// Fetches and decodes the GLB a model stencil or model shape publishes as its geometry_url
namespace MikanGlbGeometry
{
using FOnGeometryFetched = TFunction<void(bool bSuccess, const FMikanGlbGeometry& Geometry)>;

// Joins the Mikan HTTP base URL to a component's geometry_url. Empty when either is unknown.
FString MakeGeometryUrl(const FString& GeometryUrl);

// Starts a GET of Url and decodes the reply on the game thread. Returns the request so the caller
// can cancel it, or null when the request could not start.
TSharedPtr<IHttpRequest, ESPMode::ThreadSafe> FetchGeometry(const FString& Url, FOnGeometryFetched OnFetched);

// Cancels a fetch started by FetchGeometry without invoking its callback
void CancelFetch(TSharedPtr<IHttpRequest, ESPMode::ThreadSafe>& Request);

// Decodes GLB bytes. Meshes and images are read whatever their order in the file; node transforms
// are ignored, since Mikan writes every mesh under an identity node.
bool ParseGlb(const TArray<uint8>& Bytes, FMikanGlbGeometry& OutGeometry, FString& OutError);

// Decodes an encoded image into a transient texture, or returns null
class UTexture2D* CreateTexture(const TArray<uint8>& EncodedImage);
} // namespace MikanGlbGeometry
