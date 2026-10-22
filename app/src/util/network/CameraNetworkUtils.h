#pragma once

#include <libssh/libssh.h>
#include <vector>
#include <chrono>

#include <boost/asio.hpp>
#include <boost/bind/bind.hpp>

#include <boost/asio/deadline_timer.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include "src/util/network/RemoteCam.h"
#include "src/util/settings/Settings.h"

using namespace std::chrono_literals;

struct AutoJoinThread {
    std::thread thread;

    ~AutoJoinThread(){
        if(thread.joinable())
            thread.join();
    }
};

struct CameraNetworkUtils {
    /** All available network interfaces found in last scan */
    static std::vector<std::string> availableNetworkInterfaces;

    /** All found remote cams in last scan */
    static std::vector<RemoteCam> remoteCams;

    static AutoJoinThread scanThread;
    static bool scanThreadFinished;

    /**
     * Update last received
     */
    static void updateLastReceived(int uniqueID, float hostTime, int packetType){
        for(RemoteCam& cam : remoteCams){
            if(cam.isUniqueId(uniqueID)){
                if(packetType == 64)
                    cam.lastReceivedImageTime = hostTime;

                if(packetType == 1)
                    cam.lastReceivedStatusTime = hostTime;
                return;
            }
        }
    }

    /**
     * Check if port is open.
     */
    static bool is_port_open(const std::string& address, int port, int timeout_seconds) {
        boost::asio::io_context io_context;

        boost::asio::deadline_timer timer(io_context);
        timer.expires_from_now(boost::posix_time::seconds(timeout_seconds));

        boost::asio::ip::tcp::socket socket(io_context);
        boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::make_address(address), port);

        bool is_connected = false;

        socket.async_connect(endpoint, [&](const boost::system::error_code& error) {
            // Stoppe den Timer, wenn die Verbindung vor dem Timeout hergestellt wird.
            timer.cancel();
            is_connected = !error;
        });

        timer.async_wait([&](const boost::system::error_code& /*error*/) {
            // Schließe den Socket, wenn das Timeout erreicht ist, bevor eine Verbindung hergestellt wird.
            socket.close();
        });

        io_context.run();

        return is_connected;
    }

    /**
     * Scans for remote cams and update the 'remoteCams' variable.
     */
    static void scanRemoteCams(){
        if(!scanThreadFinished)
            return;

        scanThreadFinished = false;
        remoteCams = std::vector<RemoteCam>();

        if(scanThread.thread.joinable())
            scanThread.thread.join();

        scanThread.thread = std::thread([](){
            std::vector<int> results;

            size_t npos = Settings::instance.networkInterface.find_last_of('.');

            if (npos == std::string::npos){
                std::cerr << "No useful IP interface addr given in findRemoteCams call!" << std::endl;
                return;
            }


            std::string startIPPart = Settings::instance.networkInterface.substr(0, npos + 1);

            // Create Port 22 scan futures (for all 255 ip addresses):
            std::vector<std::future<bool>> sshFutures;
            for (int i = 1; i < 255; ++i) {
                sshFutures.emplace_back(std::async(std::launch::async, is_port_open, startIPPart + std::to_string(i), 22, 2));
            }

            // Create RemoteCam objects for every open ssh port:
            std::vector<RemoteCam> tmpRemoteCams;
            for (int i = 1; i < 255; ++i) {
                if (sshFutures[i-1].get()) {
                    tmpRemoteCams.emplace_back(startIPPart + std::to_string(i));
                }
            }
            remoteCams = tmpRemoteCams;

            // Create futures to check whether the found open ports are valid RemoteCams:
            std::vector<std::future<void>> camInitFutures;
            for(RemoteCam& cam : remoteCams){
                camInitFutures.emplace_back(std::async(std::launch::async, [&cam](){cam.init();}));
            }

            // Just wait for all futures to be finished:
            for(std::future<void>& future : camInitFutures){
                std::future_status status = future.wait_for(2s);

                // Only "consume" when available:
                if(status == std::future_status::ready)
                    future.get();
            }

            scanThreadFinished = true;
        });
    }

    static void scanNetworkInterfaces() {
        boost::asio::io_context io_context;
        boost::asio::ip::tcp::resolver resolver(io_context);
        boost::asio::ip::tcp::resolver::results_type endpoints = resolver.resolve(boost::asio::ip::host_name(), "");

        std::vector<std::string> networkInterfaces;
        networkInterfaces.push_back("127.0.0.1");
        for (auto it = endpoints.begin(); it != endpoints.end(); ++it) {
            boost::asio::ip::tcp::endpoint endpoint = *it;
            if (endpoint.address().is_v4()) {
                networkInterfaces.push_back(endpoint.address().to_string());
            }
        }

        availableNetworkInterfaces = networkInterfaces;
    }

    static bool initNetworkInterfaces(std::string preferredInterface){
        scanNetworkInterfaces();

        if(availableNetworkInterfaces.size() == 0)
            return false;

        int preferredInterfaceIdx = findNetworkInterfaceId(preferredInterface);
        if(preferredInterface != "" && preferredInterfaceIdx != -1){
            Settings::instance.networkInterface = preferredInterface;
        } else {
            Settings::instance.networkInterface = availableNetworkInterfaces[0];
        }

        return true;
    }

    static std::string getBroadcastAddr(){
        std::string currentInterface = Settings::instance.networkInterface;
        size_t npos = currentInterface.find_last_of('.');

        if (npos == std::string::npos){
            std::cerr << "No useful IP interface addr given in findRemoteCams call!" << std::endl;
            return "";
        }

        return currentInterface.substr(0, npos + 1) + "255";
    }

    static int findNetworkInterfaceId(std::string searchedInterface){
        int result = -1;
        for(int i=0; i < availableNetworkInterfaces.size(); ++i){
            std::string& iface = availableNetworkInterfaces[i];
            if(iface == searchedInterface)
                result = i;
        }

        return result;
    }

};
