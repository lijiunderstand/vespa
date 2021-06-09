// Copyright Verizon Media. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.

#pragma once

#include "rpcserver.h"
#include <vespa/config-sentinel.h>
#include <vespa/config-model.h>
#include <string>
#include <map>

using cloud::config::SentinelConfig;
using cloud::config::ModelConfig;

namespace config::sentinel {

/**
 * Utility class for running connectivity check.
 **/
class Connectivity {
public:
    Connectivity();
    ~Connectivity();
    void configure(const SentinelConfig::Connectivity &config);
    bool checkConnectivity(RpcServer &rpcServer);
private:
    SentinelConfig::Connectivity _config;
    std::map<std::string, std::string> _checkSpecs;
    std::map<std::string, std::string> _detailsPerHost;
};

}