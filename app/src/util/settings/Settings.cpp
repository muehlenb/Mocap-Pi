#include "src/util/settings/Settings.h"

#include <fstream>

Settings Settings::instance;

void Settings::saveSettings(){
    nlohmann::json json_to_save = Settings::instance;
    std::ofstream output_file("settings.json");
    output_file << json_to_save.dump(4);
    output_file.close();
};

bool Settings::loadSettings(){
    try {
        std::ifstream input_file("settings.json");
        nlohmann::json json_to_load;
        input_file >> json_to_load;
        Settings::instance = json_to_load.get<Settings>();
        input_file.close();
        return true;
    } catch(nlohmann::json::parse_error& e){
        std::cout << e.what() << std::endl;
    } catch (std::runtime_error& e) {
        std::cout << e.what() << std::endl;
    }
    return false;
};
