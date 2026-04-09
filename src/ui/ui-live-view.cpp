#include "ui-live-view.h"

#include "imgui.h"
#include "imgui_impl_opengl3_loader.h"

#define _CRT_SECURE_NO_WARNINGS
#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

#include "ui/ui-context.h"
#include "aw/aw-const.h"

#include "mlib/mlib.h"

// Helper function to draw corner angles for small rectangles
static void DrawRectCorners(ImDrawList *drawList, const ImVec2 &rectMin, const ImVec2 &rectMax, ImU32 color,
                            float thickness, float cornerLength) {
    // Top-left corner
    drawList->AddLine(ImVec2(rectMin.x, rectMin.y), ImVec2(rectMin.x + cornerLength, rectMin.y), color, thickness);
    drawList->AddLine(ImVec2(rectMin.x, rectMin.y), ImVec2(rectMin.x, rectMin.y + cornerLength), color, thickness);

    // Top-right corner
    drawList->AddLine(ImVec2(rectMax.x - cornerLength, rectMin.y), ImVec2(rectMax.x, rectMin.y), color, thickness);
    drawList->AddLine(ImVec2(rectMax.x, rectMin.y), ImVec2(rectMax.x, rectMin.y + cornerLength), color, thickness);

    // Bottom-left corner
    drawList->AddLine(ImVec2(rectMin.x, rectMax.y - cornerLength), ImVec2(rectMin.x, rectMax.y), color, thickness);
    drawList->AddLine(ImVec2(rectMin.x, rectMax.y), ImVec2(rectMin.x + cornerLength, rectMax.y), color, thickness);

    // Bottom-right corner
    drawList->AddLine(ImVec2(rectMax.x, rectMax.y - cornerLength), ImVec2(rectMax.x, rectMax.y), color, thickness);
    drawList->AddLine(ImVec2(rectMax.x - cornerLength, rectMax.y), ImVec2(rectMax.x, rectMax.y), color, thickness);
}

// Simple helper function to load an image into a OpenGL texture with common settings
static bool LoadTextureFromMemory(const MMemIO* memIo, ImTextureID* inOutTexture, i32* outWidth, i32* outHeight)
{
    int imageWidth = 0;
    int imageHeight = 0;
    u8* imageData = stbi_load_from_memory((const u8*)memIo->mem, (int)memIo->size, &imageWidth,
        &imageHeight, NULL, 4);
    if (imageData == NULL)
    {
        return false;
    }

    // Create a OpenGL texture identifier
    GLuint imageTexture = *inOutTexture;

    if (imageTexture)
    {
        glBindTexture(GL_TEXTURE_2D, imageTexture);
    }
    else
    {
        glGenTextures(1, &imageTexture);
        *inOutTexture = (ImTextureID)(intptr_t)imageTexture;
        glBindTexture(GL_TEXTURE_2D, imageTexture);
        // Setup filtering parameters for display
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        // Upload pixels into texture
        glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    }

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, imageWidth, imageHeight, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, imageData);

    stbi_image_free(imageData);

    *outWidth = imageWidth;
    *outHeight = imageHeight;

    return true;
}

void TextureRelease(ImTextureID* outTexture)
{
    if (*outTexture != 0)
    {
        glDeleteTextures(1, (GLuint*)outTexture);
        *outTexture = 0;
    }
}

