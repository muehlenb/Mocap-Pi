#pragma once

#include <filesystem>
#include <iostream>
#include <string>

#include "src/util/network/SSHCommunicator.h"

/**
 * Represents a remote cam
 */
struct RemoteCam {
    /** Last ip block, similar to unique id*/
    std::string address;

    /** Is a valid mocappi device to which we can connect to? */
    bool isValid;

    /** Last received image time of the framework */
    float lastReceivedImageTime = -1;

    /** Last received status time of the framework */
    float lastReceivedStatusTime = -1;

    /** Version of MoCap */
    std::string mocapCamVersion;

    /** SSH Communicator */
    std::shared_ptr<SSHCommunicator> com;

    /**
     * Remote Cam
     */
    RemoteCam(std::string address)
        : address(address)
        , isValid(false){
        com = std::make_shared<SSHCommunicator>(address);
    }

    bool isUniqueId(int uniqueId) {
        size_t lastDot = address.find_last_of('.');

        if(lastDot == std::string::npos)
            return false;

        int lastBlock = std::stoi(address.substr(lastDot + 1));

        return lastBlock == uniqueId;
    }

    /**
     * Check validity and retrieve general informations.
     */
    void init(){
        if(com->connect(2)){
            mocapCamVersion = com->execute("cd mocapcam && cat install");
            com->disconnect();
            isValid = true;
        }
    }

    bool checkValid(std::basic_ostream<char>& stream){
        if(!isValid || !com->connect(2)){
            stream << "Failed: ";

            if(!isValid)
                stream << "It's not a valid camera." << std::endl;
            else
                stream << "SSH connection not successful." << std::endl;

            return false;
        }
        return true;
    }

    /**
     * (Re-)installs the MocapCam software to the Raspberry Pi.
     */
    void install(std::basic_ostream<char>& stream = std::cout) {
        namespace fs = std::filesystem;
        stream << "Start install..." << std::endl << std::endl;

        // Locate cam folder in current working directory
        fs::path camFolder = fs::current_path() / "cam";

        if (!fs::exists(camFolder) || !fs::is_directory(camFolder)) {
            stream << "ERROR: Camera folder not found: "
                   << camFolder.string() << std::endl;
            stream << "Installation aborted!" << std::endl;
            return;
        }

        if (checkValid(stream)) {
            stream << "=================================================" << std::endl;
            stream << " Started Updating Camera Software..." << std::endl;
            stream << "=================================================" << std::endl;
            stream << " Ensure that the camera application source code" << std::endl;
            stream << " is located in '/cam' of the binary folder!" << std::endl;
            stream << "=================================================" << std::endl;
            stream << com->execute("sudo systemctl stop mocapcam.service");

            // Create remote folder structure
            stream << com->execute("mkdir -p mocapcam");

            // Upload all files recursively
            for (const auto& entry : fs::recursive_directory_iterator(camFolder)) {
                fs::path relativePath = fs::relative(entry.path(), camFolder);

                std::string dest = std::string(CAM_FOLDER) + "/" + relativePath.generic_string();

                if (entry.is_directory()) {
                    // Create subdirectories on remote machine
                    stream << com->execute(("mkdir -p \"" + dest + "\"").c_str());
                }
                else if (entry.is_regular_file()) {
                    std::string src = entry.path().string();

                    if (com->upload(src, dest)) {
                        stream << " - Uploaded: " << dest << std::endl;
                    }
                    else {
                        stream << "ERROR: Upload failed: " << src << std::endl;
                    }
                }
            }


            stream << "=================================================" << std::endl;
            stream << " Uploading source code successfully. " << std::endl;
            stream << " Camera software will now be compiled..." << std::endl;
            stream << "=================================================" << std::endl;

            // Compile
            stream << com->execute("cd mocapcam && cmake .");
            stream << com->execute("cd mocapcam && make");

            // Install service
            stream << com->execute(
                "sudo cp mocapcam/utils/mocapcam.service "
                "/etc/systemd/system/mocapcam.service"
                );

            stream << com->execute("sudo systemctl enable mocapcam.service");
            stream << com->execute("sudo systemctl start mocapcam.service");
            stream << com->execute("rm -f mocapcam/error.log") << std::endl;
            stream << com->execute("date +\"%Y%m%d\" > mocapcam/version") << std::endl;

            stream << "=================================================" << std::endl;
            stream << " Finished!" << std::endl;
            stream << "=================================================" << std::endl;

            // Free the connection
            com->disconnect();
        }
        else {
            stream << "Failed: ";

            if (!isValid)
                stream << "It's not a valid camera." << std::endl;
            else
                stream << "SSH connection not successful." << std::endl;
        }
    }

    void getLog(std::basic_ostream<char>& stream = std::cout){
        if(checkValid(stream)){
            stream << com->execute("cat mocapcam/error.log") << std::endl;
        }
    }

    void startService(std::basic_ostream<char>& stream = std::cout){
        if(checkValid(stream)){
            stream << com->execute("sudo systemctl start mocapcam.service") << std::endl;
            stream << "Service started." << std::endl;
        }
    }

    void stopService(std::basic_ostream<char>& stream = std::cout){
        if(checkValid(stream)){
            stream << com->execute("sudo systemctl stop mocapcam.service") << std::endl;
            stream << "Service stopped." << std::endl;
        }
    }
};
