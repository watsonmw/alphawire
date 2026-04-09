#pragma once

#include "mlib/mlib.h"

#include "aw/aw-backend.h"
#include "aw/aw-const.h"
#include "aw/aw-log.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Represents a list of PTP (Picture Transfer Protocol) devices and available backends.
 *
 * This structure serves as the central data model for managing connected PTP devices,
 * the respective backends for communication, and the currently active open devices.
 * It also tracks the state regarding whether the device list is up to date, in the
 * case where the backends support it.
 *
 * @code{.c}
 *    #include "aw/aw-control.h"
 *    #include "aw/paw-device-list.h"
 *    ...
 *    AwDeviceList deviceList{};
 *    // Open device list
 *    AwDeviceList_Open(&deviceList, allocator);
 *    // Poll for devices
 *    AwDeviceList_RefreshList(&deviceList);
 *    // If one or more devices
 *    if (MArraySize(awDeviceList.devices) {
 *        AwDeviceInfo* deviceInfo = deviceList.devices;
 *        AwDevice* device = NULL;
 *        // Establish transport for first device
 *        AwDeviceList_OpenDevice(&deviceList, deviceInfo, &device);
 *        AwControl aw{};
 *        // Init control structure
 *        AwControl_Init(&aw, device, &allocator);
 *        // Connect to device with given mode
 *        AwControl_Connect(&aw, SDI_EXTENSION_VERSION_300);
 *    }
 * @endcode
 */
typedef struct AwDeviceList {
    AwDeviceInfo* devices;
    AwBackend* backends;
    AwDevice* openDevices;
    // Set before calling When AwDeviceList_Open().  When et to 0 will block during device enumeration.
    u32 timeoutMilliseconds;
    AwBackendConfig backendConfig;
    MAllocator* allocator;
    AwLog logger;
} AwDeviceList;

/**
 * Opens the AwDeviceList by initializing and configuring available backends.
 *
 * This function iterates through the predefined backends and initializes each one,
 * adding them to the AwDeviceList instance. Supported backends include LibUSBK, LibUSB,
 * WIA, and TCP/IP.
 *
 * @param self A pointer to the AwDeviceList to be initialized.
 * @param allocator Allocator to use for device list enumeration
 * @return TRUE if at least one backend was initialized
 */
AW_EXPORT b32 AwDeviceList_Open(AwDeviceList* self, MAllocator* allocator);

/**
 * Closes the AwDeviceList and releases all associated resources.
 *
 * This function performs the following actions:
 * - Closes all backends associated with the AwDeviceList.
 * - Releases the list of devices managed by the AwDeviceList.
 *
 * @param self Pointer to the AwDeviceList instance to be closed.
 * @return TRUE when the closure and resource release are successful.
 */
AW_EXPORT b32 AwDeviceList_Close(AwDeviceList* self);

/**
 * @brief Refreshes the list of PTP devices by clearing the current list and updating it.
 *
 * This function updates the state of the device list and populates it with the
 * latest set of devices available from all backends.
 *
 * Blocking in most cases, but for the IP backend camera can take some time to response so response must
 * be polled.
 *
 * @param self A pointer to the AwDeviceList instance to be refreshed.
 * @return Returns TRUE (1) upon successful refresh of the device list.
 */
AW_EXPORT b32 AwDeviceList_RefreshList(AwDeviceList* self);

/**
 * Quick check if AwDeviceList needs a refresh, without actually refreshing.
 *
 * Check each backend for device addition/removal flags.  Some backends may not support
 * this functionality.
 *
 * @param self Pointer to the AwDeviceList instance to be checked.
 * @return TRUE if the list needs to be refreshed, FALSE otherwise.
 */
AW_EXPORT b32 AwDeviceList_NeedsRefresh(AwDeviceList* self);

/**
 * If AwDeviceList_RefreshList() starts a refresh in the background, use this function to check if polling for
 * updates is necessary (calls to AwDeviceList_PollUpdates() will add new devices found).
 *
 * @param self Pointer to the AwDeviceList instance to be checked.
 * @return TRUE if the list is currently being refreshed, FALSE otherwise.
 */
AW_EXPORT b32 AwDeviceList_IsRefreshingList(AwDeviceList* self);

/**
 * If AwDeviceList_RefreshList() start a refresh in the background, use this function to check if polling for
 * updates as necessary (calls to AwDeviceList_PollUpdates() will add new devices found).
 *
 * @param self Pointer to the AwDeviceList instance to be checked.
 * @return TRUE if a device was added
 */
AW_EXPORT b32 AwDeviceList_PollUpdates(AwDeviceList* self);

/**
 * Number of devices found.
 *
 * @param self Pointer to the AwDeviceList instance to be checked.
 * @return number of devices found
 */
AW_EXPORT size_t AwDeviceList_NumDevices(AwDeviceList* self);

/**
 * Connects to a device listed in the given AwDeviceList, using the specified device's transport.
 * Returns the connected device if successful.
 *
 * @param self A pointer to the AwDeviceList instance managing the devices.
 * @param deviceInfo A pointer to the AwDeviceInfo structure containing information
 *                   about the device to connect.
 * @param deviceOut A pointer to a pointer that will be set to the connected AwDevice
 *                  instance if the connection is successful.
 * @return AwResult.code == AW_RESULT_OK if the device was successfully disconnected.
 */
AW_EXPORT AwResult AwDeviceList_OpenDevice(AwDeviceList* self, AwDeviceInfo* deviceInfo, AwDevice** deviceOut);

/**
 * Disconnects from the specified device's transport.
 *
 * @param self Pointer to the AwDeviceList which manages the devices.
 * @param device Pointer to the AwDevice to be disconnected.
 * @return AwResult.code == AW_RESULT_OK if the device was successfully disconnected.
 */
AW_EXPORT AwResult AwDeviceList_CloseDevice(AwDeviceList* self, AwDevice* device);

/**
 * Retrieves the backend of the specified type from the given AwDeviceList.
 *
 * @param self A pointer to the AwDeviceList structure containing the list of backends.
 * @param backend The type of backend to be retrieved.
 * @return A pointer to the AwBackend of the specified type, or NULL if the backend is not available.
 */
AW_EXPORT AwBackend* AwDeviceList_GetBackend(AwDeviceList* self, AwBackendType backend);

#ifdef __cplusplus
} // extern "C"
#endif
