#include "GUI.h"

#include "imgui/imgui.h"
#include "imgui/imgui_impl_win32.h"

#include "Config.h"
#include "Hooks.h"
#include "ImGuiCustom.h"

#include <array>
#include <ctime>
#include <iomanip>
#include <Pdh.h>
#include <sstream>
#include <vector>
#include <Windows.h>

GUI::GUI(HWND window) noexcept
{
    ImGui::CreateContext();
    ImGui_ImplWin32_Init(window);

    ImGui::StyleColorsClassic();
    ImGuiStyle& style = ImGui::GetStyle();

    style.ScrollbarSize = 13.0f;
    style.WindowTitleAlign = { 0.5f, 0.5f };

    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    io.LogFilename = nullptr;
    io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;
}

void GUI::render() noexcept
{
    if (!open)
        return;

    ImGui::Begin("GOESP", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoCollapse);

    if (!ImGui::BeginTabBar("##tabbar", ImGuiTabBarFlags_Reorderable | ImGuiTabBarFlags_NoTooltip)) {
        ImGui::End();
        return;
    }

    ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - 350.0f);

    ImGui::TextUnformatted("Build date: " __DATE__ " " __TIME__);
    ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - 55.0f);

    if (ImGui::Button("Unload"))
        hooks->restore();

    if (ImGui::BeginTabItem("ESP")) {
        drawESPTab();
        ImGui::EndTabItem();
    }
    if (ImGui::BeginTabItem("Misc")) {
        ImGuiCustom::colorPicker("Reload Progress", config->reloadProgress);
        ImGuiCustom::colorPicker("Recoil Crosshair", config->recoilCrosshair);
        ImGui::Checkbox("Normalize Player Names", &config->normalizePlayerNames);
        ImGui::Checkbox("Purchase List", &config->purchaseList.enabled);
        ImGui::SameLine();

        if (ImGui::Button("..."))
            ImGui::OpenPopup("##purchaselist");

        if (ImGui::BeginPopup("##purchaselist")) {
            ImGui::SetNextItemWidth(75.0f);
            ImGui::Combo("Mode", &config->purchaseList.mode, "Details\0Summary\0");
            ImGui::Checkbox("Only During Freeze Time", &config->purchaseList.onlyDuringFreezeTime);
            ImGui::Checkbox("Show Prices", &config->purchaseList.showPrices);
            ImGui::EndPopup();
        }
        ImGui::Checkbox("Bomb Zone Hint", &config->bombZoneHint);
        ImGui::EndTabItem();
    }
    if (ImGui::BeginTabItem("Configs")) {
        ImGui::TextUnformatted("Config is saved as \"config.txt\" inside GOESP directory in Documents");
        if (ImGui::Button("Load"))
            config->load();
        if (ImGui::Button("Save"))
            config->save();
        ImGui::EndTabItem();
    }
    ImGui::EndTabBar();
    ImGui::End();
}

