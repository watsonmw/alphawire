#pragma once

#include "ptp-log.h"
#include "mlib/mlib.h"
#include "ptp/ptp-const.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Represents a list of PTP (Picture Transfer Protocol) devices and available backends.
 *
 * This structure serves as the central data model for managing connected PTP devices,
 * the respective backends for communication, and the currently active open devices.
 * It also tracks the state regarding whether the device list is up-to-date, in the
 * case where the backends support it.
 *
 * @code{.c}
 *    #include "ptp/ptp-control.h"
 *    #include "ptp/ptp-device-list.h"
 *    ...
 *    PTPDeviceList ptpDeviceList{};
 *    // Open device list
 *    PTPDeviceList_Open(&ptpDeviceList);
 *    // Poll for devices
 *    PTPDeviceList_RefreshList(&ptpDeviceList);
 *    // If one or more devices
 *    if (MArraySize(ptpDeviceList.devices) {
 *        PTPDeviceInfo* deviceInfo = ptpDeviceList.devices;
 *        PTPDevice* device = NULL;
 *        // Establish transport for first device
 *        PTPDeviceList_ConnectDevice(&ptpDeviceList, deviceInfo, &device);
 *        PTPControl ptp{};
 *        // Init control structure
 *        PTPControl_Init(&ptp, device, &allocator);
 *        // Connect to device with given mode
 *        PTPControl_Connect(&ptp, SDI_EXTENSION_VERSION_300);
 *    }
 * @endcode
 */
typedef struct {
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
 * adding them to the PTPDeviceList instance. Supported backends include LibUSBK
 * and WIA.
 *
 * @param self A pointer to the PTPDeviceList to be initialized.
 * @return TRUE if at least one backend was initialized
 */
b32 PTPDeviceList_Open(PTPDeviceList* self, MAllocator* allocator);

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
b32 PTPDeviceList_Close(PTPDeviceList* self);

/**
 * @brief Refreshes the list of PTP devices by clearing the current list and updating it.
 *
 * This function updates the state of the device list and populates it with the
 * latest set of devices available from all backends.
 *
 * @param self A pointer to the PTPDeviceList instance to be refreshed.
 * @return Returns TRUE (1) upon successful refresh of the device list.
 */
b32 PTPDeviceList_RefreshList(PTPDeviceList* self);

/**
 * Quick check if PTPDeviceList needs a refresh, without actually refreshing.
 *
 * Check each backend for device addition/removal flags.  Some backends may not support
 * this functionality.
 *
 * @param self Pointer to the PTPDeviceList instance to be checked.
 * @return TRUE if the list needs to be refreshed, FALSE otherwise.
 */
b32 PTPDeviceList_NeedsRefresh(PTPDeviceList* self);

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
b32 PTPDeviceList_OpenDevice(PTPDeviceList* self, PTPDeviceInfo* deviceInfo, PTPDevice** deviceOut);

/**
 * Disconnects from the specified device's transport.
 *
 * @param self Pointer to the PTPDeviceList which manages the devices.
 * @param device Pointer to the PTPDevice to be disconnected.
 * @return TRUE if the device was successfully disconnected.
 */
b32 PTPDeviceList_CloseDevice(PTPDeviceList* self, PTPDevice* device);

/**
 * Retrieves the backend of the specified type from the given PTPDeviceList.
 *
 * @param self A pointer to the PTPDeviceList structure containing the list of backends.
 * @param backendType The type of backend to be retrieved.
 * @return A pointer to the PTPBackend of the specified type, or NULL if the backend is not available.
 */
PTPBackend* PTPDeviceList_GetBackend(PTPDeviceList* self, PTPBackendType backend);

const char* PTP_GetBackendTypeStr(PTPBackendType type);

#ifdef __cplusplus
} // extern "C"
#endif
