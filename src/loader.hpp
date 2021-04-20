#pragma once

#include <vector>
#include <string>
#include <tuple>
#include <filesystem>

#include <arbor/morph/morphology.hpp>

#include "utils.hpp"

namespace io {

struct loaded_morphology {
    arb::morphology morph;
    std::vector<std::pair<std::string, std::string>> regions;
    std::vector<std::pair<std::string, std::string>> locsets;
};

loaded_morphology load_neuron_swc(const std::filesystem::path&);
loaded_morphology load_arbor_swc(const std::filesystem::path&);
loaded_morphology load_neuroml(const std::filesystem::path&);
loaded_morphology load_asc(const std::filesystem::path&);

const std::vector<std::string>& get_suffixes();
const std::vector<std::string>& get_flavors(const std::string& suffix);

struct loader_state {
    std::string message;
    std::optional<std::function<loaded_morphology(const std::filesystem::path&)>> load;
};

loader_state get_loader(const std::string& extension, const std::string& flavor);

}
