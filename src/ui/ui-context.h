#pragma once

#include "aw/aw-control.h"
#include "aw/aw-device-list.h"
#include "../mlib/utf8.h"

#include <vector>
#include <algorithm>
#include <locale>
#include <string>
#include <deque>

#ifdef AW_ENABLE_WIA
#include "aw/platform/windows/aw-backend-wia.h"
#endif

#include "imgui.h"

void TextureRelease(ImTextureID* outTexture);

struct UiPtpProperty {
    char propCode[32];
    char* propLabel;
    AwPtpProperty *prop;

    UiPtpProperty() {
        memset(propCode, 0, 32);
        propLabel = NULL;
        prop = NULL;
    }

    void copy(const UiPtpProperty &other) noexcept {
        memcpy(this->propCode, other.propCode, 32);
        if (MCStrCmp(other.propLabel, other.propCode) == 0) {
            this->propLabel = this->propCode;
        } else {
            this->propLabel = other.propLabel;
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
                    delta = UTF8_StrCmp(a.propLabel, b.propLabel);
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

    void rebuild(AwControl& ptp) {
        items.clear();
        for (size_t i = 0; i < MArraySize(ptp.properties); i++) {
            AwPtpProperty *property = ptp.properties + i;

            UiPtpProperty uiPtpProperty{};
            snprintf(uiPtpProperty.propCode,  sizeof(uiPtpProperty.propCode), "0x%04x", property->propCode);
            uiPtpProperty.propLabel = AwGetPropertyLabel(property->propCode);
            if (uiPtpProperty.propLabel == NULL) {
                uiPtpProperty.propLabel = uiPtpProperty.propCode;
            }
            uiPtpProperty.prop = property;

            if (searchText[0] == 0) {
                items.emplace_back(uiPtpProperty);
            } else {
                std::string propCode = uiPtpProperty.propCode;
                std::string propName = uiPtpProperty.propLabel;
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

struct LogEntry {
    AwLogLevel level;
    std::string message;

    LogEntry(AwLogLevel lvl, const char* msg) : level(lvl), message(msg) {}
};

struct LogWindow {
    std::deque<LogEntry> entries;
    bool autoScroll = true;
    int selectedLogLevel = AW_LOG_LEVEL_TRACE;  // Show all logs by default
    bool showWindow = false;
    static const size_t MAX_LOG_ENTRIES = 5000;

    void AddLog(AwLogLevel level, const char* message) {
        entries.emplace_back(level, message);
        if (entries.size() > MAX_LOG_ENTRIES) {
            entries.pop_front();
        }
    }

    void Clear() {
        entries.clear();
    }
};

enum LiveViewClickAction {
    LiveViewClickAction_NONE = 0,
    LiveViewClickAction_MOVE_FOCUS = 1,
    LiveViewClickAction_MAGNIFY = 2,
};

enum LiveViewOverlayMode {
    LiveViewOverlayMode_NONE = 0,
    LiveViewOverlayMode_X = 1,
    LiveViewOverlayMode_CROSSHAIR = 2,
};

struct AppContext {
    MAllocator* deviceListAllocator = NULL;
    MAllocator* autoReleasePool = NULL;
    MAllocator deviceAllocator{};
    AwDeviceList deviceList{};
    AwDevice* device = NULL;
    AwControl aw{};
    int selectedDeviceIndex = -1;
    int selectedProtoVersion = 1;

    // Device state
    bool connected = false;

    // Window state
    bool showWindowDebugPropertyOrControl = false;
    bool showWindowDeviceDebug = true;
    bool showWindowPropDebug = false;

    // Properties refresh
    double propertyLastRefreshTime = 0.;
    bool propAutoRefresh = true;
    bool propAutoRefreshIncremental = false;
    bool propRefresh = true;

    // Property & Controls Debug
    AwPtpProperty* selectedProperty = nullptr;
    AwPtpControl* selectedControl = nullptr;
    AwPtpPropValue selectedControlValue;
    AwPtpPropValue selectedPropValue;
    PropTable propTable{};

    // Debug set props
    char debugSetText[512] = {};

    // Text shadows
    char copyrightEdit[512] = {};
    char photographerEdit[512] = {};

    // Live View state
    bool liveViewOpen = false;
    double liveViewLastTime = 0.;
    MMemIO liveViewImage{};
    AwLiveViewFrames liveViewFrames{};
    ImTextureID liveViewImageGLId = 0;
    i32 liveViewImageWidth = 0;
    i32 liveViewImageHeight = 0;
    bool liveFocusOverlay = true;
    bool osdEnabled = false;
    bool osdCaptured = false;
    MMemIO osdImage{};
    ImTextureID osdImageGLId = 0;
    i32 osdImageWidth = 0;
    i32 osdImageHeight = 0;
    i32 liveViewClickAction = LiveViewClickAction_NONE;
    i32 liveViewOverlayMode = LiveViewOverlayMode_NONE;
    float liveViewOverlayColor[3] = { 0, 255, 0 };
    float liveViewOverlayThickness = 0.8f;

    // Remote buttons press features available for current device
    bool remoteButtonsEnabled = false;

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

    // Events
    double eventRefreshTime = 0.;

    bool shutterHalfPress = false;
    bool autoFocusButton = false;

    // Log window
    LogWindow logWindow;

    void RefreshDevices() {
        selectedDeviceIndex = -1;
        AwDeviceList_RefreshList(&deviceList);
    }

    void Connect() {
        if (selectedDeviceIndex != -1 && MArraySize(deviceList.devices) > selectedDeviceIndex) {
            AwDeviceInfo* deviceInfo = deviceList.devices + selectedDeviceIndex;
            DisconnectDevice();
            AwResult r = AwDeviceList_OpenDevice(&deviceList, deviceInfo, &device);
            if (r.code == AW_RESULT_OK) {
                MAllocatorMakeClibHeap(&deviceAllocator);
                deviceAllocator.name = (char*)"AwDevice";
#ifdef M_MEM_DEBUG
                MMemDebugInit(&deviceAllocator);
#endif
                r = AwControl_Init(&aw, device, &deviceAllocator);
                if (r.code == AW_RESULT_OK) {
                    r = AwControl_Connect(&aw, selectedProtoVersion ? SDI_EXTENSION_VERSION_300 : SDI_EXTENSION_VERSION_200);
                    if (r.code == AW_RESULT_OK) {
                        connected = true;
                    }
                }
            }
        }
        propTable.reset();
        if (connected) {
            cameraSettingsSaveEnabled = AwControl_PropertyEnabledByCode(&aw, DPC_CAMERA_SETTING_SAVE_ENABLED);
            cameraSettingsReadEnabled = AwControl_PropertyEnabledByCode(&aw, DPC_CAMERA_SETTING_READ_ENABLED);
            remoteButtonsEnabled = AwControl_RemoteButtonEnable(&aw);

            AwPtpProperty* photographerProperty = AwControl_GetPropertyByCode(&aw, DPC_PHOTOGRAPHER);
            if (photographerProperty) {
                size_t size = photographerProperty->value.str.size;
                if (size >= sizeof(photographerEdit)) {
                    size = sizeof(photographerEdit) - 1;
                }
                memcpy(photographerEdit, photographerProperty->value.str.str, size);
                photographerEdit[size] = 0;
            }

            AwPtpProperty* copyrightProperty = AwControl_GetPropertyByCode(&aw, DPC_COPYRIGHT);
            if (copyrightProperty) {
                size_t size = copyrightProperty->value.str.size;
                if (size >= sizeof(copyrightEdit)) {
                    size = sizeof(copyrightEdit) - 1;
                }
                memcpy(copyrightEdit, copyrightProperty->value.str.str, size);
                copyrightEdit[size] = 0;
            }
        }
    }

    void Disconnect() {
        DisconnectDevice();
        cameraSettingsReadEnabled = false;
        cameraSettingsSaveEnabled = false;
        connected = false;
        liveViewOpen = false;
        osdEnabled = false;
        osdCaptured = false;
        propAutoRefresh = true;
        propRefresh = true;
        propertyLastRefreshTime = 0.;
        selectedControl = nullptr;
        selectedControl = nullptr;
        selectedProperty = nullptr;
        selectedProperty = nullptr;
        showWindowDebugPropertyOrControl = false;
        showWindowDeviceDebug = true;
        showWindowPropDebug = false;
        shutterHalfPress = false;
        propTable.reset();
    }

    void DisconnectDevice() {
        if (device != NULL) {
            MMemFree(&this->liveViewImage);
            MMemFree(&this->osdImage);
            AwControl_FreeLiveViewFrames(&aw, &liveViewFrames);
            AwControl_Cleanup(&aw);
            AwDeviceList_CloseDevice(&deviceList, device);
#ifdef M_MEM_DEBUG
            MMemDebugDeinit(&deviceAllocator);
#endif
            device = NULL;
        }
    }

    void CleanupAll() {
        DisconnectDevice();
        AwDeviceList_Close(&deviceList);
    }

    void GraphicsCleanup()
    {
        TextureRelease(&liveViewImageGLId);
        TextureRelease(&osdImageGLId);
    }
};
