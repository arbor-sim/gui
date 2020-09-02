struct region {
    std::unordered_map<std::string, arb::mechanism_desc> mechanisms;
    arb::cable_cell_parameter_set values;
};

struct probe {
    arb::locset site;
    double frequency;
    std::string variable;

    probe(const arb::locset& s, double f, const std::string& v)
        : site{s}, frequency{f}, variable{v}
    {}
};

struct parameters {
    arb::cable_cell_parameter_set values;
    std::unordered_map<std::string, region> regions;
    std::vector<probe> probes;
    std::vector<arb::swc_record> swc;
    arb::segment_tree tree;
    arb::morphology morph;
    arb::label_dict labels;
    arb::i_clamp iclamp;

    parameters(const std::string& swc_fn):
        values{arb::neuron_parameter_defaults}
    {

        std::ifstream in(swc_fn.c_str());
        swc    = arb::parse_swc_file(in);
        tree   = arb::swc_as_segment_tree(swc);
        morph  = arb::morphology{tree};
        // These are for SWC files.
        labels.set("soma", arb::reg::tagged(1));
        labels.set("axon", arb::reg::tagged(2));
        labels.set("dend", arb::reg::tagged(3));
        labels.set("apic", arb::reg::tagged(4));
        labels.set("center", arb::ls::location(0, 0.5));
        // TODO This a default for testing
        probes.push_back(probe{arb::ls::location(0, 0.5), 1000.0, "voltage"});
    }

    void set_mech(const std::string& region, const std::string& mech, const std::string& key, double value) {
        auto& mechs = regions[region].mechanisms;
        if (mechs.find(mech) == mechs.end()) mechs[mech] = arb::mechanism_desc{mech};
        mechs[mech].set(key, value);
    }

    double get_mech(const std::string& region, const std::string& mech, const std::string& key) {
        auto& mechs = regions[region].mechanisms;
        if (mechs.find(mech) == mechs.end()) mechs[mech] = arb::mechanism_desc{mech};
        return mechs[mech][key];
    }

    void set_parameter(const std::string& region, const std::string& key, double value) {
        if (regions.find(region) == regions.end()) throw std::runtime_error("Unknown region");
        if      (key == "axial_resistivity")       regions[region].values.axial_resistivity = value;
        else if (key == "temperature_K")           regions[region].values.temperature_K = value;
        else if (key == "init_membrane_potential") regions[region].values.init_membrane_potential = value;
        else if (key == "membrane_capacitance")    regions[region].values.membrane_capacitance = value;
        else throw std::runtime_error(key + ": Unknown key");
    }

    double get_parameter(const std::string& region, const std::string& key) {
        if (regions.find(region) == regions.end()) throw std::runtime_error("Unknown region");
        if      (key == "axial_resistivity")       return regions[region].values.axial_resistivity.value_or(values.axial_resistivity.value());
        else if (key == "temperature_K")           return regions[region].values.temperature_K.value_or(values.temperature_K.value());
        else if (key == "init_membrane_potential") return regions[region].values.init_membrane_potential.value_or(values.init_membrane_potential.value());
        else if (key == "membrane_capacitance")    return regions[region].values.membrane_capacitance.value_or(values.membrane_capacitance.value());
        else throw std::runtime_error(key + ": Unknown key");
    }

    void set_parameter(const std::string& key, double value) {
        if      (key == "axial_resistivity")       values.axial_resistivity = value;
        else if (key == "temperature_K")           values.temperature_K = value;
        else if (key == "init_membrane_potential") values.init_membrane_potential = value;
        else if (key == "membrane_capacitance")    values.membrane_capacitance = value;
        else throw std::runtime_error(key + ": Unknown key");
    }

    double get_parameter(const std::string& key) {
        if      (key == "axial_resistivity")       return values.axial_resistivity.value_or(values.axial_resistivity.value());
        else if (key == "temperature_K")           return values.temperature_K.value_or(values.temperature_K.value());
        else if (key == "init_membrane_potential") return values.init_membrane_potential.value_or(values.init_membrane_potential.value());
        else if (key == "membrane_capacitance")    return values.membrane_capacitance.value_or(values.membrane_capacitance.value());
        else throw std::runtime_error(key + ": Unknown key");
    }

