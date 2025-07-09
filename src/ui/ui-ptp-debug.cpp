#include <string>
#include <SDL3/SDL_time.h>

#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"
#include "ui-ptp-debug.h"
#include "ui/ui-live-view.h"

#include "ptp/ptp-control.h"
#include "platform/windows/ptp-backend-wia.h"
#include "platform/windows/ptp-backend-libusbk.h"


// TODO:
//   Build list of property editors / widgets?
//      - Debug Set widgets
//      - Enum List Read only support (except in debug)
//   Remove sorting for other tabs
//   Connectors:
//     OSX
//     Linux
//     IP
//   Timing / Position
//   Connect multiple cameras
//   Live View
//     - Focus Area
//     - Click to focus
//     - Click to zoom
//   Capture Button
//   Event callback?
//   Logging levels

template<typename T>
bool ImGuiIntInput1(const char* label, T* value, const char* format, bool isSigned, i64 min, i64 max, int base=10) {
    // Buffer for hex input (max length depends on integer type)
    char buffer[32];

    // Convert current value to uppercase hex string
    snprintf(buffer, sizeof(buffer), format, *value);

    // ImGui input with hex-specific flags
    ImGuiInputTextFlags flags = ImGuiInputTextFlags_CharsDecimal | ImGuiInputTextFlags_CharsNoBlank;
    if (base == 16) {
        flags = ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_CharsNoBlank;
    }
    if (ImGui::InputText(label, buffer, sizeof(buffer), flags)) {
        try {
            std::string input(buffer);
            // Parse hex string
            if (isSigned) {
                long long parsed_value = std::stoll(input, nullptr, base);
                if (parsed_value >= min && parsed_value <= max) {
                    *value = static_cast<T>(parsed_value);
                    return true;
                }
            } else {
                unsigned long long parsed_value = std::stoull(input, nullptr, base);
                if (parsed_value >= min && parsed_value <= max) {
                    *value = static_cast<T>(parsed_value);
                    return true;
                }
            }
        }
        catch (...) {
            // Parsing failed, do nothing
        }
    }

    return false;
}

bool ImGuiInputInt(const char* label, u16 dataType, PropValue* value) {
    switch (dataType) {
        case PTP_DT_UINT8:
            return ImGuiIntInput1(label, &value->u8, "%hhu", false, 0, 0xff);
        case PTP_DT_INT8:
            return ImGuiIntInput1(label, &value->i8, "%hhd", true, -0x80, 0x7f);
        case PTP_DT_UINT16:
            return ImGuiIntInput1(label, &value->u16, "%hu", false, 0, 0xffff);
        case PTP_DT_INT16:
            return ImGuiIntInput1(label, &value->i16, "%hd", true, -0x8000, 0x7fff);
        case PTP_DT_UINT32:
            return ImGuiIntInput1(label, &value->u32, "%u", false, 0, 0xffffffff);
        case PTP_DT_INT32:
            return ImGuiIntInput1(label, &value->i32, "%d", true, -0x80000000, 0x7fffffff);
    }
    return false;
}

bool ImGuiInputHex(const char* label, u16 dataType, PropValue* value) {
    switch (dataType) {
        case PTP_DT_UINT8:
            return ImGuiIntInput1(label, &value->u8, "%02hhx", false, 0, 0xff);
        case PTP_DT_INT8:
            return ImGuiIntInput1(label, &value->i8, "%02hhx", true, -0x80, 0x7f);
        case PTP_DT_UINT16:
            return ImGuiIntInput1(label, &value->u16, "%04hx", false, 0, 0xffff);
        case PTP_DT_INT16:
            return ImGuiIntInput1(label, &value->i16, "%04hx", true, -0x8000, 0x7fff);
        case PTP_DT_UINT32:
            return ImGuiIntInput1(label, &value->u32, "%08x", false, 0, 0xffffffff);
        case PTP_DT_INT32:
            return ImGuiIntInput1(label, &value->i32, "%08x", true, -0x80000000, 0x7fffffff);
    }
    return false;
}

bool ImGuiInputInt(const char* label, u16 dataType, PropValue* value, PropValue min, PropValue max) {
    switch (dataType) {
        case PTP_DT_UINT8:
            return ImGuiIntInput1(label, &value->u8, "%hhu", false, min.u8, max.u8);
        case PTP_DT_INT8:
            return ImGuiIntInput1(label, &value->i8, "%hhd", true, min.i8, max.i8);
        case PTP_DT_UINT16:
            return ImGuiIntInput1(label, &value->u16, "%hu", false, min.u16, max.u16);
        case PTP_DT_INT16:
            return ImGuiIntInput1(label, &value->i16, "%hd", true, min.i16, max.i16);
        case PTP_DT_UINT32:
            return ImGuiIntInput1(label, &value->u32, "%u", false, min.u32, max.u32);
        case PTP_DT_INT32:
            return ImGuiIntInput1(label, &value->i32, "%d", true, min.i32, max.i32);
    }
    return false;
}

