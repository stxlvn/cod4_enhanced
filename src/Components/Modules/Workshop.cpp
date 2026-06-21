#include "STDInc.hpp"
#include <rapidjson/document.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include <stb_image_resize_test/old_image_resize.h>

namespace Components
{
    std::optional<std::string> Workshop::modsData;
    Workshop::Workshop_t Workshop::workshopDataInfo;
    rapidjson::Document modDataJson;

    bool isLoadingMods = false;
    float loadingStartTime = 0.0f;
    static Game::GfxImage* modImage;
    float ModImagePreviewWidth = 0.0f;
    float ModImagePreviewHeight = 0.0f;

    bool LoadTextureFromURL(const char* url)
    {
        int width, height, channels;

        auto modImageDataOpt = Utils::HTTP::GetData(url, {}, {}, false);
        if (!modImageDataOpt.has_value()) return false;

        auto& data = *modImageDataOpt;
        unsigned char* imageData = stbi_load_from_memory((const unsigned char*)data.data(), static_cast<int>(data.size()), &width, &height, &channels, 4);
        if (!imageData) return false;

        auto imageDataCleaner = gsl::finally([=]() { stbi_image_free(imageData); });

        int desiredWidth = 1000;
        int desiredHeight = 720;
        unsigned char* finalImageData = imageData;

        if ((desiredWidth != width || desiredHeight != height) && desiredWidth > 0 && desiredHeight > 0)
        {
            unsigned char* resizedData = (unsigned char*)malloc(desiredWidth * desiredHeight * 4);
            if (!resizedData) return false;

            int resizeResult = stbir_resize_uint8(
                imageData,
                width,
                height,
                width * 4,
                resizedData,
                desiredWidth,
                desiredHeight,
                desiredWidth * 4,
                4
            );

            if (resizeResult == 0)
            {
                free(resizedData);
                return false;
            }

            finalImageData = resizedData;
            width = desiredWidth;
            height = desiredHeight;
        }

        modImage->width = width;
        modImage->height = height;

        HRESULT hr = Game::GetDevice()->CreateTexture(width, height, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &modImage->texture.map, nullptr);
        if (FAILED(hr))
            return false;

        D3DLOCKED_RECT lockedRect;
        hr = modImage->texture.map->LockRect(0, &lockedRect, nullptr, 0);
        if (FAILED(hr))
            return false;

        for (int y = 0; y < height; y++)
        {
            unsigned char* dstRow = (unsigned char*)lockedRect.pBits + y * lockedRect.Pitch;
            unsigned char* srcRow = finalImageData + width * 4 * y;
            for (int x = 0; x < width; x++)
            {
                unsigned char* srcPixel = srcRow + x * 4;
                unsigned char* dstPixel = dstRow + x * 4;

                dstPixel[0] = srcPixel[2];
                dstPixel[1] = srcPixel[1];
                dstPixel[2] = srcPixel[0];
                dstPixel[3] = srcPixel[3];
            }
        }

        modImage->texture.map->UnlockRect(0);

        if (finalImageData != imageData)
            free(finalImageData);

        return true;
    }

    void DrawImageWithContainerFitKeepRatio(ImTextureID textureID, const ImVec2& imageSize, const ImVec2& containerSize, float scaleW, float scaleH)
    {
        ImVec2 scaledContainer(containerSize.x * scaleW, containerSize.y * scaleH);

        float scaleX = scaledContainer.x / imageSize.x;
        float scaleY = scaledContainer.y / imageSize.y;

        float fitScale = std::min(scaleX, scaleY);

        ImVec2 displaySize(imageSize.x * fitScale, imageSize.y * fitScale);

        ImGui::Image(textureID, displaySize);
    }

