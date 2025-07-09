#pragma once

#include "ptp/ptp-control.h"
#include "ptp/ptp-device-list.h"
#include "ptp/utf.h"

#include <vector>
#include <algorithm>
#include <locale>
#include <string>

#include "platform/windows/ptp-backend-wia.h"

#include "imgui.h"

struct UiPtpProperty {
    char propCode[32];
    char *propName;
    PtpProperty *prop;

    UiPtpProperty() {
        memset(propCode, 0, 32);
        propName = NULL;
        prop = NULL;
    }

    void copy(const UiPtpProperty &other) noexcept {
        memcpy(this->propCode, other.propCode, 32);
        if (other.propName == other.propCode) {
            this->propName = this->propCode;
        } else {
            this->propName = other.propName;
        }
        this->prop = other.prop;
    }

    UiPtpProperty(const UiPtpProperty &other) { copy(other); }

    // Move constructor
    UiPtpProperty(UiPtpProperty &&other) noexcept { copy(other); }

    // Move assignment operator
    UiPtpProperty &operator=(UiPtpProperty &&other) noexcept {
        copy(other);
        return *this;
    }
};

inline bool findStringCaseInsensitive(const std::string& strHaystack, const std::string& strNeedle) {
    auto it = std::search(strHaystack.begin(), strHaystack.end(),
        strNeedle.begin(),   strNeedle.end(),[](unsigned char ch1, unsigned char ch2) {
            return std::toupper(ch1) == std::toupper(ch2);
        }
    );
    return it != strHaystack.end();
}

struct PropTable {
    ImGuiTableSortSpecs* sortSpecs = nullptr;
    bool needsSort = false;
    bool needsRebuild = true;
    std::vector<UiPtpProperty> items;
    char searchText[32]{};

    bool compareItems(const UiPtpProperty& a, const UiPtpProperty& b) {
        for (int n = 0; n < sortSpecs->SpecsCount; n++) {
            const ImGuiTableColumnSortSpecs *spec = &sortSpecs->Specs[n];
            int delta = 0;

            switch (spec->ColumnIndex) {
                case 0: // Code
                    delta = strcmp(a.propCode, b.propCode);
                    break;
                case 1: // Name
                    delta = UTF8_StrCmp(a.propName, b.propName);
                    break;
                case 2: // Type
                    delta = (int) a.prop->dataType - (int) b.prop->dataType;
                    break;
                case 3: // Form
                    delta = (int) a.prop->formFlag - (int) b.prop->formFlag;
                    break;
                case 4: // Get/Set
                    delta = (int) a.prop->getSet - (int) b.prop->getSet;
                    break;
                case 5: // Enabled
                    delta = (int) a.prop->isEnabled - (int) b.prop->isEnabled;
                    break;
            }

            if (delta > 0) {
                return spec->SortDirection != ImGuiSortDirection_Ascending;
            } else if (delta < 0) {
                return spec->SortDirection == ImGuiSortDirection_Ascending;
            }
        }

        return false;
    }

    void rebuild(PTPControl& ptp) {
        items.clear();
        for (size_t i = 0; i < MArraySize(ptp.properties); i++) {
            PtpProperty *property = ptp.properties + i;

            UiPtpProperty uiPtpProperty{};
            sprintf(uiPtpProperty.propCode, "0x%04x", property->propCode);
            uiPtpProperty.propName = Ptp_GetPropertyStr(property->propCode);
            if (uiPtpProperty.propName == NULL) {
                uiPtpProperty.propName = uiPtpProperty.propCode;
            }
            uiPtpProperty.prop = property;

            if (searchText[0] == 0) {
                items.emplace_back(uiPtpProperty);
            } else {
                std::string propCode = uiPtpProperty.propCode;
                std::string propName = uiPtpProperty.propName;
                if (findStringCaseInsensitive(propCode, searchText) ||
                    findStringCaseInsensitive(propName, searchText)) {
                    items.emplace_back(uiPtpProperty);
                }
            }
        }
        needsRebuild = false;
    }