bool ImGuiInputHex(const char* label, u16 dataType, PropValue* value, PropValue min, PropValue max) {
    switch (dataType) {
        case PTP_DT_UINT8:
            return ImGuiIntInput1(label, &value->u8, "%02hhx", false, min.u8, max.u8);
        case PTP_DT_INT8:
            return ImGuiIntInput1(label, &value->i8, "%02hhx", true, min.i8, max.i8);
        case PTP_DT_UINT16:
            return ImGuiIntInput1(label, &value->u16, "%04hx", false, min.u16, max.u16);
        case PTP_DT_INT16:
            return ImGuiIntInput1(label, &value->i16, "%04hx", true, min.i16, max.i16);
        case PTP_DT_UINT32:
            return ImGuiIntInput1(label, &value->u32, "%08x", false, min.u32, max.u32);
        case PTP_DT_INT32:
            return ImGuiIntInput1(label, &value->i32, "%08x", true, min.i32, max.i32);
    }
    return false;
}

void ImGuiInputIntDuel(u16 dataType, PropValue* value) {
    float windowWidth = ImGui::GetWindowWidth();
    float inputWidth = (windowWidth * 0.3f) - 10.0f;
    ImGui::SetNextItemWidth(inputWidth);
    ImGuiInputInt("dec", dataType, value);
    ImGui::SameLine();
    ImGui::SetNextItemWidth(inputWidth);
    ImGuiInputHex("hex", dataType, value);
}

void ImGuiInputIntDuel(u16 dataType, PropValue* value, PropValue min, PropValue max) {
    float windowWidth = ImGui::GetWindowWidth();
    float inputWidth = (windowWidth * 0.3f) - 10.0f;
    ImGui::SetNextItemWidth(inputWidth);
    ImGuiInputInt("dec", dataType, value, min, max);
    ImGui::SameLine();
    ImGui::SetNextItemWidth(inputWidth);
    ImGuiInputHex("hex", dataType, value, min, max);
}

bool ImGuiSlider(u16 dataType, PropValue* value, PropValue minV, PropValue stepV, PropValue maxV) {
    int v = 0;
    int min = 0;
    int max = 0;
    int step = 0;
    bool displaySlider = false;
    switch (dataType) {
        case PTP_DT_INT8:
            min = minV.i8;
            step = stepV.i8;
            max = maxV.i8;
            v = value->i8;
            displaySlider = true;
            break;
        case PTP_DT_UINT8:
            min = minV.u8;
            step = stepV.u8;
            max = maxV.u8;
            v = value->u8;
            displaySlider = true;
            break;
        case PTP_DT_INT16:
            min = minV.i16;
            step = stepV.i16;
            max = maxV.i16;
            v = value->i16;
            displaySlider = true;
            break;
        case PTP_DT_UINT16:
            min = minV.u16;
            step = stepV.u16;
            max = maxV.u16;
            v = value->u16;
            displaySlider = true;
            break;
        case PTP_DT_INT32:
            min = minV.i32;
            step = stepV.i32;
            max = maxV.i32;
            v =value->i32;
            displaySlider = true;
            break;
    }

    if (displaySlider) {
        int orig = v;

        // Create the slider with the AlwaysSnap flag to force stepping
        if (ImGui::SliderInt("##stepped_slider", &v, min, max, "%d",
                             ImGuiSliderFlags_AlwaysClamp)) {
            // Round to nearest step
            v = min + ((v - min + step / 2) / step) * step;

            // Clamp to ensure we stay within bounds
            v = ImClamp(v, min, max);
            if (orig != v) {
                switch (dataType) {
                    case PTP_DT_INT8:
                        value->i8 = v;
                        return true;
                    case PTP_DT_UINT8:
                        value->u8 = v;
                        return true;
                    case PTP_DT_INT16:
                        value->i16 = v;
                        return true;
                    case PTP_DT_UINT16:
                        value->u16 = v;
                        return true;
                    case PTP_DT_INT32:
                        value->i32 = v;
                        return true;
                    case PTP_DT_UINT32:
                        value->u32 = v;
                        return true;
                    case PTP_DT_INT64:
                        value->i64 = v;
                        return true;
                    case PTP_DT_UINT64:
                        value->u64 = v;
                        return true;
                    default:
                        return false;
                }
            }
        }
    }
    return false;
}

