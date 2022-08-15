#include "main.hpp"
#include "GlobalNamespace/Saber.hpp"
#include "UnityEngine/Transform.hpp"
#include "questui/shared/QuestUI.hpp"
#include "questui/shared/BeatSaberUI.hpp"
#include "UnityEngine/Transform.hpp"
#include "MainConfig.hpp"
#include "bs-utils/shared/utils.hpp"
#include "GlobalNamespace/LobbySetupViewController.hpp"
#include "GlobalNamespace/MultiplayerModeSelectionViewController.hpp"

bool inMulti, score_sub;

using namespace GlobalNamespace;
using namespace UnityEngine;
DEFINE_CONFIG(MainConfig);

static ModInfo modInfo; // Stores the ID and version of our mod, and is sent to the modloader upon startup

// Loads the config from disk using our modInfo, then returns it for use
Configuration &getConfig()
{
    static Configuration config(modInfo);
    config.Load();
    return config;
}

// Returns a logger, useful for printing debug messages
Logger &getLogger()
{
    static Logger *logger = new Logger(modInfo);
    return *logger;
}

// tells us when the player joins multiplayer to disable the mod
MAKE_HOOK_MATCH(JoinLobbyUpdater, &LobbySetupViewController::DidActivate, void, LobbySetupViewController *self, bool firstActivation, bool addedToHierarchy, bool screenSystemEnabling)
{
    JoinLobbyUpdater(self, firstActivation, addedToHierarchy, screenSystemEnabling);
    inMulti = true;
    getLogger().info("Joined multiplayer, disabling mod");
}

// tells us when the player leavs multiplayer to enable the mod
MAKE_HOOK_MATCH(LeaveLobbyUpdater, &MultiplayerModeSelectionViewController::DidActivate, void, MultiplayerModeSelectionViewController *self, bool firstActivation, bool addedToHierarchy, bool screenSystemEnabling)
{
    LeaveLobbyUpdater(self, firstActivation, addedToHierarchy, screenSystemEnabling);
    if (inMulti)
    {
        inMulti = false;
        getLogger().info("Left multiplayer, enabling mod");
    }
}

// hook for changing saber size
MAKE_HOOK_MATCH(SaberSizeChanger, &Saber::ManualUpdate, void, Saber *self)
{
    SaberSizeChanger(self);
    
        // only activates if the saber is not correctly sized
        if (getMainConfig().Mod_active.GetValue() && !inMulti && getMainConfig().Thickness.GetValue() != self->get_transform()->get_localScale().x && getMainConfig().Length.GetValue() != self->get_transform()->get_localScale().z)
        {
            // sets the saber thickness and length based on the mod config
            self->get_transform()->set_localScale({getMainConfig().Thickness.GetValue(), getMainConfig().Thickness.GetValue(), getMainConfig().Length.GetValue()});
            getLogger().info("Not in multiplayer, resizing saber to %fw %fl", self->get_transform()->get_localScale().x, self->get_transform()->get_localScale().z);
        }

    // if the saber length is more than 1 disable score submission, reenables in multiplayer as sabers are forced to default size
    if (!inMulti)
    {
        if (getMainConfig().Mod_active.GetValue())
        {
            if (getMainConfig().Length.GetValue() > 1 || getMainConfig().Thickness.GetValue() > 1)
            {
                if (score_sub == false)
                {
                    bs_utils::Submission::disable(modInfo);
                    score_sub = true;
                    getLogger().info("Disable score");
                }
            }
        }
        else
        {
            if (score_sub == true)
            {
                bs_utils::Submission::enable(modInfo);
                getLogger().info("Enable score");
                score_sub = false;
            }
        }
    }
}

void DidActivate(HMUI::ViewController *self, bool firstActivation, bool addedToHierarchy, bool screenSystemEnabling)
{        
    if (firstActivation)
    {
        // Create a container that has a scroll bar.
        GameObject *container = QuestUI::BeatSaberUI::CreateScrollableSettingsContainer(self->get_transform());

        // Create a text that says "Hello World!" and set the parent to the container.
        QuestUI::BeatSaberUI::CreateText(container->get_transform(), "The mod config, change these settings or completely turn off the mod!");

        // the mod active toggle
        QuestUI::BeatSaberUI::CreateToggle(container->get_transform(), "Mod Enabled", getMainConfig().Mod_active.GetValue(), [](bool value)
                                           { getMainConfig().Mod_active.SetValue(value, true); });
        // the saber length config slider
        QuestUI::SliderSetting *sliderSetting1 = QuestUI::BeatSaberUI::CreateSliderSetting(container->get_transform(), "length", 0.01f, getMainConfig().Length.GetValue(), 0.01f, 15.0f, 1.0f, [](float value)
                                                                                           { getMainConfig().Length.SetValue(value, true); });
        // the saber thickness slider
        QuestUI::SliderSetting *sliderSetting2 = QuestUI::BeatSaberUI::CreateSliderSetting(container->get_transform(), "Thickness", 0.01f, getMainConfig().Thickness.GetValue(), 0.0f, 15.0f, 1.0f, [](float value)
                                                                                           { getMainConfig().Thickness.SetValue(value, true); });

        // reset button
        QuestUI::BeatSaberUI::CreateUIButton(container->get_transform(), "Reset sabers",
                                             [sliderSetting1, sliderSetting2]()
                                             {
                                                 getMainConfig().Thickness.SetValue(1);
                                                 getMainConfig().Length.SetValue(1);
                                                 sliderSetting1->set_value(getMainConfig().Length.GetValue());
                                                 sliderSetting2->set_value(getMainConfig().Thickness.GetValue());
                                             });
    }
}

// Called at the early stages of game loading
extern "C" void setup(ModInfo &info)
{
    info.id = MOD_ID;
    info.version = VERSION;
    modInfo = info;

    getConfig().Load(); // Load the config file
    getMainConfig().Init(info);
    getLogger().info("Completed setup!");
}

// Called later on in the game loading - a good time to install function hooks
extern "C" void load()
{
    il2cpp_functions::Init();

    QuestUI::Init();
    QuestUI::Register::RegisterModSettingsViewController(modInfo, DidActivate);
    QuestUI::Register::RegisterMainMenuModSettingsViewController(modInfo, DidActivate);

    getLogger().info("Installing hooks...");

    INSTALL_HOOK(getLogger(), SaberSizeChanger);
    INSTALL_HOOK(getLogger(), JoinLobbyUpdater);
    INSTALL_HOOK(getLogger(), LeaveLobbyUpdater);

    getLogger().info("Installed all hooks!");
}
