// Main file
#define IMGUI_DEFINE_MATH_OPERATORS

#include "src/model/DataModel.h"
#include "src/view/View.h"
#include "src/processors/ProcessorCollection.h"

#include "src/util/network/CameraNetworkUtils.h"
#include "src/util/settings/Settings.h"

/**
 * Initializes the application including the GUI
 */
int main(int, char**)
{
    // Data model:
    DataModel model;

    // Load app settings:
    if(Settings::loadSettings()){
        // If loaded, override default rigid bodies:
        model.rigidBodies = Settings::instance.rigidBodies;
    }

    // Init network interface:
    if(!CameraNetworkUtils::initNetworkInterfaces(Settings::instance.networkInterface)){
        std::cerr << "No valid network interface available! App could not be started." << std::endl;
    }

    CameraNetworkUtils::scanRemoteCams();

    // Init model view controller Setup:
    ProcessorCollection processors(model);
    View view(model, processors);

    // Main loop which is executed every frame until the window is closed:
    while (!glfwWindowShouldClose(view.getMainWindow()))
    {
        view.render();
    }

    // Save settings:
    Settings::instance.rigidBodies = model.rigidBodies;
    Settings::saveSettings();

    return 0;
};