    double get_ion(const std::string& region, const std::string& name, const std::string& key) {
        if (regions.find(region) == regions.end()) throw std::runtime_error("Unknown region");
        if (values.ion_data.find(name) != values.ion_data.end()) throw std::runtime_error("Unknown ion");
        auto& ion = values.ion_data[name];
        if (regions[region].values.ion_data.find(name) != regions[region].values.ion_data.end()) ion = regions[region].values.ion_data[name];
        if (key == "init_int_concentration")  return ion.init_int_concentration;
        if (key == "init_ext_concentration")  return ion.init_ext_concentration;
        if (key == "init_reversal_potential") return ion.init_reversal_potential;
        throw std::runtime_error(key + ": Unknown key");
    }

    void set_reversal_potential_method(const std::string& ion, int method) {
        if (values.ion_data.find(ion) == values.ion_data.end()) throw std::runtime_error("Unknown ion");
        auto& methods = values.reversal_potential_method;
        std::string nernst = "nernst/x=" + ion;
        if      (method == 0)  methods.erase(ion);
        else if (method == 1) methods[ion] = arb::mechanism_desc{nernst};
        else throw std::runtime_error(method + " :Unknown method");
    }

    int get_reversal_potential_method(const std::string& ion) {
        if (values.ion_data.find(ion) == values.ion_data.end()) throw std::runtime_error("Unknown ion");
        auto& methods = values.reversal_potential_method;
        std::string nernst = "nernst/x=" + ion;
        if (methods.find(ion) == methods.end()) return 0;
        if (methods[ion].name() == nernst)      return 1;
        throw std::runtime_error(methods[ion].name() + " :Unknown method");
    }

    double get_ion(const std::string& name, const std::string& key) {
        if (values.ion_data.find(name) == values.ion_data.end()) throw std::runtime_error("Unknown ion");
        auto& ion = values.ion_data[name];
        if (key == "init_int_concentration")  return ion.init_int_concentration;
        if (key == "init_ext_concentration")  return ion.init_ext_concentration;
        if (key == "init_reversal_potential") return ion.init_reversal_potential;
        throw std::runtime_error(key + " :Unknown key");
    }

    void set_ion(const std::string& region, const std::string& name, const std::string& key, double value) {
        auto& ion = regions[region].values.ion_data[name];
        if      (key == "init_int_concentration")  ion.init_int_concentration  = value;
        else if (key == "init_ext_concentration")  ion.init_ext_concentration  = value;
        else if (key == "init_reversal_potential") ion.init_reversal_potential = value;
        else throw std::runtime_error(key + ": Unknown key");
    }

    void set_ion(const std::string& name, const std::string& key, double value) {
        auto& ion        = values.ion_data[name];
        if      (key == "init_int_concentration")  ion.init_int_concentration  = value;
        else if (key == "init_ext_concentration")  ion.init_ext_concentration  = value;
        else if (key == "init_reversal_potential") ion.init_reversal_potential = value;
        else throw std::runtime_error(key + " :Unknown key");
    }

    std::vector<std::string> get_ions() {
        std::vector<std::string> result;
        for (const auto& ion: values.ion_data) result.push_back(ion.first);
        return result;
    }

    std::vector<std::string> get_regions() {
        std::vector<std::string> result;
        for (const auto& region: regions) result.push_back(region.first);
        return result;
    }

    std::vector<std::string> get_mechs(const std::string& name) {
        std::vector<std::string> result;
        for (const auto& mech: regions[name].mechanisms) result.push_back(mech.first);
        return result;
    }

    const std::vector<const std::string> ion_keys = {"init_int_concentration", "init_ext_concentration", "init_reversal_potential"};
    const std::vector<const std::string> keys = {"temperature_K", "axial_resistivity", "init_membrane_potential", "membrane_capacitance"};
    static const char* reversal_potential_methods[];
};

const char* parameters::reversal_potential_methods[] = {"const", "nernst"};