    void Workshop::CreateWorkshop(Game::gui_menus_t& menu)
    {
        ImGuiIO& io = ImGui::GetIO();
        io.WantCaptureKeyboard = false;
        io.WantCaptureMouse = true;

        float scaleW = static_cast<float>(Renderer::Width()) / 640.0f;
        float scaleH = static_cast<float>(Renderer::Height()) / 480.0f;


        static std::vector<const char*> modNames;
        static bool imageLoaded = false;

        if (!menu.one_time_init)
        {
            menu.got_layout_from_menu = false;
            for (auto m = 0; m < Game::uiInfo->uiDC.openMenuCount; m++)
            {
                // main gui menu size and positions
                if (Game::uiInfo->uiDC.menuStack[m] && !_stricmp(Game::uiInfo->uiDC.menuStack[m]->window.name, "workshop_menu"))
                {
                    for (auto i = 0; i < Game::uiInfo->uiDC.menuStack[m]->itemCount; i++)
                    {
                        if (Game::uiInfo->uiDC.menuStack[m]->items[i] && Game::uiInfo->uiDC.menuStack[m]->items[i]->window.name)
                        {
                            // get width and height for mod image preview
                            if (!_stricmp(Game::uiInfo->uiDC.menuStack[m]->items[i]->window.name, "workshop_image_preview"))
                            {
                                ModImagePreviewWidth = Game::uiInfo->uiDC.menuStack[m]->items[i]->window.rect.w;
                                ModImagePreviewHeight = Game::uiInfo->uiDC.menuStack[m]->items[i]->window.rect.h;
                            }

                            if (!_stricmp(Game::uiInfo->uiDC.menuStack[m]->items[i]->window.name, "workshop_area"))
                            {
                                GUI::SetMenuLayout(
                                    menu,
                                    Game::uiInfo->uiDC.menuStack[m]->items[i]->window.rect.x,
                                    Game::uiInfo->uiDC.menuStack[m]->items[i]->window.rect.y,
                                    Game::uiInfo->uiDC.menuStack[m]->items[i]->window.rect.w,
                                    Game::uiInfo->uiDC.menuStack[m]->items[i]->window.rect.h,
                                    Game::uiInfo->uiDC.menuStack[m]->items[i]->window.rect.horzAlign,
                                    Game::uiInfo->uiDC.menuStack[m]->items[i]->window.rect.vertAlign);
                                menu.got_layout_from_menu = true;
                            }
                        }
                    }
                }
            }
        }

        if (menu.got_layout_from_menu)
        {
            if (!menu.one_time_init)
            {
                Game::ScrPlace_ApplyRect(Game::scrPlaceFull, &menu.position[0], &menu.position[1], &menu.size[0], &menu.size[1], menu.horzAlign, menu.vertAlign);
                ImGui::SetNextWindowPos(ImVec2(menu.position[0], menu.position[1]), ImGuiCond_Always);
                ImGui::SetNextWindowSize(ImVec2(menu.size[0], menu.size[1]), ImGuiCond_Always);
                menu.one_time_init = true;
            }
        }
        else
        {
            menu.menustate = false;
            return;
        }

        bool isWorkshopMenuActive = false;
        for (auto m = 0; m < Game::uiInfo->uiDC.openMenuCount; m++)
        {
            if (Game::uiInfo->uiDC.menuStack[m] && !_stricmp(Game::uiInfo->uiDC.menuStack[m]->window.name, "workshop_menu"))
            {
                isWorkshopMenuActive = true;
                break;
            }
        }
        menu.menustate = isWorkshopMenuActive;

        if (menu.menustate)
        {
            if (!ImGui::Begin("Workshop", &menu.menustate, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse))
            {
                ImGui::End();
                return;
            }

            ImGui::Columns(2, nullptr, true);

            static const char* modTypes[] = { "Custom Campaign", "Custom Weapons", "Game Mode", "Custom Mission", "Stand-Alone Projects"};
            static int selectedModTypeIndex = 0;
            static int lastSelectedModTypeIndex = -1;
            static int selectedModIndex = -1;
            static int lastSelectedModIndex = -1;

            if (!menu.one_time_init)
            {
                selectedModTypeIndex = 0;
                lastSelectedModTypeIndex = -1;
                selectedModIndex = -1;
                lastSelectedModIndex = -1;
            }

            ImGui::Combo("Mod Type", &selectedModTypeIndex, modTypes, IM_ARRAYSIZE(modTypes));
            if (selectedModTypeIndex != lastSelectedModTypeIndex)
            {
                lastSelectedModTypeIndex = selectedModTypeIndex;
                modNames.clear();
                if (modDataJson.IsArray())
                {
                    const auto files = modDataJson.GetArray();
                    for (const auto& file : files)
                    {
                        if (strcmp(file["mod_type"].GetString(), modTypes[selectedModTypeIndex]) == 0)
                        {
                            modNames.push_back(file["mod_title"].GetString());
                        }
                    }
                }

                selectedModIndex = -1;
                lastSelectedModIndex = -1;
            }

            ImGui::BeginChild("Mods List", ImVec2(0, 0), true);

            if (isLoadingMods)
            {
                float currentTime = ImGui::GetTime();
                int downloadDots = static_cast<int>((currentTime - loadingStartTime) * 2) % 4;
                std::string loadingText = "Loading mod data" + std::string(downloadDots, '.');
                ImGui::Text("%s", loadingText.c_str());
                selectedModIndex = -1;
                lastSelectedModIndex = -1;
            }
            else
            {
                ImGui::PushItemWidth(menu.size[0]);
                if (selectedModIndex >= static_cast<int>(modNames.size()))
                    selectedModIndex = -1;
                ImGui::ListBox("Mods", &selectedModIndex, modNames.data(), static_cast<int>(modNames.size()), static_cast<int>(modNames.size()));
            }

            if (!modNames.empty() && selectedModIndex == -1)
            {
                selectedModIndex = 0;
            }


            ImGui::EndChild();

            ImGui::NextColumn();
            ImGui::BeginChild("Mod Info", ImVec2(0, 0), true);

            if (selectedModIndex != lastSelectedModIndex)
            {
                lastSelectedModIndex = selectedModIndex;

                if (selectedModIndex >= 0 && static_cast<size_t>(selectedModIndex) < modNames.size())
                {
                    int modDataIndex = -1;
                    if (modDataJson.IsArray())
                    {
                        const auto files = modDataJson.GetArray();
                        for (size_t i = 0; i < files.Size(); i++) {
                            if (strcmp(files[i]["mod_title"].GetString(), modNames[selectedModIndex]) == 0) {
                                modDataIndex = static_cast<int>(i);
                                break;
                            }
                        }
                    }

                    if (modDataIndex != -1) {
                        const auto& modData = modDataJson.GetArray()[modDataIndex];

                        workshopDataInfo.mod_title = modData["mod_title"].GetString();
                        workshopDataInfo.mod_author = modData["mod_author"].GetString();
                        workshopDataInfo.mod_image_preview = modData["mod_image_preview"].GetString();
                        workshopDataInfo.mod_description = modData["mod_description"].GetString();
                        workshopDataInfo.mod_external_download_link = modData["mod_external_download_link"].GetString();
                        workshopDataInfo.mod_type = modData["mod_type"].GetString();

                        LoadTextureFromURL(workshopDataInfo.mod_image_preview.c_str());
                    }
                }
                else
                {
                    workshopDataInfo = Workshop::Workshop_t{};
                }
            }

            if (modImage->texture.map)
            {
                ImVec2 originalSize(modImage->width, modImage->height);
                ImVec2 containerSize(ModImagePreviewWidth, ModImagePreviewHeight);
                DrawImageWithContainerFitKeepRatio((ImTextureID)modImage->texture.map, originalSize, containerSize, scaleW, scaleH);
                ImGui::Spacing();
            }

            if (selectedModIndex >= 0 && static_cast<size_t>(selectedModIndex) < modNames.size())
            {
                ImGui::Text("%s", workshopDataInfo.mod_title.c_str());
                ImGui::Separator();
                ImGui::Text("Author: %s", workshopDataInfo.mod_author.c_str());
                ImGui::Separator();
                ImGui::TextWrapped("Description: %s", workshopDataInfo.mod_description.c_str());
                ImGui::Separator();

                // Show "Download Mod" button
                if (ImGui::Button("Download Mod"))
                {
                    // Open warning popup
                    ImGui::OpenPopup("External Link Warning");
                }

                // Warning popup modal
                if (ImGui::BeginPopupModal("External Link Warning", NULL, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove ))
                {
                    // Warning text
                    ImGui::Text("You are about to open an external link:");
                    ImGui::Spacing();

                    std::string link_text = workshopDataInfo.mod_external_download_link;

                    // Make link look like a hyperlink (blue color)
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 0.0f, 1.0f, 1.0f));
                    if (ImGui::Selectable(link_text.c_str()))
                    {
                        Utils::OpenUrl(workshopDataInfo.mod_external_download_link.c_str());
                        ImGui::CloseCurrentPopup();
                    }
                    ImGui::PopStyleColor();

                    ImGui::Spacing();

                    // Calculate total width needed for buttons
                    float window_width = Renderer::Width();

                    // Define button size
                    float button_width = 80.0f; // fixed width, can be adjusted

                    // Number of buttons
                    int button_count = 2;
                    float spacing_between = 10.0f;

                    // Calculate total width of buttons
                    float total_buttons_width = button_width * button_count + spacing_between * (button_count - 1);

                    // Set cursor x to center buttons
                    float cursor_x = (window_width - total_buttons_width) / 2.0f;
                    ImGui::SetCursorPosX(cursor_x);

                    // "Open" button
                    if (ImGui::Button("Open", ImVec2(button_width, 0)))
                    {
                        Utils::OpenUrl(workshopDataInfo.mod_external_download_link.c_str());
                        ImGui::CloseCurrentPopup();
                    }

                    // Same line with spacing
                    ImGui::SameLine(0, spacing_between);

                    // "Cancel" button
                    if (ImGui::Button("Cancel", ImVec2(button_width, 0)))
                    {
                        ImGui::CloseCurrentPopup();
                    }

                    ImGui::EndPopup();
                }
            }

