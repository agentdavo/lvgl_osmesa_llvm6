#ifndef DX8GL_UNUSED_PARAMS_H
#define DX8GL_UNUSED_PARAMS_H

// Helper macros for unused parameters in stub implementations
#define UNUSED_DEVICE_PARAMS(device) UNUSED(device)
#define UNUSED_TEXTURE_PARAMS(refguid, pData, SizeOfData, Flags) \
    UNUSED(refguid); UNUSED(pData); UNUSED(SizeOfData); UNUSED(Flags)
#define UNUSED_PRIVATE_DATA_GET(refguid, pData, pSizeOfData) \
    UNUSED(refguid); UNUSED(pData); UNUSED(pSizeOfData)
#define UNUSED_PRIVATE_DATA_FREE(refguid) UNUSED(refguid)

#endif // DX8GL_UNUSED_PARAMS_H