#include <arbor/morph/morphology.hpp>
#include <arbor/cable_cell.hpp>
#include <arbor/load_balance.hpp>
#include <arbor/recipe.hpp>
#include <arbor/simulation.hpp>
#include <arbor/swcio.hpp>

struct probe_site {
    arb::mlocation site;  // Location of sample on morphology.
    double frequency;     // Sampling frequency [Hz].
};

struct global_props_shim {
    arb::mechanism_catalogue cat;
    arb::cable_cell_global_properties props;
    global_props_shim() {
        cat = arb::global_allen_catalogue();
        cat.import(arb::global_default_catalogue(), "");
        props.catalogue = &cat;
    }
};

// Stores a single trace, which can be queried and viewed by the user at the end
// of a simulation run.
struct trace {
    std::string variable;           // Name of the variable being recorded.
    arb::mlocation loc;             // Sample site on morphology.
    std::vector<arb::time_type> t;  // Sample times [ms].
    std::vector<double> v;          // Sample values [units specific to sample variable].
};

// Callback provided to sampling API that records into a trace variable.
struct trace_callback {
    trace& trace_;

    trace_callback(trace& t): trace_(t) {}

    void operator()(arb::probe_metadata, std::size_t n, const arb::sample_record* recs) {
        // Push each (time, value) pair from the last epoch into trace_.
        for (std::size_t i=0; i<n; ++i) {
            if (auto p = arb::util::any_cast<const double*>(recs[i].data)) {
                trace_.t.push_back(recs[i].time);
                trace_.v.push_back(*p);
            }
            else log_error("unexpected sample type");
        }
    }
};

// Used internally by the single cell model to expose model information to the
// arb::simulation API when a model is instantiated.
// Model descriptors, i.e. the cable_cell and probes, are instantiated
// in the single_cell_model by user calls. The recipe is generated lazily, just
// before simulation construction, so the recipe can use const references to all
// of the model descriptors.
struct single_cell_recipe: arb::recipe {
    const arb::cable_cell& cell_;
    const std::vector<probe_site>& probes_;
    const arb::cable_cell_global_properties& gprop_;

    single_cell_recipe(
            const arb::cable_cell& c,
            const std::vector<probe_site>& probes,
            const arb::cable_cell_global_properties& props):
        cell_(c), probes_(probes), gprop_(props)
    {}

    virtual arb::cell_size_type num_cells() const override {
        return 1;
    }

    virtual arb::util::unique_any get_cell_description(arb::cell_gid_type gid) const override {
        return cell_;
    }

    virtual arb::cell_kind get_cell_kind(arb::cell_gid_type) const override {
        return arb::cell_kind::cable;
    }

    virtual arb::cell_size_type num_sources(arb::cell_gid_type) const override {
        return cell_.detectors().size();
    }

    // synapses, connections and event generators

    virtual arb::cell_size_type num_targets(arb::cell_gid_type) const override {
        return cell_.synapses().size();
    }

    virtual std::vector<arb::cell_connection> connections_on(arb::cell_gid_type) const override {
        return {}; // no connections on a single cell model
    }

    virtual std::vector<arb::event_generator> event_generators(arb::cell_gid_type) const override {
        return {};
    }

    // probes

    virtual std::vector<arb::probe_info> get_probes(arb::cell_gid_type gid) const override {
        // For now only voltage can be selected for measurement.
        std::vector<arb::probe_info> pinfo;
        for (auto& p: probes_) {
            pinfo.push_back(arb::cable_probe_membrane_voltage{p.site});
        }
        return pinfo;
    }

    // gap junctions

    virtual arb::cell_size_type num_gap_junction_sites(arb::cell_gid_type gid)  const override {
        return 0; // No gap junctions on a single cell model.
    }

    virtual std::vector<arb::gap_junction_connection> gap_junctions_on(arb::cell_gid_type) const override {
        return {}; // No gap junctions on a single cell model.
    }

    virtual arb::util::any get_global_properties(arb::cell_kind) const override {
        return gprop_;
    }
};

struct single_cell_model {
    arb::cable_cell cell_;
    arb::context ctx_;
    bool run_ = false;

    std::vector<probe_site> probes_;
    std::unique_ptr<arb::simulation> sim_;
    std::vector<double> spike_times_;
    // Create one trace for each probe.
    std::vector<trace> traces_;
    // Make gprop public to make it possible to expose it as a field in Python:
    //      model.properties.catalogue = my_custom_cat
    //arb::cable_cell_global_properties gprop;
    global_props_shim gprop;

    single_cell_model(arb::cable_cell c):
        cell_(std::move(c)), ctx_(arb::make_context())
    {
        gprop.props.default_parameters = arb::neuron_parameter_defaults;
    }

    // example use:
    //      m.probe('voltage', arbor.location(2,0.5))
    //      m.probe('voltage', '(location 2 0.5)')
    //      m.probe('voltage', 'term')
    void probe(const std::string& what, const arb::locset& where, double frequency) {
        if (what != "voltage") log_error("{} does not name a valid variable to trace (currently only 'voltage' is supported)", what);
        if (frequency<=0) log_error("sampling frequency is not greater than zero");
        for (auto& l: cell_.concrete_locset(where)) probes_.push_back({l, frequency});
    }

    void run(double tfinal, double dt) {
        single_cell_recipe rec(cell_, probes_, gprop.props);

        auto domdec = arb::partition_load_balance(rec, ctx_);

        sim_ = std::make_unique<arb::simulation>(rec, domdec, ctx_);

        // Create one trace for each probe.
        traces_.reserve(probes_.size());

        // Add probes
        for (arb::cell_lid_type i=0; i<probes_.size(); ++i) {
            const auto& p = probes_[i];

            traces_.push_back({"voltage", p.site, {}, {}});

            auto sched = arb::regular_schedule(1000./p.frequency);

            // Now attach the sampler at probe site, with sampling schedule sched, writing to voltage
            sim_->add_sampler(arb::one_probe({0,i}), sched, trace_callback(traces_[i]));
        }

        // Set callback that records spike times.
        sim_->set_global_spike_callback(
            [this](const std::vector<arb::spike>& spikes) {
                for (auto& s: spikes) {
                    spike_times_.push_back(s.time);
                }
            });

        sim_->run(tfinal, dt);

        run_ = true;
    }

    const std::vector<double>& spike_times() const {
        return spike_times_;
    }

    const std::vector<trace>& traces() const {
        return traces_;
    }
};