void ShowDebugExtendedPropWindow(AppContext& c, PtpProperty *property) {
    char* label = Ptp_GetPropertyStr(property->propCode);
    ImGui::AlignTextToFramePadding();
    ImGui::Text("%s", label);
    ImGui::SameLine();
    ImGui::TextDisabled("0x%04X", property->propCode);
    label = Ptp_GetDataTypeStr((PTPDataType)property->dataType);
    ImGui::SameLine();
    ImGui::TextDisabled("%s", label);
    ImGui::SameLine();
    ImGui::Checkbox("Debug", &c.showWindowPropDebug);
    ImGui::Separator();

    MStr propAsString = {};
    if (PTPControl_GetPropertyAsStr(&c.ptp, property->propCode, &propAsString)) {
        ImGui::Text("Value: %s", propAsString.str);
        if (propAsString.size) {
            MStrFree(propAsString);
        }
    } else {
        char text[32];
        Ptp_GetPropValueStr((PTPDataType)property->dataType, property->value, text, sizeof(text));
        ImGui::Text("Value: %s", text);
    }

    ImGui::Separator();

    if (property->formFlag == PTP_FORM_FLAG_ENUM) {
        ImGuiTableFlags flags =
                ImGuiTableFlags_Resizable |
                ImGuiTableFlags_Reorderable |
                ImGuiTableFlags_Hideable |
                ImGuiTableFlags_RowBg |
                ImGuiTableFlags_BordersOuter |
                ImGuiTableFlags_BordersV |
                ImGuiTableFlags_ScrollY;

        PropValueEnums outEnums{};

        if (PTPControl_GetEnumsForProperty(&c.ptp, property->propCode, &outEnums) && MArraySize(outEnums.values)) {
            if (ImGui::BeginTable("Properties", 3, flags)) {
                ImGui::TableSetupColumn("Option", ImGuiTableColumnFlags_WidthStretch);
                ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthFixed,130.0);
                ImGui::TableSetupColumn("RW",ImGuiTableColumnFlags_WidthFixed);
                ImGui::TableHeadersRow();

                char text[32];
                for (size_t i = 0; i < MArraySize(outEnums.values); ++i) {
                    ImGui::TableNextRow();
                    ImGui::PushID(i);

                    PropValueEnum *valueEnum = outEnums.values + i;
                    Ptp_GetPropValueStr((PTPDataType)property->dataType, valueEnum->propValue, text, sizeof(text));

                    ImGuiSelectableFlags selectFlags =
                            ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowOverlap;

                    bool selected = Ptp_PropValueEq((PTPDataType)property->dataType,
                                                    valueEnum->propValue, property->value);

                    ImGui::TableNextColumn();
                    char* str = valueEnum->str.str;
                    if (str == NULL) {
                        str = text;
                    }

                    if (ImGui::Selectable(str, selected, selectFlags)) {
                        PTPControl_SetProperty(&c.ptp, property->propCode, valueEnum->propValue);
                    }

                    ImGui::TableNextColumn();
                    ImGui::Text("%s", text);

                    ImGui::TableNextColumn();
                    char rStatus = '-';
                    char wStatus = '-';
                    if (valueEnum->flags & ENUM_VALUE_READ) {
                        rStatus = 'R';
                    }
                    if (valueEnum->flags & ENUM_VALUE_WRITE) {
                        wStatus = 'W';
                    }
                    ImGui::Text("%c%c", rStatus, wStatus);

                    ImGui::PopID();
                }

                ImGui::EndTable();
            }
        }

        Ptp_FreePropValueEnums(&outEnums);
    }
    else if (property->formFlag == PTP_FORM_FLAG_RANGE) {
        if (ImGuiSlider(property->dataType, &property->value, property->form.range.min, property->form.range.step, property->form.range.max)) {
            PTPControl_SetProperty(&c.ptp, property->propCode, property->value);
        }
    }

    if (c.showWindowPropDebug) {
        char text[32];

        Ptp_GetPropValueStr((PTPDataType) property->dataType, property->defaultValue, text,
                            sizeof(text));
        ImGui::Text("Default: %s", text);

        ImGuiInputIntDuel(property->dataType, &property->value);
        ImGui::SameLine();
        if (ImGui::Button("Send")) {
            PTPControl_SetProperty(&c.ptp, property->propCode, property->value);
        }

        switch (property->formFlag) {
            case PTP_FORM_FLAG_RANGE: {
                Ptp_GetPropValueStr((PTPDataType) property->dataType, property->form.range.min, text,
                                    sizeof(text));
                ImGui::Text("Min: %s", text);
                Ptp_GetPropValueStr((PTPDataType) property->dataType, property->form.range.max, text,
                                    sizeof(text));
                ImGui::Text("Max: %s", text);
                Ptp_GetPropValueStr((PTPDataType) property->dataType, property->form.range.step, text,
                                    sizeof(text));
                ImGui::Text("Step: %s", text);
                break;
            }
            case PTP_FORM_FLAG_ENUM: {
                ImGui::Text("Set Enum Values:");
                for (size_t i = 0; i < MArraySize(property->form.enums.set); ++i) {
                    PropValue v = property->form.enums.set[i];
                    Ptp_GetPropValueStr((PTPDataType) property->dataType, v, text, sizeof(text));
                    ImGui::Text("    %s", text);
                }
                ImGui::Text("Get/Set Enum Values:");
                for (size_t i = 0; i < MArraySize(property->form.enums.getSet); ++i) {
                    PropValue v = property->form.enums.getSet[i];
                    Ptp_GetPropValueStr((PTPDataType) property->dataType, v, text, sizeof(text));
                    ImGui::Text("    %s", text);
                }
                break;
            }
        }
    }
}

