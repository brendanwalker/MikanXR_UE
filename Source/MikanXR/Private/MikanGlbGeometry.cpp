#include "MikanGlbGeometry.h"
#include "MikanAPI.h"
#include "MikanEngineSubsystem.h"

#include "Engine/Texture2D.h"
#include "HttpModule.h"
#include "ImageUtils.h"
#include "Interfaces/IHttpResponse.h"

THIRD_PARTY_INCLUDES_START
#if PLATFORM_WINDOWS
#pragma warning(push)
#pragma warning(disable : 4996)
#endif
#define CGLTF_IMPLEMENTATION
#include "cgltf.h"
#if PLATFORM_WINDOWS
#pragma warning(pop)
#endif
THIRD_PARTY_INCLUDES_END

namespace MikanGlbGeometry
{
FString MakeGeometryUrl(const FString& GeometryUrl)
{
	UMikanEngineSubsystem* Subsystem = UMikanEngineSubsystem::Get();
	IMikanAPI* MikanAPI = Subsystem != nullptr ? Subsystem->GetMikanAPI() : nullptr;
	if (MikanAPI == nullptr || GeometryUrl.IsEmpty())
	{
		return FString();
	}

	const FString BaseUrl = UTF8_TO_TCHAR(MikanAPI->getHttpBaseUrl().c_str());
	return BaseUrl.IsEmpty() ? FString() : BaseUrl + GeometryUrl;
}

TSharedPtr<IHttpRequest, ESPMode::ThreadSafe> FetchGeometry(const FString& Url, FOnGeometryFetched OnFetched)
{
	if (Url.IsEmpty())
	{
		return nullptr;
	}

	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
	Request->SetURL(Url);
	Request->SetVerb(TEXT("GET"));
	Request->OnProcessRequestComplete().BindLambda(
		[OnFetched = MoveTemp(OnFetched), Url](FHttpRequestPtr, FHttpResponsePtr Response, bool bConnectedSuccessfully)
		{
			FMikanGlbGeometry Geometry;
			if (!bConnectedSuccessfully || !Response.IsValid() || Response->GetResponseCode() != 200)
			{
				UE_LOG(LogTemp, Warning, TEXT("MikanGlbGeometry: fetch of %s failed (status %d)"), *Url,
					   Response.IsValid() ? Response->GetResponseCode() : 0);
				OnFetched(false, Geometry);
				return;
			}

			FString Error;
			if (!ParseGlb(Response->GetContent(), Geometry, Error))
			{
				UE_LOG(LogTemp, Warning, TEXT("MikanGlbGeometry: %s is not a usable GLB: %s"), *Url, *Error);
				OnFetched(false, Geometry);
				return;
			}

			OnFetched(true, Geometry);
		});

	if (!Request->ProcessRequest())
	{
		return nullptr;
	}

	return Request;
}

void CancelFetch(TSharedPtr<IHttpRequest, ESPMode::ThreadSafe>& Request)
{
	if (Request.IsValid())
	{
		Request->OnProcessRequestComplete().Unbind();
		Request->CancelRequest();
		Request.Reset();
	}
}

bool ParseGlb(const TArray<uint8>& Bytes, FMikanGlbGeometry& OutGeometry, FString& OutError)
{
	OutGeometry = FMikanGlbGeometry();

	cgltf_options Options;
	FMemory::Memzero(Options);

	cgltf_data* Data = nullptr;
	cgltf_result Result = cgltf_parse(&Options, Bytes.GetData(), Bytes.Num(), &Data);
	if (Result != cgltf_result_success)
	{
		OutError = FString::Printf(TEXT("parse failed (%d)"), (int32)Result);
		return false;
	}

	// A GLB carries its only buffer in the binary chunk, so no path is needed to resolve it
	Result = cgltf_load_buffers(&Options, Data, nullptr);
	if (Result != cgltf_result_success)
	{
		OutError = FString::Printf(TEXT("buffer load failed (%d)"), (int32)Result);
		cgltf_free(Data);
		return false;
	}

	for (cgltf_size ImageIndex = 0; ImageIndex < Data->images_count; ++ImageIndex)
	{
		const cgltf_buffer_view* View = Data->images[ImageIndex].buffer_view;
		TArray<uint8>& ImageBytes = OutGeometry.Images.AddDefaulted_GetRef();
		if (View != nullptr && View->buffer != nullptr && View->buffer->data != nullptr)
		{
			const uint8* ViewData = (const uint8*)View->buffer->data + View->offset;
			ImageBytes.Append(ViewData, (int32)View->size);
		}
	}

	for (cgltf_size MeshIndex = 0; MeshIndex < Data->meshes_count; ++MeshIndex)
	{
		const cgltf_mesh& Mesh = Data->meshes[MeshIndex];
		for (cgltf_size PrimitiveIndex = 0; PrimitiveIndex < Mesh.primitives_count; ++PrimitiveIndex)
		{
			const cgltf_primitive& Primitive = Mesh.primitives[PrimitiveIndex];
			if (Primitive.type != cgltf_primitive_type_triangles || Primitive.indices == nullptr)
			{
				continue;
			}

			const cgltf_accessor* PositionAccessor = nullptr;
			const cgltf_accessor* NormalAccessor = nullptr;
			const cgltf_accessor* UVAccessor = nullptr;
			for (cgltf_size AttributeIndex = 0; AttributeIndex < Primitive.attributes_count; ++AttributeIndex)
			{
				const cgltf_attribute& Attribute = Primitive.attributes[AttributeIndex];
				if (Attribute.type == cgltf_attribute_type_position)
				{
					PositionAccessor = Attribute.data;
				}
				else if (Attribute.type == cgltf_attribute_type_normal)
				{
					NormalAccessor = Attribute.data;
				}
				else if (Attribute.type == cgltf_attribute_type_texcoord && Attribute.index == 0)
				{
					UVAccessor = Attribute.data;
				}
			}
			if (PositionAccessor == nullptr || PositionAccessor->count == 0)
			{
				continue;
			}

			FMikanGlbMesh& OutMesh = OutGeometry.Meshes.AddDefaulted_GetRef();
			const int32 VertexCount = (int32)PositionAccessor->count;

			OutMesh.Positions.SetNumUninitialized(VertexCount);
			cgltf_accessor_unpack_floats(PositionAccessor, (cgltf_float*)OutMesh.Positions.GetData(), VertexCount * 3);

			if (NormalAccessor != nullptr && (int32)NormalAccessor->count == VertexCount)
			{
				OutMesh.Normals.SetNumUninitialized(VertexCount);
				cgltf_accessor_unpack_floats(NormalAccessor, (cgltf_float*)OutMesh.Normals.GetData(), VertexCount * 3);
			}

			if (UVAccessor != nullptr && (int32)UVAccessor->count == VertexCount)
			{
				OutMesh.UVs.SetNumUninitialized(VertexCount);
				cgltf_accessor_unpack_floats(UVAccessor, (cgltf_float*)OutMesh.UVs.GetData(), VertexCount * 2);
			}

			const int32 IndexCount = (int32)Primitive.indices->count;
			OutMesh.Indices.SetNumUninitialized(IndexCount);
			for (int32 Index = 0; Index < IndexCount; ++Index)
			{
				OutMesh.Indices[Index] = (uint32)cgltf_accessor_read_index(Primitive.indices, Index);
			}

			const cgltf_material* Material = Primitive.material;
			const cgltf_texture* BaseColorTexture =
				Material != nullptr ? Material->pbr_metallic_roughness.base_color_texture.texture : nullptr;
			if (BaseColorTexture != nullptr && BaseColorTexture->image != nullptr)
			{
				OutMesh.ImageIndex = (int32)(BaseColorTexture->image - Data->images);
			}
		}
	}

	cgltf_free(Data);

	if (OutGeometry.Meshes.Num() == 0)
	{
		OutError = TEXT("no triangle meshes");
		return false;
	}

	return true;
}

UTexture2D* CreateTexture(const TArray<uint8>& EncodedImage)
{
	if (EncodedImage.Num() == 0)
	{
		return nullptr;
	}

	return FImageUtils::ImportBufferAsTexture2D(TArrayView64<const uint8>(EncodedImage.GetData(), EncodedImage.Num()));
}
} // namespace MikanGlbGeometry
