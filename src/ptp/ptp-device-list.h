#pragma once

#include "mlib/mlib.h"

#include "ptp/ptp-backend.h"
#include "ptp/ptp-const.h"
#include "ptp/ptp-log.h"

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
 *    #include "ptp/ptp-control.h"
 *    #include "ptp/ptp-device-list.h"
 *    ...
 *    PTPDeviceList ptpDeviceList{};
 *    // Open device list
 *    PTPDeviceList_Open(&ptpDeviceList, allocator);
 *    // Poll for devices
 *    PTPDeviceList_RefreshList(&ptpDeviceList);
 *    // If one or more devices
 *    if (MArraySize(ptpDeviceList.devices) {
 *        PTPDeviceInfo* deviceInfo = ptpDeviceList.devices;
 *        PTPDevice* device = NULL;
 *        // Establish transport for first device
 *        PTPDeviceList_OpenDevice(&ptpDeviceList, deviceInfo, &device);
 *        PTPControl ptp{};
 *        // Init control structure
 *        PTPControl_Init(&ptp, device, &allocator);
 *        // Connect to device with given mode
 *        PTPControl_Connect(&ptp, SDI_EXTENSION_VERSION_300);
 *    }
 * @endcode
 */
typedef struct PTPDeviceList {
    PTPDeviceInfo* devices;
    PTPBackend* backends;
    PTPDevice* openDevices;
    // Set before calling When PTPDeviceList_Open().  When et to 0 will block during device enumeration.
    u32 timeoutMilliseconds;

    MAllocator* allocator;
    PTPLog logger;
} PTPDeviceList;

/**
 * Opens the PTPDeviceList by initializing and configuring available backends.
 *
 * This function iterates through the predefined backends and initializes each one,
 * adding them to the PTPDeviceList instance. Supported backends include LibUSBK, LibUSB,
 * WIA, and TCP/IP.
 *
 * @param self A pointer to the PTPDeviceList to be initialized.
 * @param allocator Allocator to use for device list enumeration
 * @return TRUE if at least one backend was initialized
 */
PTP_EXPORT b32 PTPDeviceList_Open(PTPDeviceList* self, MAllocator* allocator);

/**
 * Closes the PTPDeviceList and releases all associated resources.
 *
 * This function performs the following actions:
 * - Closes all backends associated with the PTPDeviceList.
 * - Releases the list of devices managed by the PTPDeviceList.
 *
 * @param self Pointer to the PTPDeviceList instance to be closed.
 * @return TRUE when the closure and resource release are successful.
 */
PTP_EXPORT b32 PTPDeviceList_Close(PTPDeviceList* self);

/**
 * @brief Refreshes the list of PTP devices by clearing the current list and updating it.
 *
 * This function updates the state of the device list and populates it with the
 * latest set of devices available from all backends.
 *
 * Blocking in most cases, but for the IP backend camera can take some time to response so response must
 * be polled.
 *
 * @param self A pointer to the PTPDeviceList instance to be refreshed.
 * @return Returns TRUE (1) upon successful refresh of the device list.
 */
PTP_EXPORT b32 PTPDeviceList_RefreshList(PTPDeviceList* self);

/**
 * Quick check if PTPDeviceList needs a refresh, without actually refreshing.
 *
 * Check each backend for device addition/removal flags.  Some backends may not support
 * this functionality.
 *
 * @param self Pointer to the PTPDeviceList instance to be checked.
 * @return TRUE if the list needs to be refreshed, FALSE otherwise.
 */
PTP_EXPORT b32 PTPDeviceList_NeedsRefresh(PTPDeviceList* self);

/**
 * If PTPDeviceList_RefreshList() starts a refresh in the background, use this function to check if polling for
 * updates is necessary (calls to PTPDeviceList_PollUpdates() will add new devices found).
 *
 * @param self Pointer to the PTPDeviceList instance to be checked.
 * @return TRUE if the list is currently being refreshed, FALSE otherwise.
 */
PTP_EXPORT b32 PTPDeviceList_IsRefreshingList(PTPDeviceList* self);

/**
 * If PTPDeviceList_RefreshList() start a refresh in the background, use this function to check if polling for
 * updates as necessary (calls to PTPDeviceList_PollUpdates() will add new devices found).
 *
 * @param self Pointer to the PTPDeviceList instance to be checked.
 * @return TRUE if a device was added
 */
PTP_EXPORT b32 PTPDeviceList_PollUpdates(PTPDeviceList* self);

/**
 * Number of devices found.
 *
 * @param self Pointer to the PTPDeviceList instance to be checked.
 * @return number of devices found
 */
PTP_EXPORT size_t PTPDeviceList_NumDevices(PTPDeviceList* self);

/**
 * Connects to a device listed in the given PTPDeviceList, using the specified device's transport.
 * Returns the connected device if successful.
 *
 * @param self A pointer to the PTPDeviceList instance managing the devices.
 * @param deviceInfo A pointer to the PTPDeviceInfo structure containing information
 *                   about the device to connect.
 * @param deviceOut A pointer to a pointer that will be set to the connected PTPDevice
 *                  instance if the connection is successful.
 * @return TRUE if the connection was successful.
 */
PTP_EXPORT b32 PTPDeviceList_OpenDevice(PTPDeviceList* self, PTPDeviceInfo* deviceInfo, PTPDevice** deviceOut);

/**
 * Disconnects from the specified device's transport.
 *
 * @param self Pointer to the PTPDeviceList which manages the devices.
 * @param device Pointer to the PTPDevice to be disconnected.
 * @return TRUE if the device was successfully disconnected.
 */
PTP_EXPORT b32 PTPDeviceList_CloseDevice(PTPDeviceList* self, PTPDevice* device);

/**
 * Retrieves the backend of the specified type from the given PTPDeviceList.
 *
 * @param self A pointer to the PTPDeviceList structure containing the list of backends.
 * @param backend The type of backend to be retrieved.
 * @return A pointer to the PTPBackend of the specified type, or NULL if the backend is not available.
 */
PTP_EXPORT PTPBackend* PTPDeviceList_GetBackend(PTPDeviceList* self, PTPBackendType backend);

#ifdef __cplusplus
} // extern "C"
#endif