void ShowDebugExtendedControlWindow(AppContext& c, PtpControl *control) {
    ImGui::AlignTextToFramePadding();
    ImGui::Text("%s", control->name);
    ImGui::SameLine();
    ImGui::TextDisabled("0x%04X", control->controlCode);
    char* label = Ptp_GetDataTypeStr((PTPDataType)control->dataType);
    ImGui::SameLine();
    ImGui::TextDisabled("%s", label);
    ImGui::SameLine();
    ImGui::Checkbox("Debug", &c.showWindowPropDebug);
    ImGui::Separator();

    if (control->formFlag == PTP_FORM_FLAG_ENUM) {
        ImGuiTableFlags flags =
                ImGuiTableFlags_Resizable |
                ImGuiTableFlags_Reorderable |
                ImGuiTableFlags_Hideable |
                ImGuiTableFlags_RowBg |
                ImGuiTableFlags_BordersOuter |
                ImGuiTableFlags_BordersV;

        if (control->form.enums.size) {
            if (ImGui::BeginTable("Properties", 3, flags)) {
                ImGui::TableSetupColumn("Option", ImGuiTableColumnFlags_WidthStretch);
                ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthFixed,130.0);
                ImGui::TableHeadersRow();

                char text[32];
                for (size_t i = 0; i < control->form.enums.size; ++i) {
                    ImGui::TableNextRow();
                    ImGui::PushID(i);

                    PropValueEnum *valueEnum = control->form.enums.values + i;
                    Ptp_GetPropValueStr((PTPDataType)control->dataType, valueEnum->propValue, text, sizeof(text));

                    ImGuiSelectableFlags selectFlags =
                            ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowOverlap;

                    ImGui::TableNextColumn();
                    const char* str = valueEnum->str.str;
                    if (str == NULL) {
                        str = text;
                    }

                    if (ImGui::Selectable(str, false, selectFlags)) {
                        PTPControl_SetControl(&c.ptp, control->controlCode, valueEnum->propValue);
                    }

                    ImGui::TableNextColumn();
                    ImGui::Text("%s", text);

                    ImGui::PopID();
                }

                ImGui::EndTable();
            }
        }

        if (c.showWindowPropDebug) {
            ImGuiInputIntDuel(control->dataType, &c.selectedControlValue);
            ImGui::SameLine();
            if (ImGui::Button("Set")) {
                PTPControl_SetControl(&c.ptp, control->controlCode, c.selectedControlValue);
            }
        }
    } else if (control->formFlag == PTP_FORM_FLAG_RANGE) {
        char text[32];
        Ptp_GetPropValueStr((PTPDataType)control->dataType, control->form.range.min, text, sizeof(text));
        ImGui::Text("Min: %s", text);
        Ptp_GetPropValueStr((PTPDataType)control->dataType, control->form.range.max, text, sizeof(text));
        ImGui::Text("Max: %s", text);
        Ptp_GetPropValueStr((PTPDataType)control->dataType, control->form.range.step, text, sizeof(text));
        ImGui::Text("Step: %s", text);

        ImGuiSlider(control->dataType, &c.selectedControlValue, control->form.range.min, control->form.range.step, control->form.range.max);

        if (c.showWindowPropDebug) {
            ImGuiInputIntDuel(control->dataType, &c.selectedControlValue);
        } else {
            ImGuiInputIntDuel(control->dataType, &c.selectedControlValue, control->form.range.min, control->form.range.max);
        }

        ImGui::SameLine();

        if (ImGui::Button("Set")) {
            PTPControl_SetControl(&c.ptp, control->controlCode, c.selectedControlValue);
        }
        // Display current value
    }
}

static void ShowDebugPropertyOrControl(AppContext& c) {
    ImGui::Begin("Debug Info", &c.showWindowDebugPropertyOrControl);
    PtpProperty *property = c.selectedProperty;
    PtpControl *control = c.selectedControl;
    if (property) {
        ShowDebugExtendedPropWindow(c, property);
    } else if (control) {
        ShowDebugExtendedControlWindow(c, control);
    }

    ImGui::End();
}

