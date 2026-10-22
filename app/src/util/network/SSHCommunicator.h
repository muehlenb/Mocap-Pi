#pragma once

#include <libssh/libssh.h>
#include <stdlib.h>
#include <stdio.h>

#include <fstream> // Stellen Sie sicher, dass Sie diese Zeile hinzufügen
#include <vector>
#include <iostream>
#include <filesystem>

#define CAM_USERNAME "mocappi"
#define CAM_PASSWORD "mocappi"
#define CAM_FOLDER "/home/mocappi/mocapcam"
#define CAM_PORT     22

#define SSH_LOG_VERBOSITY SSH_LOG_NOLOG

/**
 * SSH Communicator for execute commands remotely and upload files via scp.
 */
class SSHCommunicator {
private:
    ssh_session session;

    /** IP Address */
    std::string host;

    /** Is connected */
    bool isConnected;

public:
    SSHCommunicator(std::string host)
        : host(host)
        , isConnected(false){}


    bool connect(int timeout = 0){
        // Initialize session and set options
        session = ssh_new();
        if (session == NULL){
            return false;
        }

        int port = CAM_PORT;
        int verbosity = SSH_LOG_VERBOSITY;

        ssh_options_set(session, SSH_OPTIONS_HOST, host.c_str());
        ssh_options_set(session, SSH_OPTIONS_USER, CAM_USERNAME);
        ssh_options_set(session, SSH_OPTIONS_LOG_VERBOSITY, &verbosity);
        ssh_options_set(session, SSH_OPTIONS_PORT, &port);
        ssh_options_set(session, SSH_OPTIONS_TIMEOUT, &timeout);

        int rc;
        // Connect to server
        rc = ssh_connect(session);
        if (rc != SSH_OK) {
            fprintf(stderr, "Error connecting to localhost: %s\n",
                    ssh_get_error(session));
            ssh_free(session);
            return false;
        }

        // Authenticate ourselves
        rc = ssh_userauth_password(session, NULL, CAM_PASSWORD);
        if (rc != SSH_AUTH_SUCCESS) {
            fprintf(stderr, "Error authenticating with password: %s\n",
                    ssh_get_error(session));
            ssh_disconnect(session);
            ssh_free(session);
            return false;
        }

        isConnected = true;
        return true;
    }

    void disconnect(){
        if(!isConnected)
            return;

        ssh_disconnect(session);
        ssh_free(session);

        isConnected = false;
    }

    std::string execute(const char* command, bool printCommand = true) {
        if(!isConnected)
            return "";

        ssh_channel channel;
        int rc;
        char buffer[256];
        int nbytes;

        std::string result = printCommand ? "$ " + std::string(command) + "\n" : "";

        channel = ssh_channel_new(session);
        if (channel == NULL) return "";

        rc = ssh_channel_open_session(channel);
        if (rc != SSH_OK) {
            ssh_channel_free(channel);
            return "";
        }

        rc = ssh_channel_request_exec(channel, command);
        if (rc != SSH_OK) {
            ssh_channel_close(channel);
            ssh_channel_free(channel);
            return "";
        }

        nbytes = ssh_channel_read(channel, buffer, sizeof(buffer), 0);
        while (nbytes > 0) {
            result.append(buffer, nbytes);
            nbytes = ssh_channel_read(channel, buffer, sizeof(buffer), 0);
        }

        if (nbytes < 0) {
            ssh_channel_close(channel);
            ssh_channel_free(channel);
            return "";
        }

        ssh_channel_send_eof(channel);
        ssh_channel_close(channel);
        ssh_channel_free(channel);

        return result;
    }

    bool upload(std::string local_filepath, std::string remote_filepath) {
        if(!isConnected)
            return false;

        std::string remote_folder = "";
        std::string remote_file = remote_filepath;

        size_t lastSlash = remote_filepath.find_last_of('/');
        if (lastSlash != std::string::npos) {
            remote_folder = remote_filepath.substr(0, lastSlash);
            remote_file = remote_filepath.substr(lastSlash + 1);
        }

        std::cout << "Folder: " << remote_folder << std::endl;
        std::cout << "File: " << remote_file << std::endl;

        ssh_scp scp;
        int rc;
        std::ifstream file(local_filepath, std::ios::binary | std::ios::ate);
        std::streamsize size = file.tellg();
        file.seekg(0, std::ios::beg);

        if (size == -1) {
            std::cerr << "Failed to open local file" << std::endl;
            return false;
        }

        scp = ssh_scp_new(session, SSH_SCP_WRITE | SSH_SCP_RECURSIVE, remote_folder.c_str());
        if (scp == NULL) {
            std::cerr << "Error creating SCP session: " << ssh_get_error(session) << std::endl;
            return false;
        }

        rc = ssh_scp_init(scp);
        if (rc != SSH_OK) {
            std::cerr << "Error initializing SCP session: " << ssh_get_error(session) << std::endl;
            ssh_scp_free(scp);
            return false;
        }

        // Beginnt die Übertragung der Datei
        rc = ssh_scp_push_file(scp, remote_file.c_str(), size, 0644);
        if (rc != SSH_OK) {
            std::cerr << "Can't push file: " << ssh_get_error(session) << std::endl;
            ssh_scp_close(scp);
            ssh_scp_free(scp);
            return false;
        }

        std::vector<char> buffer(size);
        if (file.read(buffer.data(), size)) {
            rc = ssh_scp_write(scp, buffer.data(), size);
            if (rc != SSH_OK) {
                std::cerr << "Error writing to SCP: " << ssh_get_error(session) << std::endl;
                return false;
            }
        } else {
            std::cerr << "Error reading local file" << std::endl;
            return false;
        }

        ssh_scp_close(scp);
        ssh_scp_free(scp);

        return true;
    }

    std::vector<std::string> listFiles(std::string filepath) {
        const std::filesystem::path path(filepath);
        std::vector<std::string> results;
        try {
            if (std::filesystem::exists(path) && std::filesystem::is_directory(path)) {
                for (const auto& entry : std::filesystem::recursive_directory_iterator(path)) {
                    if (std::filesystem::is_regular_file(entry)) {
                        std::string foundPath = entry.path().string().substr(filepath.size()+1);
                        std::replace(foundPath.begin(), foundPath.end(), '\\', '/');
                        results.push_back(foundPath);
                    }
                }
            }
        } catch (const std::filesystem::filesystem_error& e) {
            std::cerr << e.what() << std::endl;
        }
        return results;
    }
};
