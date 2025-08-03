#include "private_data.h"
#include "d3d8_types.h"
#include "d3d8_constants.h"
#include <cstring>

// Private data flags (D3D8 doesn't define D3DSPD_IUNKNOWN)
#ifndef D3DSPD_IUNKNOWN
#define D3DSPD_IUNKNOWN 0x00000001L
#endif

namespace dx8gl {

HRESULT PrivateDataManager::SetPrivateData(REFGUID refguid, const void* pData, 
                                          DWORD SizeOfData, DWORD Flags) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // D3DSPD_IUNKNOWN is not supported in D3D8
    if (Flags & D3DSPD_IUNKNOWN) {
        return D3DERR_INVALIDCALL;
    }
    
    // If pData is null, it's equivalent to freeing the data
    if (!pData) {
        return FreePrivateData(refguid);
    }
    
    // Store the data
    PrivateData& pd = private_data_[*refguid];
    pd.data.resize(SizeOfData);
    if (SizeOfData > 0) {
        std::memcpy(pd.data.data(), pData, SizeOfData);
    }
    pd.flags = Flags;
    
    return D3D_OK;
}

HRESULT PrivateDataManager::GetPrivateData(REFGUID refguid, void* pData, 
                                          DWORD* pSizeOfData) {
    if (!pSizeOfData) {
        return D3DERR_INVALIDCALL;
    }
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = private_data_.find(*refguid);
    if (it == private_data_.end()) {
        return D3DERR_NOTFOUND;
    }
    
    const PrivateData& pd = it->second;
    DWORD dataSize = static_cast<DWORD>(pd.data.size());
    
    // If pData is null, just return the required size
    if (!pData) {
        *pSizeOfData = dataSize;
        return D3D_OK;
    }
    
    // Check if the provided buffer is large enough
    if (*pSizeOfData < dataSize) {
        *pSizeOfData = dataSize;
        return E_FAIL;  // D3D8 doesn't have D3DERR_MOREDATA
    }
    
    // Copy the data
    if (dataSize > 0) {
        std::memcpy(pData, pd.data.data(), dataSize);
    }
    *pSizeOfData = dataSize;
    
    return D3D_OK;
}

HRESULT PrivateDataManager::FreePrivateData(REFGUID refguid) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = private_data_.find(*refguid);
    if (it == private_data_.end()) {
        return D3DERR_NOTFOUND;
    }
    
    private_data_.erase(it);
    return D3D_OK;
}

} // namespace dx8gl