void ShowDebugPropertyListTab(AppContext& c) {
    if (ImGui::BeginTabItem("Properties")) {
        ImGui::Text("Properties: %lld", c.propTable.items.size());

        ImGui::SameLine();
        if (ImGui::InputText("Search", c.propTable.searchText, sizeof(c.propTable.searchText))) {
            c.propTable.needsRebuild = true;
        }

        ImGui::SameLine();
        if (ImGui::Button("Clear")) {
            c.propTable.searchText[0] = 0;
            c.propTable.needsRebuild = true;
        }

        ImGui::SameLine();

        if (c.liveViewLastTime == 0) {
            c.liveViewLastTime = ImGui::GetTime();
        }

        if (ImGui::Button("Refresh")) {
            c.propAutoRefresh = true;
        }

        ImGui::Checkbox("Auto Refresh", &c.propAutoRefresh);

        ImGuiTableFlags flags =
                ImGuiTableFlags_Sortable |
                ImGuiTableFlags_Resizable |
                ImGuiTableFlags_Reorderable |
                ImGuiTableFlags_Hideable |
                ImGuiTableFlags_RowBg |
                ImGuiTableFlags_BordersOuter |
                ImGuiTableFlags_BordersV |
                ImGuiTableFlags_ScrollY |
                ImGuiTableFlags_SortMulti;

        if (ImGui::BeginTable("Properties", 8, flags)) {
            ImGui::TableSetupColumn("Code",
                                    ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_DefaultSort,
                                    60.0);
            ImGui::TableSetupColumn("Name",
                                    ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_DefaultSort,
                                    200.0);
            ImGui::TableSetupColumn("Type",
                                    ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_DefaultSort);
            ImGui::TableSetupColumn("Form",
                                    ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_DefaultSort);
            ImGui::TableSetupColumn("Get/Set",
                                    ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_DefaultSort);
            ImGui::TableSetupColumn("Enabled",
                                    ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_DefaultSort);
            ImGui::TableSetupColumn("Raw Value", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_DefaultSort);
            ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableHeadersRow();

            // Handle sorting
            ImGuiTableSortSpecs *sortsSpecs = ImGui::TableGetSortSpecs();
            if (sortsSpecs) {
                if (sortsSpecs->SpecsDirty) {
                    c.propTable.sortSpecs = sortsSpecs;
                    sortsSpecs->SpecsDirty = false;
                    c.propTable.needsSort = true;
                }
            }

            if (c.propTable.needsSort) {
                c.propTable.sort();
            }

            int i = 0;
            char text[32];
            for (const auto& uiPtpProperty : c.propTable.items) {
                ImGui::TableNextRow();
                ImGui::TableNextColumn();

                PtpProperty *property = uiPtpProperty.prop;
                ImGuiSelectableFlags selectFlags =
                        ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowOverlap;
                ImGui::PushID(i);

                if (ImGui::Selectable(uiPtpProperty.propCode, c.selectedProperty == property,
                                      selectFlags)) {
                    c.selectedProperty = property;
                    c.selectedControl = nullptr;
                    c.selectedControlValue = {};
                    c.showWindowDebugPropertyOrControl = true;
                }

                ImGui::TableNextColumn();
                ImGui::Text(uiPtpProperty.propName);

                ImGui::TableNextColumn();
                char *dataTypeName = Ptp_GetDataTypeStr((PTPDataType) property->dataType);
                if (dataTypeName) {
                    ImGui::Text(dataTypeName);
                }

                ImGui::TableNextColumn();
                char *formTypeName = Ptp_GetFormFlagStr((PTPFormFlag) property->formFlag);
                if (formTypeName) {
                    ImGui::Text(formTypeName);
                }

                ImGui::TableNextColumn();
                sprintf(text, "%02x", property->getSet);
                ImGui::Text(text);

                ImGui::TableNextColumn();
                char *isEnabled = Ptp_GetPropIsEnabledStr(property->isEnabled);
                if (isEnabled) {
                    ImGui::Text(isEnabled);
                } else {
                    sprintf(text, "%02x", property->isEnabled);
                    ImGui::Text("%s", text);
                }

                ImGui::TableNextColumn();
                Ptp_GetPropValueStr((PTPDataType) property->dataType, property->value, text,
                                    sizeof(text));
                ImGui::Text("%s", text);

                ImGui::TableNextColumn();
                MStr propAsString = {};
                if (PTPControl_GetPropertyAsStr(&c.ptp, property->propCode, &propAsString)) {
                    ImGui::Text("%s", propAsString.str);
                    if (propAsString.size) {
                        MStrFree(propAsString);
                    }
                }

                ImGui::PopID();
                i++;
            }

            ImGui::EndTable();
        }

        ImGui::EndTabItem();
    }
}

