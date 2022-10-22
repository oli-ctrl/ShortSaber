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
#include "UnityEngine/SceneManagement/SceneManager.hpp"
#include "GlobalNamespace/MainMenuViewController.hpp"

using namespace GlobalNamespace;
using namespace UnityEngine;

DEFINE_CONFIG(MainConfig);

bool inMulti;
int shouldUpdate = 2;

bool fairSize()
{
    if(getMainConfig().Thickness.GetValue() > 1 || getMainConfig().Length.GetValue() > 1)
        return false;
    else 
        return true;
}

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

// when the player joins multiplayer
MAKE_HOOK_MATCH(LobbySetupViewController_DidActivate, &LobbySetupViewController::DidActivate, void, LobbySetupViewController *self, bool firstActivation, bool addedToHierarchy, bool screenSystemEnabling)
{
    LobbySetupViewController_DidActivate(self, firstActivation, addedToHierarchy, screenSystemEnabling);
    inMulti = true;
}

// activates on scene change, useful for resetting update count at start of song
MAKE_HOOK_MATCH(SceneManager_Internal_ActiveSceneChanged, &SceneManagement::SceneManager::Internal_ActiveSceneChanged, void, UnityEngine::SceneManagement::Scene previousActiveScene, UnityEngine::SceneManagement::Scene newActiveScene)
{
    SceneManager_Internal_ActiveSceneChanged(previousActiveScene, newActiveScene);
    shouldUpdate = 2;
}

// when the player leaves multiplayer
MAKE_HOOK_MATCH(MultiplayerModeSelectionViewController_DidActivate, &MultiplayerModeSelectionViewController::DidActivate, void, MultiplayerModeSelectionViewController *self, bool firstActivation, bool addedToHierarchy, bool screenSystemEnabling)
{
    MultiplayerModeSelectionViewController_DidActivate(self, firstActivation, addedToHierarchy, screenSystemEnabling);
    inMulti = false;
}

// Update score submission in menus so the red text is shown if neccessary
MAKE_HOOK_MATCH(MainMenuViewController_DidActivate, &MainMenuViewController::DidActivate, void, MainMenuViewController *self, bool firstActivation, bool addedToHierarchy, bool screenSystemEnabling)
{
    MainMenuViewController_DidActivate(self, firstActivation, addedToHierarchy, screenSystemEnabling);

    if((fairSize() && !bs_utils::Submission::getEnabled()) || !getMainConfig().Mod_active.GetValue())
        bs_utils::Submission::enable(modInfo);
    else if (!fairSize() && bs_utils::Submission::getEnabled())
        bs_utils::Submission::disable(modInfo);
}

// hook for changing saber size
MAKE_HOOK_MATCH(Saber_ManualUpdate, &Saber::ManualUpdate, void, Saber *self)
{
    Saber_ManualUpdate(self);

    // only activates twice
    if (getMainConfig().Mod_active.GetValue() && shouldUpdate)
    {
        // sets the saber thickness and length based on the mod config
        if (!inMulti)
            self->get_transform()->set_localScale({getMainConfig().Thickness.GetValue(), getMainConfig().Thickness.GetValue(), getMainConfig().Length.GetValue()});

        // only change the thickness of the saber when in multiplayer but have the default length
        if (inMulti)
            self->get_transform()->set_localScale({getMainConfig().Thickness.GetValue(), getMainConfig().Thickness.GetValue(), 1});

        shouldUpdate--;
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
        QuestUI::SliderSetting *sliderSetting1 = QuestUI::BeatSaberUI::CreateSliderSetting(container->get_transform(), "Length", 0.01f, getMainConfig().Length.GetValue(), 0.00f, 15.0f, 0.01f, [](float value)
        {
            getMainConfig().Length.SetValue(value, true);
        });
        // the saber thickness slider
        QuestUI::SliderSetting *sliderSetting2 = QuestUI::BeatSaberUI::CreateSliderSetting(container->get_transform(), "Thickness", 0.01f, getMainConfig().Thickness.GetValue(), 0.00f, 15.0f, 0.01f, [](float value)
        {
            getMainConfig().Thickness.SetValue(value, true);
        });

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

    INSTALL_HOOK(getLogger(), Saber_ManualUpdate);
    INSTALL_HOOK(getLogger(), LobbySetupViewController_DidActivate);
    INSTALL_HOOK(getLogger(), MultiplayerModeSelectionViewController_DidActivate);
    INSTALL_HOOK(getLogger(), MainMenuViewController_DidActivate);
    INSTALL_HOOK(getLogger(), SceneManager_Internal_ActiveSceneChanged);

    getLogger().info("Installed all hooks!");
}