void GUI::drawESPTab() noexcept
{
    static std::size_t currentCategory;
    static std::string currentItem = "All";
    static bool selectedSubItem;

    constexpr auto getConfigShared = [](std::size_t category, std::string item) noexcept -> Shared& {
        switch (category) {
        case 0: default: return config->allies[item];
        case 1: return config->enemies[item];
        case 2: return config->_weapons[item];
        case 3: return config->_projectiles[item];
        case 4: return config->_otherEntities[item];
        }
    };

    constexpr auto getConfigPlayer = [](std::size_t category, std::string item) noexcept -> Player& {
        switch (category) {
        case 0: default: return config->allies[item];
        case 1: return config->enemies[item];
        }
    };

    if (ImGui::ListBoxHeader("##list", { 170.0f, 300.0f })) {
        constexpr std::array categories{ "Allies", "Enemies", "Weapons", "Projectiles", "Other Entities" };

        for (std::size_t i = 0; i < categories.size(); ++i) {
            if (ImGui::Selectable(categories[i], currentCategory == i && currentItem == "All")) {
                currentCategory = i;
                currentItem = "All";
            }

            if (ImGui::BeginDragDropSource()) {
                switch (i) {
                case 0: case 1: ImGui::SetDragDropPayload("Player", &getConfigPlayer(i, "All"), sizeof(Player)); break;
                case 2: ImGui::SetDragDropPayload("Weapon", &config->_weapons["All"], sizeof(Weapon)); break;
                case 3: ImGui::SetDragDropPayload("Projectile", &config->_projectiles["All"], sizeof(Projectile)); break;
                default: ImGui::SetDragDropPayload("Entity", &getConfigShared(i, "All"), sizeof(Shared)); break;
                }
                ImGui::EndDragDropSource();
            }

            if (ImGui::BeginDragDropTarget()) {
                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("Player")) {
                    const auto& data = *(Player*)payload->Data;

                    switch (i) {
                    case 0: case 1: getConfigPlayer(i, "All") = data; break;
                    case 2: config->_weapons["All"] = data; break;
                    case 3: config->_projectiles["All"] = data; break;
                    default: getConfigShared(i, "All") = data; break;
                    }
                }

                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("Weapon")) {
                    const auto& data = *(Weapon*)payload->Data;

                    switch (i) {
                    case 0: case 1: getConfigPlayer(i, "All") = data; break;
                    case 2: config->_weapons["All"] = data; break;
                    case 3: config->_projectiles["All"] = data; break;
                    default: getConfigShared(i, "All") = data; break;
                    }
                }

                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("Projectile")) {
                    const auto& data = *(Projectile*)payload->Data;

                    switch (i) {
                    case 0: case 1: getConfigPlayer(i, "All") = data; break;
                    case 2: config->_weapons["All"] = data; break;
                    case 3: config->_projectiles["All"] = data; break;
                    default: getConfigShared(i, "All") = data; break;
                    }
                }

                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("Entity")) {
                    const auto& data = *(Shared*)payload->Data;

                    switch (i) {
                    case 0: case 1: getConfigPlayer(i, "All") = data; break;
                    case 2: config->_weapons["All"] = data; break;
                    case 3: config->_projectiles["All"] = data; break;
                    default: getConfigShared(i, "All") = data; break;
                    }
                }
                ImGui::EndDragDropTarget();
            }

            if (getConfigShared(i, "All").enabled)
                continue;

            ImGui::PushID(i);
            ImGui::Indent();

            const auto items = [](std::size_t category) noexcept -> std::vector<const char*> {
                switch (category) {
                case 0:
                case 1: return { "Visible", "Occluded" };
                case 2: return { "Pistols", "SMGs", "Rifles", "Sniper Rifles", "Shotguns", "Machineguns", "Grenades" };
                case 3: return { "Flashbang", "HE Grenade", "Breach Charge", "Bump Mine", "Decoy Grenade", "Molotov", "TA Grenade", "Smoke Grenade", "Snowball" };
                case 4: return { "Defuse Kit", "Chicken", "Planted C4" };
                default: return { };
                }
            }(i);

            for (std::size_t j = 0; j < items.size(); ++j) {
                if (ImGui::Selectable(items[j], currentCategory == i && !selectedSubItem && currentItem == items[j])) {
                    currentCategory = i;
                    currentItem = items[j];
                    selectedSubItem = false;
                }

                if (ImGui::BeginDragDropSource()) {
                    switch (i) {
                    case 0: case 1: ImGui::SetDragDropPayload("Player", &getConfigPlayer(i, items[j]), sizeof(Player)); break;
                    case 2: ImGui::SetDragDropPayload("Weapon", &config->_weapons[items[j]], sizeof(Weapon)); break;
                    case 3: ImGui::SetDragDropPayload("Projectile", &config->_projectiles[items[j]], sizeof(Projectile)); break;
                    default: ImGui::SetDragDropPayload("Entity", &getConfigShared(i, items[j]), sizeof(Shared)); break;
                    }
                    ImGui::EndDragDropSource();
                }

                if (ImGui::BeginDragDropTarget()) {
                    if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("Player")) {
                        const auto& data = *(Player*)payload->Data;

                        switch (i) {
                        case 0: case 1: getConfigPlayer(i, items[j]) = data; break;
                        case 2: config->_weapons[items[j]] = data; break;
                        case 3: config->_projectiles[items[j]] = data; break;
                        default: getConfigShared(i, items[j]) = data; break;
                        }
                    }

                    if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("Weapon")) {
                        const auto& data = *(Weapon*)payload->Data;

                        switch (i) {
                        case 0: case 1: getConfigPlayer(i, items[j]) = data; break;
                        case 2: config->_weapons[items[j]] = data; break;
                        case 3: config->_projectiles[items[j]] = data; break;
                        default: getConfigShared(i, items[j]) = data; break;
                        }
                    }

                    if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("Projectile")) {
                        const auto& data = *(Projectile*)payload->Data;

                        switch (i) {
                        case 0: case 1: getConfigPlayer(i, items[j]) = data; break;
                        case 2: config->_weapons[items[j]] = data; break;
                        case 3: config->_projectiles[items[j]] = data; break;
                        default: getConfigShared(i, items[j]) = data; break;
                        }
                    }

                    if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("Entity")) {
                        const auto& data = *(Shared*)payload->Data;

                        switch (i) {
                        case 0: case 1: getConfigPlayer(i, items[j]) = data; break;
                        case 2: config->_weapons[items[j]] = data; break;
                        case 3: config->_projectiles[items[j]] = data; break;
                        default: getConfigShared(i, items[j]) = data; break;
                        }
                    }
                    ImGui::EndDragDropTarget();
                }

                if (i != 2 || getConfigShared(i, items[j]).enabled)
                    continue;

                ImGui::Indent();

                const auto subItems = [](std::size_t item) noexcept -> std::vector<const char*> {
                    switch (item) {
                    case 0: return { "Glock-18", "P2000", "USP-S", "Dual Berettas", "P250", "Tec-9", "Five-SeveN", "CZ75-Auto", "Desert Eagle", "R8 Revolver" };
                    case 1: return { "MAC-10", "MP9", "MP7", "MP5-SD", "UMP-45", "P90", "PP-Bizon" };
                    case 2: return { "Galil AR", "FAMAS", "AK-47", "M4A4", "M4A1-S", "SG 553", "AUG" };
                    case 3: return { "SSG 08", "AWP", "G3SG1", "SCAR-20" };
                    case 4: return { "Nova", "XM1014", "Sawed-Off", "MAG-7" };
                    case 5: return { "M249", "Negev" };
                    case 6: return { "Flashbang", "HE Grenade", "Smoke Grenade", "Molotov", "Decoy Grenade", "Incendiary", "TA Grenade", "Fire Bomb", "Diversion", "Frag Grenade", "Snowball" };
                    default: return { };
                    }
                }(j);

                for (const auto subItem : subItems) {
                    if (ImGui::Selectable(subItem, currentCategory == i && selectedSubItem && currentItem == subItem)) {
                        currentCategory = i;
                        currentItem = subItem;
                        selectedSubItem = true;
                    }

                    if (ImGui::BeginDragDropSource()) {
                        ImGui::SetDragDropPayload("Weapon", &config->_weapons[subItem], sizeof(Weapon));
                        ImGui::EndDragDropSource();
                    }

                    if (ImGui::BeginDragDropTarget()) {
                        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("Player")) {
                            const auto& data = *(Player*)payload->Data;
                            config->_weapons[subItem] = data;
                        }

                        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("Weapon")) {
                            const auto& data = *(Weapon*)payload->Data;
                            config->_weapons[subItem] = data;
                        }

                        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("Projectile")) {
                            const auto& data = *(Projectile*)payload->Data;
                            config->_weapons[subItem] = data;
                        }

                        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("Entity")) {
                            const auto& data = *(Shared*)payload->Data;
                            config->_weapons[subItem] = data;
                        }
                        ImGui::EndDragDropTarget();
                    }
                }

                ImGui::Unindent();
            }
            ImGui::Unindent();
            ImGui::PopID();
        }
        ImGui::ListBoxFooter();
    }

    ImGui::SameLine();

    if (ImGui::BeginChild("##child", { 400.0f, 0.0f })) {
        auto& sharedConfig = getConfigShared(currentCategory, currentItem);

        ImGui::Checkbox("Enabled", &sharedConfig.enabled);
        ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - 260.0f);
        ImGui::SetNextItemWidth(220.0f);
        if (ImGui::BeginCombo("Font", config->systemFonts[sharedConfig.font.index].c_str())) {
            for (size_t i = 0; i < config->systemFonts.size(); i++) {
                bool isSelected = config->systemFonts[i] == sharedConfig.font.name;
                if (ImGui::Selectable(config->systemFonts[i].c_str(), isSelected, 0, { 250.0f, 0.0f })) {
                    sharedConfig.font.index = i;
                    sharedConfig.font.name = config->systemFonts[i];
                    sharedConfig.font.size = 15;
                    sharedConfig.font.fullName = sharedConfig.font.name + ' ' + std::to_string(sharedConfig.font.size);
                    config->scheduleFontLoad(sharedConfig.font.name);
                }
                if (isSelected)
                    ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }

        ImGui::Separator();

        constexpr auto spacing = 220.0f;
        ImGuiCustom::colorPicker("Snaplines", sharedConfig.snaplines);
        ImGui::SameLine();
        ImGui::SetNextItemWidth(90.0f);
        ImGui::Combo("##1", &sharedConfig.snaplineType, "Bottom\0Top\0Crosshair\0");
        ImGui::SameLine(spacing);
        ImGuiCustom::colorPicker("Box", sharedConfig.box);
        ImGui::SameLine();
        ImGui::SetNextItemWidth(95.0f);
        ImGui::Combo("##2", &sharedConfig.boxType, "2D\0" "2D corners\0" "3D\0" "3D corners\0");
        ImGuiCustom::colorPicker("Name", sharedConfig.name);
        ImGui::SameLine(spacing);
        ImGuiCustom::colorPicker("Text Background", sharedConfig.textBackground);
        ImGui::SetNextItemWidth(95.0f);
        ImGui::InputFloat("Text Cull Distance", &sharedConfig.textCullDistance, 0.4f, 0.8f, "%.1fm");
        sharedConfig.textCullDistance = std::clamp(sharedConfig.textCullDistance, 0.0f, 999.9f);

        if (currentCategory < 2) {
            auto& playerConfig = getConfigPlayer(currentCategory, currentItem);

            ImGuiCustom::colorPicker("Weapon", playerConfig.weapon);
            ImGui::SameLine(spacing);
            ImGuiCustom::colorPicker("Flash Duration", playerConfig.flashDuration);
            ImGui::Checkbox("Audible Only", &playerConfig.audibleOnly);
        } else if (currentCategory == 2) {
            auto& weaponConfig = config->_weapons[currentItem];

           // if (currentItem != 7)
                ImGuiCustom::colorPicker("Ammo", weaponConfig.ammo);
        } else if (currentCategory == 3) {
            ImGuiCustom::colorPicker("Trail", config->_projectiles[currentItem].trail);
            ImGui::SameLine(spacing);
            ImGui::SetNextItemWidth(95.0f);
            ImGui::InputFloat("Trail Time", &config->_projectiles[currentItem].trailTime, 0.1f, 0.5f, "%.1fs");
            config->_projectiles[currentItem].trailTime = std::clamp(config->_projectiles[currentItem].trailTime, 1.0f, 60.0f);
        }
    }

    ImGui::EndChild();
}