void ShowCameraControlsWindow(AppContext& c) {
    ImGui::Begin("Camera Controls");

    ImGui::Text("Model: %s", c.ptp.model.str);
    ImGui::SameLine();
    ImGui::Text("Serial Number: %s", c.ptp.serialNumber.str);

    ImGui::Checkbox("LiveView", &c.liveViewOpen);
    ImGui::SameLine();
    ImGui::Checkbox("Inspect Controls", &c.showWindowDeviceDebug);

    if (ImGui::Button("Shutter")) {
        PTPControl_SetControlToggle(&c.ptp, DPC_SHUTTER, false);
    }
    if (ImGui::IsItemHovered()) {
        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
            PTPControl_SetControlToggle(&c.ptp, DPC_SHUTTER, true);
        }
    }

    ImGui::SameLine();
    if (ImGui::Checkbox("Half-Press", &c.shutterHalfPress)) {
        PTPControl_SetControlToggle(&c.ptp, DPC_SHUTTER_HALF_PRESS, c.shutterHalfPress);
    }
    ImGui::SameLine();

    if (ImGui::Checkbox("Auto-Focus", &c.autoFocusButton)) {
        PTPControl_SetControlToggle(&c.ptp, DPC_AUTO_FOCUS_HOLD, c.autoFocusButton);
    }

    ImGui::Spacing();

    if (ImGui::CollapsingHeader("Capture Files", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Text("Files On Camera: %d", PtpControl_GetPendingFiles(&c.ptp));
        ImGui::Text("Last Download Time: %lldms (%lldms)", c.fileDownloadTotalMillis, c.fileDownloadTimeMillis);
        ImGui::Text("Last Filename: %s", c.fileDownloadPath.c_str());
        ImGui::SameLine();
        ImGui::Text("Size: %llu", c.fileDownloadTotalBytes);

        ImGui::Checkbox("Auto Download", &c.fileDownloadAuto);

        ImGui::SameLine();

        bool fileDownload = false;
        if (c.fileDownloadAuto && PtpControl_GetPendingFiles(&c.ptp)) {
            fileDownload = true;
        }

        if (c.fileDownloadAuto) {
            ImGui::BeginDisabled();
        }

        if (ImGui::Button("Download")) {
            fileDownload = true;
        }

        if (c.fileDownloadAuto) {
            ImGui::EndDisabled();
        }

        if (fileDownload) {
            MMemIO fileContents{};
            PtpCapturedImageInfo cii = {};
            SDL_Time startTime = 0;
            SDL_GetCurrentTime(&startTime);
            PTPResult r = PtpControl_GetCapturedImage(&c.ptp, &fileContents, &cii);
            SDL_Time dlTime = 0;
            SDL_GetCurrentTime(&dlTime);
            if (r != PTP_OK) {
                MLogf("Error fetching image from camera: %d", r);
            } else {
                MFileWriteDataFully(cii.filename.str, fileContents.mem, fileContents.size);
                SDL_Time writeTime = 0;
                SDL_GetCurrentTime(&writeTime);
                c.fileDownloadTotalMillis = (writeTime - startTime) / (1000*1000);
                c.fileDownloadTimeMillis = (dlTime - startTime) / (1000*1000);
                MLogf("Total image save time %lld (dl time %lld)",
                    c.fileDownloadTotalMillis, c.fileDownloadTimeMillis);
                c.fileDownloadTotalBytes = fileContents.size;
                c.fileDownloadPath = cii.filename.str;
                MMemFree(&fileContents);
                MStrFree(cii.filename);
            }
        }

        if (c.device->backendType == PTP_BACKEND_LIBUSBK) {
            if (ImGui::Button("Read Event")) {
                PTPUsbkDevice_ReadEvent(c.device);
            }
        }
    }

    ImGui::Spacing();

    if (ImGui::CollapsingHeader("Settings File")) {
        bool showSettingsFileInput = false;

        if (c.cameraSettingsReadEnabled) {
            if (ImGui::Button("Load")) {
                MReadFileRet file = MFileReadFully(c.cameraSettingsPathBuffer);
                if (file.size > 0) {
                    MMemIO memIo{};
                    MMemInit(&memIo, file.data, file.size);
                    memIo.size = file.size;
                    PTPResult r = PtpControl_PutCameraSettingsFile(&c.ptp, &memIo);
                    if (r != PTP_OK) {
                        MLogf("Error uploading file %s", c.cameraSettingsPathBuffer);
                    }
                    MMemFree(&memIo);
                }
                showSettingsFileInput = true;
            }
        }
        if (c.cameraSettingsSaveEnabled) {
            ImGui::SameLine();
            if (ImGui::Button("Save")) {
                MMemIO memIo{};
                PTPResult r = PtpControl_GetCameraSettingsFile(&c.ptp, &memIo);
                if (r != PTP_OK) {
                    MLogf("Error fetching settings from camera: %d", r);
                } else {
                    MFileWriteDataFully(c.cameraSettingsPathBuffer, memIo.mem, memIo.size);
                    MMemFree(&memIo);
                }
            }
            showSettingsFileInput = true;
        }
        if (showSettingsFileInput) {
            ImGui::InputText("Settings File Path", c.cameraSettingsPathBuffer, sizeof(c.cameraSettingsPathBuffer));
        } else {
            ImGui::Text("Remote Read/Save settings not supported");
        }
    }

    ImGui::End();
}

const float AUTO_REFRESH_INTERVAL_SECS = 2.0f;

static void RefreshProperties(AppContext& c) {
    if (c.propAutoRefresh) {
        double currentTime = ImGui::GetTime();
        if (currentTime - c.propertyLastRefreshTime >= AUTO_REFRESH_INTERVAL_SECS) {
            c.propRefresh = true;
        }
    }

    if (c.propRefresh) {
        PtpControl_UpdateProperties(&c.ptp);
        c.propertyLastRefreshTime = ImGui::GetTime();
        c.propTable.needsRebuild = true;
    }

    if (c.propTable.needsRebuild) {
        c.propTable.rebuild(c.ptp);
        c.propTable.needsSort = true;
    }
}

