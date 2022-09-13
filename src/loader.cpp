#include <vector>
#include <string>
#include <tuple>
#include <filesystem>
#include <variant>

#include <arborio/cableio.hpp>
#include <arborio/neurolucida.hpp>
#include <arborio/neuroml.hpp>
#include <arborio/swcio.hpp>
#include <arbor/morph/morphology.hpp>

#include "loader.hpp"

namespace io {

loaded_morphology load_swc(const std::filesystem::path &fn,
                           std::function<arb::morphology(const std::vector<arborio::swc_record> &)> swc_to_morph) {
    return { swc_to_morph(arborio::parse_swc(slurp(fn)).records()),
             {{"soma",   "(tag 1)"},
              {"axon",   "(tag 2)"},
              {"dend",   "(tag 3)"},
              {"apic",   "(tag 4)"}},
             {}};
}

loaded_morphology load_neuron_swc(const std::filesystem::path &fn) { return load_swc(fn, arborio::load_swc_neuron); }
loaded_morphology load_arbor_swc(const std::filesystem::path &fn)  { return load_swc(fn, arborio::load_swc_arbor); }

loaded_morphology load_neuroml_morph(const std::filesystem::path &fn) {
    arborio::neuroml nml(slurp(fn));
    if (nml.morphology_ids().empty()) log_error("NML file {} has no morphologies.", fn.string());
    auto id = nml.morphology_ids().front();
    auto morph_data = nml.morphology(id);
    if (!morph_data) log_error("Invalid morphology id {} in NML file.");
    auto morph = morph_data.value();
    loaded_morphology result{.morph=morph.morphology};
    for (const auto& [k, v]: morph.groups.regions()) result.regions.emplace_back(k, to_string(v));
    for (const auto& [k, v]: morph.groups.locsets()) result.locsets.emplace_back(k, to_string(v));
    return result;
}

loaded_morphology load_neuroml_cell(const std::filesystem::path &fn) {
    arborio::neuroml nml(slurp(fn));
    if (nml.cell_ids().empty()) log_error("NML file {} has no cells.", fn.string());
    auto id = nml.cell_ids().front();
    auto morph_data = nml.cell_morphology(id);
    if (!morph_data) log_error("Invalid cell id {} in NML file.");
    auto morph = morph_data.value();
    loaded_morphology result{.morph=morph.morphology};
    for (const auto& [k, v]: morph.groups.regions()) result.regions.emplace_back(k, to_string(v));
    for (const auto& [k, v]: morph.groups.locsets()) result.locsets.emplace_back(k, to_string(v));
    return result;
}

loaded_morphology load_asc(const std::filesystem::path &fn) {
    auto m = arborio::load_asc(fn);
    loaded_morphology result{.morph=m.morphology};
    for (const auto& [k, v]: m.labels.regions()) result.regions.emplace_back(k, to_string(v));
    for (const auto& [k, v]: m.labels.locsets()) result.locsets.emplace_back(k, to_string(v));
    return result;
}

loaded_morphology load_acc(const std::filesystem::path &fn) {
    if (auto m = arborio::parse_component(slurp(fn))) {
       try {
           return loaded_morphology{.morph=std::get<arb::morphology>(m.value().component)};
       }
       catch (std::bad_variant_access const& ex) {
          log_error("ACC file {} doesn't describe a morphology.", fn.string());
       }
    }
    else {
       log_error("Error: {} in ACC file {}.", m.error().what(), fn.string());
    }
    return {};
}


static std::unordered_map<std::string,
                          std::unordered_map<std::string,
                                             std::function<loaded_morphology(const std::filesystem::path &fn)>>>
loaders{{".swc", {{"Arbor",   load_arbor_swc},
                  {"Neuron",  load_neuron_swc}}},
        {".nml", {{"Cell",    load_neuroml_cell},
                  {"Morph",   load_neuroml_morph}}},
        {".asc", {{"Default", load_asc}}},
        {".acc", {{"Default", load_acc}}}};

const std::vector<std::string>& get_suffixes() {
    static std::vector<std::string> result;
    if (result.empty()) {
        for (const auto& [k, v]: loaders) result.push_back(k);
    }
    return result;
}

const std::vector<std::string>& get_flavors(const std::string& suffix) {
    static std::unordered_map<std::string, std::vector<std::string>> result;
    if (result.empty()) {
        for (const auto& [s, kvs]: loaders) {
            for (const auto& [k, v]: kvs) {
                result[s].push_back(k);
            }
        }
    }
    return result[suffix];
}

loader_state get_loader(const std::string &extension, const std::string &flavor) {
    if (extension.empty())                    return {"Please select a file.",   {}};
    if (!loaders.contains(extension))         return {"Unknown file type.",      {}};
    if (flavor.empty())                       return {"Please select a flavor.", {}};
    if (!loaders[extension].contains(flavor)) return {"Unknown flavor type.",    {}};
    return {"Ok.", {loaders[extension][flavor]}};
}

}
