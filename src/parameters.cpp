namespace arb {
    void to_json(json& j, const cable_cell_ion_data& d) {
        j = {{"int_concentration",         d.init_int_concentration},
             {"ext_concentration",         d.init_ext_concentration},
             {"reversal_potential",        d.init_reversal_potential}};
    }

    void to_json(json& j, const mechanism_desc& d) {
        json ps;
        for (const auto& [k, v]: d.values()) {
            ps[k] = v;
        }
        j = json{{"name", d.name()},
                 {"parameters", ps}};
    }

    std::string to_string(const arb::locset& ls) {
        std::stringstream ss;
        ss << ls;
        return ss.str();
    }

    std::string to_string(const arb::region& ls) {
        std::stringstream ss;
        ss << ls;
        return ss.str();
    }
}

struct region {
    std::unordered_map<std::string, arb::mechanism_desc> mechanisms;
    arb::cable_cell_parameter_set values;
    arb::region location;
};

void to_json(json& j, const region& r) {
    json parameters = {{"membrane_capacitance",    r.values.membrane_capacitance.value_or(NAN)},
                       {"axial_resistivity",       r.values.axial_resistivity.value_or(NAN)},
                       {"temperature_K",           r.values.temperature_K.value_or(NAN)}};
    json mechs;
    for (const auto& [mech, desc]: r.mechanisms) {
        json d = desc;
        d["location"] = to_string(r.location);
        mechs.push_back(d);
    }
    json ions;
    for (const auto& [ion, desc]: r.values.ion_data) {
        json d = desc;
        d["name"] = ion;
        ions.push_back(d);
    }
    j = json{{"parameters", parameters},
             {"ions", ions},
             {"mechanisms", mechs}};
}

struct probe {
    arb::locset location;
    double frequency;
    std::string variable;
    probe(const arb::locset& s, double f, const std::string& v) : location{s}, frequency{f}, variable{v} {}
};

void to_json(json& j, const probe& p) {
    j = json{{"location", to_string(p.location)},
             {"frequency", p.frequency},
             {"variable", p.variable}};
}

struct stimulus {
    arb::locset location;
    arb::i_clamp clamp;
    stimulus(const arb::locset& s, double d, double l, double a) : location{s}, clamp{d, l, a} {}
};


void to_json(json& j, const stimulus& c) {
    j = json{{"location",  to_string(c.location)},
             {"delay",     c.clamp.delay},
             {"duration",  c.clamp.duration},
             {"amplitude", c.clamp.amplitude}};
}

struct simulation {
    double t_stop  = 1400;
    double dt = 1.0/200.0;
    std::vector<stimulus> stimuli = {{"center", 200, 1000, 0.15}};
    std::vector<probe> probes = {{"center", 1000.0, "voltage"}};
};

void to_json(json& j, const simulation& s) {
    j = json{{"t_stop", s.t_stop},
             {"dt", s.dt},
             {"probes", s.probes},
             {"stimuli", s.stimuli}};
}

struct parameters {
    arb::cable_cell_parameter_set values;
    std::unordered_map<std::string, region> regions;

    std::vector<arb::swc_record> swc;
    arb::segment_tree tree;
    arb::morphology morph;
    arb::label_dict labels;
    simulation sim;

    parameters(const std::string& swc_fn):
        values{arb::neuron_parameter_defaults}
    {
        log_debug("Reading {}", swc_fn);
        std::ifstream in(swc_fn.c_str());
        swc    = arb::parse_swc_file(in);
        tree   = arb::swc_as_segment_tree(swc);
        morph  = arb::morphology{tree};
        log_debug("Generated morphology");
        log_debug("Tagging regions");
        // These are for SWC files.
        add_region("soma", arb::reg::tagged(1));
        add_region("axon", arb::reg::tagged(2));
        add_region("dend", arb::reg::tagged(3));
        add_region("apic", arb::reg::tagged(4));

        // This is for placing debug things
        labels.set("center", arb::ls::location(0, 0.5));

        log_debug("Parameters generated");
    }

    void add_region(const std::string& name, const arb::region& ls) {
        if (regions.find(name) != regions.end()) log_error("Duplicate region name");
        labels.set(name, ls);
        regions[name].location = ls;
    }