static void ShowMainDeviceDebugWindow(AppContext& c) {
    ImGui::Begin("Inspect Controls", &c.showWindowDeviceDebug);
    if (c.connected) {
        if (ImGui::BeginTabBar("Device Info Tabs")) {
            if (ImGui::BeginTabItem("Info")) {
                int majorVersion = c.ptp.protocolVersion / 100;
                int minorVersion = c.ptp.protocolVersion % 100;
                ImGui::Text("Protocol Version: %d.%02d", majorVersion, minorVersion);

                ImGui::Text("Manufacturer: %s", c.ptp.manufacturer.str);
                ImGui::Text("Model: %s", c.ptp.model.str);
                ImGui::Text("Device Version: %s", c.ptp.deviceVersion.str);
                ImGui::Text("Serial Number: %s", c.ptp.serialNumber.str);

                ImGui::Text("Vendor Extension Id: %d", c.ptp.vendorExtensionId);
                majorVersion = c.ptp.vendorExtensionVersion / 100;
                minorVersion = c.ptp.vendorExtensionVersion % 100;
                ImGui::Text("Vendor Extension Version: %d.%02d", majorVersion, minorVersion);
                ImGui::Text("Vendor Extension: %s", c.ptp.vendorExtension.str);

                ImGuiTableFlags flags =
                        ImGuiTableFlags_Sortable |
                        ImGuiTableFlags_Resizable |
                        ImGuiTableFlags_Reorderable |
                        ImGuiTableFlags_Hideable |
                        ImGuiTableFlags_RowBg |
                        ImGuiTableFlags_BordersOuter |
                        ImGuiTableFlags_BordersV;

                ImGui::Text("Image Formats:");
                if (ImGui::BeginTable("Image Formats", 2, flags)) {
                    ImGui::TableSetupColumn("Code",
                                            ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_DefaultSort,
                                            120.0);
                    ImGui::TableSetupColumn("Format",
                                            ImGuiTableColumnFlags_WidthStretch | ImGuiTableColumnFlags_DefaultSort);
                    ImGui::TableHeadersRow();

                    for (size_t i = 0; i < MArraySize(c.ptp.imageFormats); i++) {
                        ImGui::TableNextRow();

                        u16 imageFormat = c.ptp.imageFormats[i];

                        ImGui::TableNextColumn();

                        char label[32];
                        sprintf(label, "0x%04x", imageFormat);

                        ImGui::SameLine();
                        ImGui::Text(label);

                        ImGui::TableNextColumn();
                        char *objectFormatStr = Ptp_GetObjectFormatStr(imageFormat);
                        if (objectFormatStr == NULL) {
                            objectFormatStr = label;
                        }
                        ImGui::Text(objectFormatStr);
                    }

                    ImGui::EndTable();
                }

                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Operations")) {
                ImGui::Text("Operations: %lld", MArraySize(c.ptp.supportedOperations));

                ImGuiTableFlags flags =
                        ImGuiTableFlags_Sortable |
                        ImGuiTableFlags_Resizable |
                        ImGuiTableFlags_Reorderable |
                        ImGuiTableFlags_Hideable |
                        ImGuiTableFlags_RowBg |
                        ImGuiTableFlags_BordersOuter |
                        ImGuiTableFlags_BordersV |
                        ImGuiTableFlags_ScrollY |
                        ImGuiTableFlags_SortMulti;

                if (ImGui::BeginTable("Operations", 2, flags)) {
                    ImGui::TableSetupColumn("Code",
                                            ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_DefaultSort,
                                            120.0);
                    ImGui::TableSetupColumn("Name",
                                            ImGuiTableColumnFlags_WidthStretch | ImGuiTableColumnFlags_DefaultSort);
                    ImGui::TableHeadersRow();

                    for (int i = 0; i < MArraySize(c.ptp.supportedOperations); i++) {
                        ImGui::TableNextRow();

                        u16 propertyCode = c.ptp.supportedOperations[i];

                        ImGui::TableNextColumn();

                        char label[32];
                        sprintf(label, "0x%04x", propertyCode);

                        ImGui::SameLine();
                        ImGui::Text(label);

                        ImGui::TableNextColumn();
                        char *opName = Ptp_GetOperationStr(propertyCode);
                        if (opName == NULL) {
                            opName = label;
                        }
                        ImGui::Text(opName);
                    }

                    ImGui::EndTable();
                }

                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Controls")) {
                ImGui::Text("Controls: %lld", MArraySize(c.ptp.supportedControls));

                ImGuiTableFlags flags =
                        ImGuiTableFlags_Sortable |
                        ImGuiTableFlags_Resizable |
                        ImGuiTableFlags_Reorderable |
                        ImGuiTableFlags_Hideable |
                        ImGuiTableFlags_RowBg |
                        ImGuiTableFlags_BordersOuter |
                        ImGuiTableFlags_BordersV |
                        ImGuiTableFlags_ScrollY |
                        ImGuiTableFlags_SortMulti;

                if (ImGui::BeginTable("Controls", 4, flags)) {
                    ImGui::TableSetupColumn("Code",
                                            ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_DefaultSort,
                                            120.0);
                    ImGui::TableSetupColumn("Name",
                                            ImGuiTableColumnFlags_WidthStretch | ImGuiTableColumnFlags_DefaultSort);
                    ImGui::TableSetupColumn("Type",
                                            ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_DefaultSort);
                    ImGui::TableSetupColumn("Form",
                                            ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_DefaultSort);
                    ImGui::TableHeadersRow();

                    for (size_t i = 0; i < MArraySize(c.ptp.controls); i++) {
                        ImGui::TableNextRow();

                        PtpControl* control = c.ptp.controls + i;
                        u16 controlCode = control->controlCode;

                        ImGui::TableNextColumn();
                        ImGuiSelectableFlags selectFlags =
                                ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowOverlap;
                        ImGui::PushID(i);

                        char label[32];
                        sprintf(label, "0x%04x", controlCode);

                        if (ImGui::Selectable(label, c.selectedControl == control, selectFlags)) {
                            c.selectedControlValue = {};
                            c.selectedControl = control;
                            c.selectedProperty = nullptr;
                            c.showWindowDebugPropertyOrControl = true;
                        }

                        ImGui::TableNextColumn();
                        const char* name = control->name;
                        if (!name) {
                            name = label;
                        }
                        ImGui::Text(name);

                        ImGui::TableNextColumn();
                        const char *dataType = Ptp_GetDataTypeStr((PTPDataType)control->dataType);
                        if (dataType == NULL) {
                            dataType = "";
                        }
                        ImGui::Text(dataType);

                        ImGui::TableNextColumn();
                        const char *formFlagStr = Ptp_GetFormFlagStr((PTPFormFlag)control->formFlag);
                        if (formFlagStr == NULL) {
                            formFlagStr = "";
                        }
                        ImGui::Text(formFlagStr);

                        ImGui::PopID();
                    }

                    ImGui::EndTable();
                }

                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Events")) {
                ImGui::Text("Events: %lld", MArraySize(c.ptp.supportedEvents));

                ImGuiTableFlags flags =
                        ImGuiTableFlags_Sortable |
                        ImGuiTableFlags_Resizable |
                        ImGuiTableFlags_Reorderable |
                        ImGuiTableFlags_Hideable |
                        ImGuiTableFlags_RowBg |
                        ImGuiTableFlags_BordersOuter |
                        ImGuiTableFlags_BordersV |
                        ImGuiTableFlags_ScrollY |
                        ImGuiTableFlags_SortMulti;

                if (ImGui::BeginTable("Events", 2, flags)) {
                    ImGui::TableSetupColumn("Code",
                                            ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_DefaultSort,
                                            120.0);
                    ImGui::TableSetupColumn("Name",
                                            ImGuiTableColumnFlags_WidthStretch | ImGuiTableColumnFlags_DefaultSort);
                    ImGui::TableHeadersRow();

                    for (size_t i = 0; i < MArraySize(c.ptp.supportedEvents); i++) {
                        ImGui::TableNextRow();

                        u16 eventCode = c.ptp.supportedEvents[i];

                        ImGui::TableNextColumn();

                        char label[32];
                        sprintf(label, "0x%04x", eventCode);

                        ImGui::SameLine();
                        ImGui::Text(label);

                        ImGui::TableNextColumn();
                        char *name = Ptp_GetEventStr(eventCode);
                        if (name == NULL) {
                            name = label;
                        }
                        ImGui::Text(name);
                    }

                    ImGui::EndTable();
                }

                ImGui::EndTabItem();
            }

            ShowDebugPropertyListTab(c);

            ImGui::EndTabBar();
        }
    }
    ImGui::End();
}