    void sort() {
        std::sort(items.begin(), items.end(), [this](const UiPtpProperty &a, const UiPtpProperty &b) {
            return compareItems(a, b);
        });
        needsSort = false;
    }

    void reset() {
        needsRebuild = true;
        searchText[0] = 0;
        items.clear();
    }
};

struct AppContext {
    PTPDeviceList ptpDeviceList{};
    PTPDevice* device = NULL;
    PTPControl ptp{};
    int selectedDeviceIndex = -1;
    int selectedProtoVersion = 1;

    // Device state
    bool connected = false;

    // Window state
    bool showWindowDebugPropertyOrControl = false;
    bool showWindowDeviceDebug = false;
    bool showWindowPropDebug = false;

    // Properties refresh
    double propertyLastRefreshTime = 0.;
    bool propAutoRefresh = true;
    bool propRefresh = true;

    // Property & Controls Debug
    PtpProperty* selectedProperty = nullptr;
    PtpControl* selectedControl = nullptr;
    PropValue selectedControlValue;
    PropTable propTable{};

    // Live View state
    bool liveViewOpen = false;
    double liveViewLastTime = 0.;
    MMemIO liveViewImage{};
    LiveViewFrames liveViewFrames{};
    ImTextureID liveViewImageGLId = 0;
    i32 liveViewImageWidth = 0;
    i32 liveViewImageHeight = 0;

    // Camera settings file
    bool cameraSettingsSaveEnabled = false;
    bool cameraSettingsReadEnabled = false;
    char cameraSettingsPathBuffer[1024] = "camera_settings.dat";

    // File download
    i64 fileDownloadTotalMillis = 0;
    i64 fileDownloadTimeMillis = 0;
    size_t fileDownloadTotalBytes = 0;
    bool fileDownloadAuto = false;
    std::string fileDownloadPath = "";

    bool shutterHalfPress = false;
    bool autoFocusButton = false;

    void refreshDevices() {
        selectedDeviceIndex = -1;
        PTPDeviceList_RefreshList(&ptpDeviceList);
    }

    void connect() {
        if (selectedDeviceIndex != -1 && MArraySize(ptpDeviceList.devices) > selectedDeviceIndex) {
            PTPDeviceInfo* deviceInfo = ptpDeviceList.devices + selectedDeviceIndex;
            MLogf("Connecting to device: %s (%s)...", deviceInfo->deviceName.str, deviceInfo->manufacturer.str);
            disconnectDevice();
            b32 ok = PTPDeviceList_ConnectDevice(&ptpDeviceList, deviceInfo, &device);
            if (ok) {
                PTPControl_Init(&ptp, &device->transport);
                PTPControl_Connect(&ptp, selectedProtoVersion ? SDI_EXTENSION_VERSION_300 : SDI_EXTENSION_VERSION_200);
                connected = true;
            }
        }
        propTable.reset();
        if (connected) {
            cameraSettingsSaveEnabled = PtpControl_PropertyEnabled(&ptp, DPC_CAMERA_SETTING_SAVE_ENABLED);
            cameraSettingsReadEnabled = PtpControl_PropertyEnabled(&ptp, DPC_CAMERA_SETTING_READ_ENABLED);
        }
    }

    void disconnect() {
        disconnectDevice();
        connected = false;
        selectedProperty = nullptr;
        selectedControl = nullptr;
        cameraSettingsSaveEnabled = false;
        cameraSettingsReadEnabled = false;
        shutterHalfPress = false;
        propTable.reset();
    }

    void disconnectDevice() {
        if (device != NULL) {
            PTPControl_Cleanup(&ptp);
            PTPDeviceList_DisconnectDevice(&ptpDeviceList, device);
            MMemFree(&this->liveViewImage);
            device = NULL;
        }
    }

    void cleanupAll() {
        disconnectDevice();
        PTPDeviceList_Close(&ptpDeviceList);
    }
};
