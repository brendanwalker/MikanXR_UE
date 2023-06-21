/**
\file
*/ 

#ifndef __MIKAN_CLIENT_CAPI_H
#define __MIKAN_CLIENT_CAPI_H
#include "MikanClient_export.h"
#include "MikanClientTypes.h"
#include <stdbool.h>
#include <stdint.h>
//cut_before

/** 
\brief Client Interface for MikanXR
\defgroup MikanClient_CAPI Client Interface
\addtogroup MikanClient_CAPI 
@{ 
*/
 
// Wrapper Types
//--------------

typedef void (MIKAN_CALLBACK* MikanLogCallback)(int /*log_level*/, const char* /*log_message*/);

/** \brief Initializes the MikanXR Client API.
 This function must be called before calling any other client functions. 
 Calling this function after the api is already initialized will return MikanResult_Success.

 \returns MikanResult_Success on success or MikanResult_Error on a general connection error.
 */
MIKAN_PUBLIC_FUNCTION(MikanResult) Mikan_Initialize(MikanLogLevel min_log_level, MikanLogCallback log_callback);

MIKAN_PUBLIC_FUNCTION(bool) Mikan_GetIsInitialized();

// System State Queries
/** \brief Get the client API version string
    \return A zero-terminated version string of the format "Product.Major-Phase Minor.Release.Hotfix", ex: "0.9-alpha 8.1.0"
 */
MIKAN_PUBLIC_FUNCTION(const char*) Mikan_GetVersionString();

MIKAN_PUBLIC_FUNCTION(MikanResult) Mikan_Connect(MikanClientInfo* client_info);

/** \brief Get the client connection status
    \return true if the client is connected to MikanXR Client API
 */
MIKAN_PUBLIC_FUNCTION(bool) Mikan_GetIsConnected();

/** \brief Retrieve the next message from the message queue.
	A call to \ref Mikan_UpdateNoPollMessages will queue messages received from MikanXR Client API.
	Use this function to processes the queued event and response messages one by one.
	If a response message does not have a callback registered with \ref Mikan_RegisterCallback it will get returned here.	
	\param[out] out_messaage The next \ref MikanMessage read from the incoming message queue.
	\param message_size The size of the message structure. Pass in sizeof(MikanMessage).
	\return MikanResult_Success or MikanResult_NoData if no more messages are available.
 */
MIKAN_PUBLIC_FUNCTION(MikanResult) Mikan_PollNextEvent(MikanEvent *out_event);

MIKAN_PUBLIC_FUNCTION(MikanResult) Mikan_GetVideoSourceIntrinsics(MikanVideoSourceIntrinsics *out_intrinsics);

MIKAN_PUBLIC_FUNCTION(MikanResult) Mikan_GetVideoSourceMode(MikanVideoSourceMode *out_info);

MIKAN_PUBLIC_FUNCTION(MikanResult) Mikan_GetVideoSourceAttachment(MikanVideoSourceAttachmentInfo* out_info);

MIKAN_PUBLIC_FUNCTION(MikanResult) Mikan_GetVRDeviceList(MikanVRDeviceList *out_vr_device_list);

MIKAN_PUBLIC_FUNCTION(MikanResult) Mikan_GetVRDeviceInfo(MikanVRDeviceID device_id, MikanVRDeviceInfo *out_vr_device_info);

MIKAN_PUBLIC_FUNCTION(MikanResult) Mikan_SubscribeToVRDevicePoseUpdates(MikanVRDeviceID device_id);

MIKAN_PUBLIC_FUNCTION(MikanResult) Mikan_UnsubscribeFromVRDevicePoseUpdates(MikanVRDeviceID device_id);

MIKAN_PUBLIC_FUNCTION(MikanResult) Mikan_SetGraphicsDeviceInterface(MikanClientGraphicsApi api, void* graphicsDeviceInterface);

MIKAN_PUBLIC_FUNCTION(MikanResult) Mikan_GetGraphicsDeviceInterface(MikanClientGraphicsApi api, void** outGraphicsDeviceInterface);

MIKAN_PUBLIC_FUNCTION(MikanResult) Mikan_AllocateRenderTargetBuffers(const MikanRenderTargetDescriptor *descriptor, MikanRenderTargetMemory *out_memory_ptr);

MIKAN_PUBLIC_FUNCTION(MikanResult) Mikan_PublishRenderTargetTexture(void* ApiTexturePtr, uint64_t frame_index);

MIKAN_PUBLIC_FUNCTION(MikanResult) Mikan_PublishRenderTargetBuffers(uint64_t frame_index);

MIKAN_PUBLIC_FUNCTION(MikanResult) Mikan_FreeRenderTargetBuffers();

MIKAN_PUBLIC_FUNCTION(MikanResult) Mikan_GetStencilList(MikanStencilList* out_stencil_list);

MIKAN_PUBLIC_FUNCTION(MikanResult) Mikan_GetQuadStencil(MikanStencilID stencil_id, MikanStencilQuad* out_stencil);

MIKAN_PUBLIC_FUNCTION(MikanResult) Mikan_GetBoxStencil(MikanStencilID stencil_id, MikanStencilBox* out_stencil);

MIKAN_PUBLIC_FUNCTION(MikanResult) Mikan_GetModelStencil(MikanStencilID stencil_id, MikanStencilModel* out_stencil);

MIKAN_PUBLIC_FUNCTION(MikanResult) Mikan_GetSpatialAnchorList(MikanSpatialAnchorList* out_anchor_list);

MIKAN_PUBLIC_FUNCTION(MikanResult) Mikan_GetSpatialAnchorInfo(MikanSpatialAnchorID anchor_id, MikanSpatialAnchorInfo* out_anchor_info);

MIKAN_PUBLIC_FUNCTION(MikanResult) Mikan_FindSpatialAnchorInfoByName(const char* anchor_name, MikanSpatialAnchorInfo* out_anchor_info);

MIKAN_PUBLIC_FUNCTION(MikanResult) Mikan_Disconnect();

/** \brief Cleans up the MikanXR Client API
 Free the resources allocated by the MikanXR Client API.
 Calling this function again after the api already cleaned up will return MikanResult_Error.

  \returns MikanResult_Success on success or MikanResult_Error if there was no valid connection.
 */
MIKAN_PUBLIC_FUNCTION(MikanResult) Mikan_Shutdown();


/** 
@} 
*/ 

//cut_after
#endif
