#ifndef DX8GL_PRIVATE_DATA_H
#define DX8GL_PRIVATE_DATA_H

#include "d3d8_types.h"
#include "guid_utils.h"
#include <unordered_map>
#include <vector>
#include <memory>
#include <mutex>

namespace dx8gl {

// Helper class to manage private data storage for D3D8 resources
class PrivateDataManager {
public:
    PrivateDataManager() = default;
    ~PrivateDataManager() = default;

    // Set private data for a given GUID
    HRESULT SetPrivateData(REFGUID refguid, const void* pData, DWORD SizeOfData, DWORD Flags);
    
    // Get private data for a given GUID
    HRESULT GetPrivateData(REFGUID refguid, void* pData, DWORD* pSizeOfData);
    
    // Free private data for a given GUID
    HRESULT FreePrivateData(REFGUID refguid);

private:
    struct PrivateData {
        std::vector<uint8_t> data;
        DWORD flags;
    };

    mutable std::mutex mutex_;
    std::unordered_map<GUID, PrivateData> private_data_;
};

} // namespace dx8gl

#endif // DX8GL_PRIVATE_DATA_H