static void ShowDeviceListWindow(AppContext& c) {
    ImGui::Begin("Device List");

    if (!c.ptpDeviceList.listUpToDate) {
        c.refreshDevices();
    }
    if (c.device && c.device->disconnected) {
        c.disconnect();
    }
    if (ImGui::Button("Refresh")) {
        c.refreshDevices();
    }
    ImGui::SameLine();
    if (ImGui::Button("Connect")) {
        c.connect();
    }
    ImGui::SameLine();
    if (ImGui::Button("Disconnect")) {
        c.disconnect();
    }

    // if (c.device) {
    //     ImGui::SameLine();
    //     if (ImGui::Button("Reset")) {
    //         PTPDeviceList_Reset(&c.ptpDeviceList, c.device);
    //     }
    // }

    ImGui::SameLine();
    if (ImGui::Button("Restart WIA Service")) {
        PTPWiaServiceRestart();
    }

    const char *protoVersions[] = {"2.00", "3.00"};
    ImGui::Combo("Protocol Version", &c.selectedProtoVersion, protoVersions, 2);

    ImGuiTableFlags tableFlags = ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_RowBg |
                                 ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable |
                                 ImGuiTableFlags_Reorderable | ImGuiTableFlags_Hideable;

    if (ImGui::BeginTable("Devices", 2, tableFlags)) {
        ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthFixed, 200.0);
        ImGui::TableSetupColumn("Manufacturer", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableHeadersRow();

        for (size_t i = 0; i < MArraySize(c.ptpDeviceList.devices); i++) {
            ImGui::TableNextRow();

            ImGui::TableNextColumn();

            PTPDeviceInfo *deviceInfo = c.ptpDeviceList.devices + i;
            ImGuiSelectableFlags selectFlags =
                    ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowOverlap;
            ImGui::PushID(i);
            char label[32];
            sprintf(label, "##%04lld", i);
            if (ImGui::Selectable(label, c.selectedDeviceIndex == i, selectFlags)) {
                c.selectedDeviceIndex = i;
            }
            if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0)) {
                c.connect();
            }

            ImGui::SameLine();
            ImGui::Text(deviceInfo->deviceName.str);

            ImGui::TableNextColumn();
            ImGui::Text(deviceInfo->manufacturer.str);

            ImGui::PopID();
        }

        ImGui::EndTable();
    }

    ImGui::End();
}

void UiPtpShow(AppContext& c) {
    ShowDeviceListWindow(c);

    if (c.connected) {
        RefreshProperties(c);

        if (c.showWindowDeviceDebug) {
            ShowMainDeviceDebugWindow(c);
        }

        if (c.showWindowDebugPropertyOrControl) {
            ShowDebugPropertyOrControl(c);
        }

        if (c.liveViewOpen) {
            UiPtpLiveViewShow(c);
        }

        ShowCameraControlsWindow(c);
    }
}
