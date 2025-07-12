#include "ui-live-view.h"

#include "imgui.h"
#include "imgui_impl_opengl3_loader.h"

#define _CRT_SECURE_NO_WARNINGS
#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

#include "ui/ui-context.h"
#include "ptp/ptp-const.h"

#include "mlib/mlib.h"

// Simple helper function to load an image into a OpenGL texture with common settings
bool LoadTextureFromMemory(const MMemIO* memIo, ImTextureID* out_texture, i32* out_width, i32* out_height)
{
    // Load from file
    int image_width = 0;
    int image_height = 0;
    unsigned char* image_data = stbi_load_from_memory((const unsigned char*)memIo->mem, (int)memIo->size,
                                                      &image_width, &image_height, NULL, 4);
    if (image_data == NULL)
        return false;

    // Create a OpenGL texture identifier
    GLuint image_texture;
    glGenTextures(1, &image_texture);
    glBindTexture(GL_TEXTURE_2D, image_texture);

    // Setup filtering parameters for display
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Upload pixels into texture
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image_width, image_height, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, image_data);
    stbi_image_free(image_data);

    *out_texture = (ImTextureID)(intptr_t)image_texture;
    *out_width = image_width;
    *out_height = image_height;

    return true;
}

void UiPtpLiveViewShow(AppContext& c) {
    ImGui::SetNextWindowPos(ImVec2(910, 0), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(1000, 660), ImGuiCond_FirstUseEver);

    ImGui::Begin("Live View");

    if (c.liveViewLastTime == 0) {
        c.liveViewLastTime = ImGui::GetTime();
    }

    double currentTime = ImGui::GetTime();
    bool refresh = (currentTime - c.liveViewLastTime >= 0.1f);
    if (refresh) {
        c.liveViewLastTime = currentTime;
        if (PTPControl_GetLiveViewImage(&c.ptp, &c.liveViewImage, &c.liveViewFrames) == PTP_OK) {
            PTP_FreeLiveViewFrames(&c.liveViewFrames);
        }

        LoadTextureFromMemory(&c.liveViewImage, &c.liveViewImageGLId,
                              &c.liveViewImageWidth, &c.liveViewImageHeight);
    }

    if (c.liveViewImage.size != 0) {
        ImVec2 windowSize = ImGui::GetWindowSize();

        // Calculate scaled dimensions while maintaining aspect ratio
        float windowAspect = windowSize.x / windowSize.y;
        float aspectRatio = (float)c.liveViewImageWidth / (float)c.liveViewImageHeight;

        float renderWidth, renderHeight;
        if (windowAspect > aspectRatio) {
            // Window is wider than image
            renderHeight = windowSize.y;
            renderWidth = renderHeight * aspectRatio;
        } else {
            // Window is taller than image
            renderWidth = windowSize.x;
            renderHeight = renderWidth / aspectRatio;
        }

        // Center the image
        ImVec2 imagePos(
                (windowSize.x - renderWidth) * 0.5f,
                (windowSize.y - renderHeight) * 0.5f);
        ImGui::SetCursorPos(imagePos);
        ImGui::Image(c.liveViewImageGLId, ImVec2(renderWidth, renderHeight));
    }

    ImGui::End();
}