    void set_mech(const std::string& region, const std::string& mech, const std::string& key, double value) {
        if (regions.find(region) == regions.end()) log_error("Unknown region");
        auto& mechs = regions[region].mechanisms;
        if (mechs.find(mech) == mechs.end()) mechs[mech] = arb::mechanism_desc{mech};
        mechs[mech].set(key, value);
    }

    double get_mech(const std::string& region, const std::string& mech, const std::string& key) {
        if (regions.find(region) == regions.end()) log_error("Unknown region");
        auto& mechs = regions[region].mechanisms;
        if (mechs.find(mech) == mechs.end()) mechs[mech] = arb::mechanism_desc{mech};
        return mechs[mech][key];
    }

    void set_parameter(const std::string& region, const std::string& key, double value) {
        if (regions.find(region) == regions.end()) log_error("Unknown region");
        if      (key == "axial_resistivity")       regions[region].values.axial_resistivity = value;
        else if (key == "temperature_K")           regions[region].values.temperature_K = value;
        else if (key == "init_membrane_potential") regions[region].values.init_membrane_potential = value;
        else if (key == "membrane_capacitance")    regions[region].values.membrane_capacitance = value;
        else log_error("Unknown key: '{}'");
    }

    double get_parameter(const std::string& region, const std::string& key) {
        if (regions.find(region) == regions.end()) log_error("Unknown region");
        if      (key == "axial_resistivity")       return regions[region].values.axial_resistivity.value_or(values.axial_resistivity.value());
        else if (key == "temperature_K")           return regions[region].values.temperature_K.value_or(values.temperature_K.value());
        else if (key == "init_membrane_potential") return regions[region].values.init_membrane_potential.value_or(values.init_membrane_potential.value());
        else if (key == "membrane_capacitance")    return regions[region].values.membrane_capacitance.value_or(values.membrane_capacitance.value());
        log_error("Unknown key: '{}'");
    }

    void set_parameter(const std::string& key, double value) {
        if      (key == "axial_resistivity")       values.axial_resistivity = value;
        else if (key == "temperature_K")           values.temperature_K = value;
        else if (key == "init_membrane_potential") values.init_membrane_potential = value;
        else if (key == "membrane_capacitance")    values.membrane_capacitance = value;
        else log_error("Unknown key: '{}'");
    }

    double get_parameter(const std::string& key) {
        if      (key == "axial_resistivity")       return values.axial_resistivity.value_or(values.axial_resistivity.value());
        else if (key == "temperature_K")           return values.temperature_K.value_or(values.temperature_K.value());
        else if (key == "init_membrane_potential") return values.init_membrane_potential.value_or(values.init_membrane_potential.value());
        else if (key == "membrane_capacitance")    return values.membrane_capacitance.value_or(values.membrane_capacitance.value());
        log_error("Unknown key: '{}'");
    }

    double get_ion(const std::string& region, const std::string& name, const std::string& key) {
        if (regions.find(region) == regions.end()) log_error("{}: Unknown region: '{}'", __FUNCTION__, region);
        if (values.ion_data.find(name) == values.ion_data.end()) log_error("{}: Unknown ion: '{}'", __FUNCTION__, name);
        auto& ion = values.ion_data[name];
        if (regions[region].values.ion_data.find(name) != regions[region].values.ion_data.end()) ion = regions[region].values.ion_data[name];
        if (key == "init_int_concentration")  return ion.init_int_concentration;
        if (key == "init_ext_concentration")  return ion.init_ext_concentration;
        if (key == "init_reversal_potential") return ion.init_reversal_potential;
        log_error("Unknown key: '{}'");
    }

    void set_reversal_potential_method(const std::string& ion, int method) {
        if (values.ion_data.find(ion) == values.ion_data.end()) log_error("{}: Unknown ion: '{}'", __FUNCTION__, ion);
        auto& methods = values.reversal_potential_method;
        std::string nernst = fmt::format("nernst/x={}", ion);
        if      (method == 0) methods.erase(ion);
        else if (method == 1) methods[ion] = arb::mechanism_desc{nernst};
        else log_error("Unknown method: '{}'", method);
    }

