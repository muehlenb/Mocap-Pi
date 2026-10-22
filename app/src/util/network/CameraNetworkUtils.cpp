#include "src/util/network/CameraNetworkUtils.h"

std::vector<std::string> CameraNetworkUtils::availableNetworkInterfaces;
std::vector<RemoteCam> CameraNetworkUtils::remoteCams;

AutoJoinThread CameraNetworkUtils::scanThread;
bool CameraNetworkUtils::scanThreadFinished = true;