void UiPtpLiveViewShow(AppContext& c) {
    ImGui::SetNextWindowPos(ImVec2(910, 0), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(1000, 660), ImGuiCond_FirstUseEver);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

    ImGui::Begin("Live View");

    if (c.liveViewLastTime == 0) {
        c.liveViewLastTime = ImGui::GetTime();
    }

    double currentTime = ImGui::GetTime();
    bool refresh = (currentTime - c.liveViewLastTime >= 0.1f);
    if (refresh) {
        c.liveViewLastTime = currentTime;
        if (AwControl_GetLiveViewImage(&c.aw, &c.liveViewImage, &c.liveViewFrames).code == AW_RESULT_OK) {
            LoadTextureFromMemory(&c.liveViewImage, &c.liveViewImageGLId, &c.liveViewImageWidth, &c.liveViewImageHeight);
        }
        if (c.osdEnabled) {
            if (AwControl_GetOSDImage(&c.aw, &c.osdImage).code == AW_RESULT_OK) {
                LoadTextureFromMemory(&c.osdImage, &c.osdImageGLId, &c.osdImageWidth, &c.osdImageHeight);
                c.osdCaptured = true;
            } else {
                c.osdCaptured = false;
            }
        }
    }

    if (c.liveViewImage.size != 0) {
        // Calculate scaled dimensions while maintaining aspect ratio
        ImVec2 windowContentSize = ImGui::GetContentRegionAvail();
        float windowAspect = windowContentSize.x / windowContentSize.y;
        float aspectRatio = (float)c.liveViewImageWidth / (float)c.liveViewImageHeight;
        float renderWidth, renderHeight;
        if (windowAspect > aspectRatio) {
            // Window is wider than image
            renderHeight = windowContentSize.y;
            renderWidth = renderHeight * aspectRatio;
        } else {
            // Window is taller than image
            renderWidth = windowContentSize.x;
            renderHeight = renderWidth / aspectRatio;
        }
        ImVec2 cursor = ImGui::GetCursorPos();

        // Center the image
        ImVec2 imagePos(
                cursor.x + (windowContentSize.x - renderWidth) * 0.5f,
                cursor.y + (windowContentSize.y - renderHeight) * 0.5f);

        ImGui::SetCursorPos(imagePos);
        ImGui::Image(c.liveViewImageGLId, ImVec2(renderWidth, renderHeight));

        if (c.osdEnabled && c.osdCaptured) {
            // Render OSD at aspect
            ImGui::SetCursorPos(imagePos);
            ImGui::Image(c.osdImageGLId, ImVec2(renderWidth, renderHeight));
        }

        // Indicate detected focus frames
        if (c.liveFocusOverlay && MArraySize(c.liveViewFrames.focus.frames)) {
            ImDrawList *drawList = ImGui::GetWindowDrawList();
            ImVec2 windowPos = ImGui::GetWindowPos();

            MArrayEachPtr(c.liveViewFrames.focus.frames, it) {
                const auto& frame = it.p;

                f32 x = frame->x / (f32)c.liveViewFrames.focus.xDenominator;
                f32 y = frame->y / (f32)c.liveViewFrames.focus.yDenominator;
                f32 w = frame->width / (f32)c.liveViewFrames.focus.xDenominator;
                f32 h = frame->height / (f32)c.liveViewFrames.focus.yDenominator;

                x *= renderWidth;
                y *= renderHeight;
                w *= renderWidth * .5;
                h *= renderHeight * .5;

                ImVec2 rectMin(windowPos.x + imagePos.x + x - w, windowPos.y + imagePos.y + y - h);
                ImVec2 rectMax(windowPos.x + imagePos.x + x + w, windowPos.y + imagePos.y + y + h);

                ImU32 colour = IM_COL32(0, 255, 0, 255);
                if (frame->focusFrameState == SD_Focused) {
                    colour = IM_COL32(0, 255, 0, 255);
                } else if (frame->focusFrameState == SD_NotFocused) {
                    colour = IM_COL32(255, 0, 0, 255);
                } else {
                    colour = IM_COL32(255, 255, 255, 255);
                }

                float cornerLength = 10.0f;
                if (w > cornerLength && h > cornerLength) {
                    DrawRectCorners(drawList, rectMin, rectMax, colour, 2.0f, cornerLength);
                } else {
                    drawList->AddRect(rectMin, rectMax, colour, 0.0f, 0, 2.0f);
                }
            }
        }

        // Draw overlay if any
        if (c.liveViewOverlayMode != LiveViewOverlayMode_NONE) {
            ImDrawList *drawList = ImGui::GetWindowDrawList();
            ImVec2 windowPos = ImGui::GetWindowPos();
            ImU32 overlayOutlineColor = ImColor(0, 0, 0);
            ImU32 overlayMainColor = ImColor(c.liveViewOverlayColor[0], c.liveViewOverlayColor[1], c.liveViewOverlayColor[2]);
            float thickness = c.liveViewOverlayThickness;
            float outlineThickness = thickness +  0.5f;

            ImVec2 topLeft(windowPos.x + imagePos.x, windowPos.y + imagePos.y);
            ImVec2 topRight(windowPos.x + imagePos.x + renderWidth, windowPos.y + imagePos.y);
            ImVec2 bottomLeft(windowPos.x + imagePos.x, windowPos.y + imagePos.y + renderHeight);
            ImVec2 bottomRight(windowPos.x + imagePos.x + renderWidth, windowPos.y + imagePos.y + renderHeight);
            ImVec2 center(windowPos.x + imagePos.x + renderWidth * 0.5f, windowPos.y + imagePos.y + renderHeight * 0.5f);

            if (c.liveViewOverlayMode == LiveViewOverlayMode_CROSSHAIR) {
                float crosshairLen = renderHeight * 0.05f;
                drawList->AddLine(ImVec2(center.x, center.y - crosshairLen),
                    ImVec2(center.x, center.y + crosshairLen), overlayOutlineColor, outlineThickness);
                drawList->AddLine(ImVec2(center.x - crosshairLen, center.y),
                    ImVec2(center.x + crosshairLen, center.y), overlayOutlineColor, outlineThickness);
                drawList->AddLine(ImVec2(center.x, center.y - crosshairLen),
                    ImVec2(center.x, center.y + crosshairLen), overlayMainColor, thickness);
                drawList->AddLine(ImVec2(center.x - crosshairLen, center.y),
                    ImVec2(center.x + crosshairLen, center.y), overlayMainColor, thickness);
            } else if (c.liveViewOverlayMode == LiveViewOverlayMode_X) {
                drawList->AddLine(topLeft, bottomRight, overlayOutlineColor, outlineThickness);
                drawList->AddLine(topRight, bottomLeft, overlayOutlineColor, outlineThickness);
                drawList->AddLine(topLeft, bottomRight, overlayMainColor, thickness);
                drawList->AddLine(topRight, bottomLeft, overlayMainColor, thickness);
            }
        }

        // Check if mouse is clicked over window
        if (ImGui::IsWindowHovered()) {
            if (ImGui::IsMouseClicked(0, false)) {
                ImVec2 mousePos = ImGui::GetMousePos();
                ImVec2 windowPos = ImGui::GetWindowPos();

                // Calculate mouse position relative to image
                float relX = mousePos.x - (windowPos.x + imagePos.x);
                float relY = mousePos.y - (windowPos.y + imagePos.y);

                // Check if click is within image bounds
                if (relX >= 0 && relX <= renderWidth && relY >= 0 && relY <= renderHeight) {
                    switch (c.liveViewClickAction) {
                        case LiveViewClickAction_MOVE_FOCUS: {
                            // If no touch focus mode is set - set one
                            // Still need to be set to a flexible spot mode
                            AwPtpProperty* liveViewTouchOp =
                                AwControl_GetPropertyByCode(&c.aw, DPC_TOUCH_FOCUS_OPERATION);
                            if (liveViewTouchOp->value.u8 == 0) {
                                AwControl_SetPropertyValue(&c.aw, liveViewTouchOp, AwPtpPropValue{.u8 = 1});
                            }
                            AwMagnifier magnifier = {};
                            AwControl_GetMagnifier(&c.aw, &magnifier);

                            AwPosInt2 newPos = AwMagnifierMoveViewport(&magnifier,
                                AwPosFloat2{.x = relX / renderWidth, .y = relY / renderHeight});
                            u32 value = (u32)(newPos.x << 16) | newPos.y;
                            AwControl_SetControlValue(&c.aw, DPC_REMOTE_TOUCH_XY, AwPtpPropValue{.u32=value});
                            break;
                        }
                        case LiveViewClickAction_MAGNIFY: {
                            AwMagnifier magnifier = {};
                            AwControl_GetMagnifier(&c.aw, &magnifier);

                            // Move to next magnification level
                            i32 index = magnifier.ratioIndex;
                            if (magnifier.ratio.ratioByTen == 0) {
                                index = 1;
                            }
                            if (index < (magnifier.numRatios - 1)) {
                                index++;
                            }

                            AwMagnifierRatio* ratio = magnifier.ratios + index;

                            AwPosInt2 newPos = AwMagnifierMoveViewport(&magnifier,
                                AwPosFloat2{.x = relX / renderWidth, .y = relY / renderHeight});

                            AwControl_SetMagnifier(&c.aw,
                                AwMagnifierSet{.x = newPos.x, .y = newPos.y, .ratio = *ratio});
                            break;
                        }
                    }
                }
            }
        }
    }

    ImGui::End();
    ImGui::PopStyleVar();
}
