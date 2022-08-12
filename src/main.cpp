#include "main.hpp"
#include "GlobalNamespace/Saber.hpp"
#include "UnityEngine/Transform.hpp"
#include "questui/shared/QuestUI.hpp"
#include "questui/shared/BeatSaberUI.hpp"
#include "UnityEngine/Transform.hpp"
#include "MainConfig.hpp"


using namespace GlobalNamespace;
using namespace UnityEngine;
DEFINE_CONFIG(MainConfig)

static ModInfo modInfo; // Stores the ID and version of our mod, and is sent to the modloader upon startup

// Loads the config from disk using our modInfo, then returns it for use
Configuration& getConfig() {
    static Configuration config(modInfo);
    config.Load();
    return config;
}

// Returns a logger, useful for printing debug messages
Logger& getLogger() {
    static Logger* logger = new Logger(modInfo);
    return *logger;
}


MAKE_HOOK_MATCH(sabersize, &Saber::ManualUpdate, void, Saber *self)
{
    sabersize (self);
    // checks if the mod config says the mod is enabled
    if (getMainConfig().Mod_active.GetValue()){
    // sets the saber thickness and length based on the mod config
    self->get_transform()->set_localScale({getMainConfig().Thickness.GetValue(), getMainConfig().Thickness.GetValue(), getMainConfig().Length.GetValue()});
    }
}



void DidActivate(HMUI::ViewController* self, bool firstActivation, bool addedToHierarchy, bool screenSystemEnabling) {
    // Create our UI elements only when shown for the first time.
    if(firstActivation) {
        // Create a container that has a scroll bar.
        GameObject* container = QuestUI::BeatSaberUI::CreateScrollableSettingsContainer(self->get_transform());
       
        // Create a text that says "Hello World!" and set the parent to the container.
        QuestUI::BeatSaberUI::CreateText(container->get_transform(), "The mod config, change these settings or completely turn off the mod!");

        // the mod active toggle
        QuestUI::BeatSaberUI::CreateToggle(container->get_transform(), "Mod Enabled", getMainConfig().Mod_active.GetValue(), [](bool value) {
		getMainConfig().Mod_active.SetValue(value, true);
        });
        // the saber length config slider
        QuestUI::SliderSetting *sliderSetting1 = QuestUI::BeatSaberUI::CreateSliderSetting(container->get_transform(), "length", 0.01f , getMainConfig().Length.GetValue(), 0.01f, 15.0f, 1.0f, [](float value){
            getMainConfig().Length.SetValue(value, true);
        });
        // the saber thickness slider
        QuestUI::SliderSetting *sliderSetting2 = QuestUI::BeatSaberUI::CreateSliderSetting(container->get_transform(), "Thickness", 0.01f , getMainConfig().Thickness.GetValue(), 0.0f, 15.0f, 1.0f, [](float value){
            getMainConfig().Thickness.SetValue(value, true);
        });

        // reset button

        QuestUI::BeatSaberUI::CreateUIButton(container->get_transform(), "Reset sabers",
        [sliderSetting1, sliderSetting2]() {
            getMainConfig().Thickness.SetValue(1);
            getMainConfig().Length.SetValue(1);
            sliderSetting1->set_value(getMainConfig().Length.GetValue());
            sliderSetting2->set_value(getMainConfig().Thickness.GetValue());
        });
    }       
}


// Called at the early stages of game loading
extern "C" void setup(ModInfo& info) {
    info.id = MOD_ID;
    info.version = VERSION;
    modInfo = info;
	
    getConfig().Load(); // Load the config file
    getMainConfig().Init(info);
    getLogger().info("Completed setup!");
}

// Called later on in the game loading - a good time to install function hooks
extern "C" void load() {
    il2cpp_functions::Init();
    
    

    QuestUI::Init();
    QuestUI::Register::RegisterModSettingsViewController(modInfo, DidActivate);
    QuestUI::Register::RegisterMainMenuModSettingsViewController(modInfo, DidActivate);


    getLogger().info("Installing hooks...");

    INSTALL_HOOK(getLogger(),sabersize);
    
    
    getLogger().info("Installed all hooks!");
}