    int get_reversal_potential_method(const std::string& ion) {
        if (values.ion_data.find(ion) == values.ion_data.end()) log_error("{}: Unknown ion: '{}'", __FUNCTION__, ion);
        auto& methods = values.reversal_potential_method;
        std::string nernst = fmt::format("nernst/x={}", ion);
        if (methods.find(ion) == methods.end()) return 0;
        if (methods[ion].name() == nernst)      return 1;
        log_fatal("Unknown method: '{}'", methods[ion].name());
    }

    std::string get_reversal_potential_method_as_string(const std::string& ion) {
        return reversal_potential_methods[get_reversal_potential_method(ion)];
    }

    double get_ion(const std::string& name, const std::string& key) {
        if (values.ion_data.find(name) == values.ion_data.end()) log_error("{}: Unknown ion: '{}'", __FUNCTION__, name);
        auto& ion = values.ion_data[name];
        if (key == "init_int_concentration")  return ion.init_int_concentration;
        if (key == "init_ext_concentration")  return ion.init_ext_concentration;
        if (key == "init_reversal_potential") return ion.init_reversal_potential;
        log_error("Unknown key: '{}' for ion '{}'", key, name);
    }

    void set_ion(const std::string& region, const std::string& name, const std::string& key, double value) {
        auto& ion = regions[region].values.ion_data[name];
        if      (key == "init_int_concentration")  ion.init_int_concentration  = value;
        else if (key == "init_ext_concentration")  ion.init_ext_concentration  = value;
        else if (key == "init_reversal_potential") ion.init_reversal_potential = value;
        else log_error("Unknown key: '{}'", key);
    }

    void set_ion(const std::string& name, const std::string& key, double value) {
        auto& ion = values.ion_data[name];
        if      (key == "init_int_concentration")  ion.init_int_concentration  = value;
        else if (key == "init_ext_concentration")  ion.init_ext_concentration  = value;
        else if (key == "init_reversal_potential") ion.init_reversal_potential = value;
        else log_error("Unknown key: '{}'");
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

void to_json(json& j, const parameters& p) {
    json parameters = {{"membrane_capacitance",    p.values.membrane_capacitance.value_or(NAN)},
                       {"axial_resistivity",       p.values.axial_resistivity.value_or(NAN)},
                       {"temperature_K",           p.values.temperature_K.value_or(NAN)},
                       {"init_membrane_potential", p.values.init_membrane_potential.value_or(NAN)}};
    json ions;
    for (const auto& [ion, desc]: p.values.ion_data) {
        ions[ion] = desc;
        if (p.values.reversal_potential_method.find(ion) != p.values.reversal_potential_method.end()) {
            ions[ion]["reversal_potential_method"] = p.values.reversal_potential_method.at(ion).name();
        } else {
            ions[ion]["reversal_potential_method"] = "const";
        }
    }
    json globals = json{{"ions", ions},
                        {"parameters", parameters}};
    json locals;
    json mechanisms;
    for (const auto& [region, data]: p.regions) {
        // Parameter settings
        json parameters = {{"membrane_capacitance",    data.values.membrane_capacitance.value_or(NAN)},
                           {"axial_resistivity",       data.values.axial_resistivity.value_or(NAN)},
                           {"temperature_K",           data.values.temperature_K.value_or(NAN)}};
        json ions;
        for (const auto& [ion, desc]: data.values.ion_data) {
            json d = desc;
            d["name"] = ion;
            ions.push_back(d);
        }
        json local = {{"location", to_string(data.location)},
                      {"parameters", parameters},
                      {"ions", ions}};
        locals.push_back(local);

        // mechanisms
        for (const auto& [mech, desc]: data.mechanisms) {
            json d = desc;
            d["location"] = to_string(data.location);
            mechanisms.push_back(d);
        }
    }

    j = json{{"electrical", {{"global", globals},
                             {"local",  locals},
                             {"mechanisms", mechanisms}}},
             {"simulation", p.sim},
             {"morphology",  {{"kind", "swc"},
                              {"file", "data/full.swc"}}}};
}
