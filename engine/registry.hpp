#pragma once

// InstanceRegistry — process-local discovery of analyzer instances, so a later
// instance can select an earlier one as a comparison source BY NAME (the
// before/after diff's "source" picker).
//
// Scope (load-bearing, see ROADMAP): this is IN-PROCESS only. It works when
// instances share one address space — the common case for a single host
// process. Sandboxed AU/VST3/CLAP instances can land in separate processes; a
// cross-process source needs shared-memory IPC and is a deliberate, larger
// follow-up. Instances select by NAME (not a transient runtime id) so a missing
// remote stays selected-by-name and simply reads as unavailable.
//
// Thread-safety: a mutex guards the registry. This is control-plane work
// (registration on construct/destruct, UI selection) — never the audio thread,
// which only ever touches its own lock-free publisher.

#include "spectrum_snapshot.hpp"

#include <functional>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

namespace pulp::analyzer {

// A named source of the latest spectrum (the published snapshot reader).
class SpectrumSource {
public:
    virtual ~SpectrumSource() = default;
    virtual std::string source_name() const = 0;
    virtual SpectrumSnapshot latest_spectrum() const = 0;
};

class InstanceRegistry {
public:
    InstanceRegistry() = default;

    // Process-global registry (function-local static — one per process). A
    // separate InstanceRegistry can still be constructed directly (e.g. for an
    // isolated test or an embedding that wants its own scope).
    static InstanceRegistry& instance() {
        static InstanceRegistry reg;
        return reg;
    }

    void register_source(SpectrumSource* src) {
        if (!src) return;
        std::lock_guard<std::mutex> lock(mu_);
        for (auto* s : sources_) {
            if (s == src) return;  // idempotent
        }
        sources_.push_back(src);
    }

    void deregister_source(SpectrumSource* src) {
        std::lock_guard<std::mutex> lock(mu_);
        for (std::size_t i = 0; i < sources_.size(); ++i) {
            if (sources_[i] == src) {
                sources_.erase(sources_.begin() + static_cast<long>(i));
                return;
            }
        }
    }

    // Names of all currently-registered sources (snapshot copy).
    std::vector<std::string> names() const {
        std::lock_guard<std::mutex> lock(mu_);
        std::vector<std::string> out;
        out.reserve(sources_.size());
        for (auto* s : sources_) out.push_back(s->source_name());
        return out;
    }

    // The source registered under @p name, or nullptr if none (e.g. it was
    // destroyed — selected-by-name remotes simply read as unavailable).
    SpectrumSource* find(const std::string& name) const {
        std::lock_guard<std::mutex> lock(mu_);
        for (auto* s : sources_) {
            if (s->source_name() == name) return s;
        }
        return nullptr;
    }

    std::size_t size() const {
        std::lock_guard<std::mutex> lock(mu_);
        return sources_.size();
    }

private:
    mutable std::mutex mu_;
    std::vector<SpectrumSource*> sources_;
};

// RAII source: registers on construction, deregisters on destruction (so a
// destroyed instance is cleaned out of the registry automatically — no stale
// dangling pointers). The snapshot provider is supplied by the owner (typically
// reading its RtSpectrumPublisher's TripleBuffer).
class RegisteredSpectrumSource : public SpectrumSource {
public:
    using Provider = std::function<SpectrumSnapshot()>;

    RegisteredSpectrumSource(std::string name, Provider provider,
                             InstanceRegistry& registry = InstanceRegistry::instance())
        : name_(std::move(name)),
          provider_(std::move(provider)),
          registry_(registry) {
        registry_.register_source(this);
    }

    ~RegisteredSpectrumSource() override { registry_.deregister_source(this); }

    RegisteredSpectrumSource(const RegisteredSpectrumSource&) = delete;
    RegisteredSpectrumSource& operator=(const RegisteredSpectrumSource&) = delete;

    std::string source_name() const override { return name_; }
    SpectrumSnapshot latest_spectrum() const override {
        return provider_ ? provider_() : SpectrumSnapshot{};
    }

private:
    std::string name_;
    Provider provider_;
    InstanceRegistry& registry_;
};

} // namespace pulp::analyzer