            ImGui::EndChild();
            ImGui::Columns(1);

            io.WantCaptureKeyboard = false;
            io.WantCaptureMouse = false;
            GUI::MouseReset();

            ImGui::End();
        }
    }

    void Workshop::LoadModData()
    {
        isLoadingMods = true;
        loadingStartTime = ImGui::GetTime();

        modsData = Utils::HTTP::GetData(URL_MASTER + "workshop.json"s, {}, {}, false);

        if (!modsData.has_value())
        {
            Game::Com_Printf(0, "[Workshop]: ^1json file is empty or HTTP request can't be reach to the server. Abort the loading workshop.\n");
            isLoadingMods = false;
            return;
        }

        modDataJson.Parse(modsData.value().data());

        if (!modDataJson.IsArray())
        {
            Game::Com_Printf(0, "[Workshop]: ^1json file doesn't have array list. Abort the loading workshop.\n");
            isLoadingMods = false;
            return;
        }

        isLoadingMods = false;
    }

    Workshop::Workshop()
    {
        modImage = Utils::Memory::GetAllocator()->allocate<Game::GfxImage>();
        modImage->name = "workshop_mod_preview_image";

        UIScript::Add("get_workshop", []([[maybe_unused]] const UIScript::Token& token, [[maybe_unused]] const Game::uiInfo_s* info)
        {
            Scheduler::Once([]
            {
                LoadModData();
            }, Scheduler::Pipeline::ASYNC);
        });

        Renderer::OnDeviceRecoveryBegin([]()
		{
            if(modImage->texture.map)
            {
                modImage->texture.map->Release();
                modImage->texture.map = nullptr;
            }
		});
    }

    Workshop::~Workshop()
    {
    }
}