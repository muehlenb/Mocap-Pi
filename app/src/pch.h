// pch.h
#ifndef PCH_H
#define PCH_H

#include <omp.h>

#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <filesystem>
#include <iostream>
#include <mutex>
#include <set>
#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <tuple>
#include <thread>
#include <vector>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <boost/asio.hpp>
#include <boost/bind/bind.hpp>

#include <ceres/ceres.h>
#include <ceres/rotation.h>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui_internal.h>
#include <imgui.h>

#include <libssh/libssh.h>

#include <libssh/libssh.h>

#include <nlohmann/json.hpp>

#include <pcl/point_types.h>
#include <pcl/registration/transformation_estimation.h>
#include <pcl/registration/transformation_estimation_svd.h>
#include <pcl/registration/correspondence_rejection_sample_consensus.h>

#endif // PCH_H
