// Simulated populations for the choisys landing: two use cases, one JSON document.
//
//   make cases                                   (or: g++ -std=c++17 examples/simulate_cases.cpp -o build/simulate-cases)
//   ./build/simulate-cases > cases.json          (fixed seed: the same bytes on every machine)
//   ./build/simulate-cases 20261028 > cases.json (seed)
//
// What is simulated and what is computed:
// - The BEHAVIOUR is synthetic and documented in docs/SIMULATED-CASES.md: made-up segments choose cells with
//   stated preferences, and some people leave before finishing. Nothing here describes real people.
// - Every Run goes through the real library (scenarys::Run). Counts, the funnel and the similarity with a
//   reference (scenarys::compare) are computed here, in C++, so the landing only draws numbers.
// - Determinism: std::mt19937 is fully specified by the standard; the distributions of <random> are not, so
//   choices are drawn with our own `pick` and the output is identical on Linux, macOS and Windows.
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <random>
#include <string>
#include <vector>

#include "../src/core/cube.hpp"

namespace {

using Weights = std::array<double, 3>;
constexpr std::size_t cells = scenarys::rowCount * scenarys::columnCount;

double uniform(std::mt19937& random) { return static_cast<double>(random()) / 4294967296.0; }

std::size_t pick(const Weights& weights, std::mt19937& random) {
    const double total = weights[0] + weights[1] + weights[2];
    double point = uniform(random) * total;
    for (std::size_t i = 0; i < 2; ++i) {
        if (point < weights[i]) return i;
        point -= weights[i];
    }
    return 2;
}

// A preference of 1 for `index`, `spread` for its neighbours and spread² further away.
Weights around(std::size_t index, double spread) {
    Weights w{};
    for (std::size_t i = 0; i < 3; ++i) {
        const std::size_t distance = i > index ? i - index : index - i;
        w[i] = distance == 0 ? 1.0 : distance == 1 ? spread : spread * spread;
    }
    return w;
}

std::string measurementsJson(const std::vector<scenarys::Measurement>& run) {
    std::string json = "[";
    for (const auto& m : run) {
        if (json.size() > 1) json += ',';
        json += "[" + std::to_string(m.phase) + "," + std::to_string(m.row) + "," + std::to_string(m.column) + "]";
    }
    return json + "]";
}

std::string tenths(std::size_t value) { return std::to_string(value / 10) + "." + std::to_string(value % 10); }

template <typename T, std::size_t N>
std::string list(const std::array<T, N>& values) {
    std::string json = "[";
    for (std::size_t i = 0; i < N; ++i) json += (i ? "," : "") + std::to_string(values[i]);
    return json + "]";
}

// ---------------------------------------------------------------------------------------------------------
// Case 1. Product selection: three phases narrow a purchase down to one concrete product.
//   Phase 1  rows = budget (low, mid, high)            columns = use (home, commute, sport)
//   Phase 2  rows = priority (sound, battery, comfort) columns = format (in-ear, over-ear, open)
//   Phase 3  rows = range (basic, mid, premium)        columns = type (in-ear, over-ear, sport)  → 9 products
// Phase 3 follows the earlier answers: the range leans to the budget row, the type to the format column.
// ---------------------------------------------------------------------------------------------------------
struct Buyer {
    const char* id;
    double share;  // part of the population
    Weights budget, use, priority, format;
    double exitRate;  // chance of leaving at each phase
};

constexpr std::array<Buyer, 4> buyers{{
    {"estudiante", 0.34, {0.70, 0.25, 0.05}, {0.25, 0.60, 0.15}, {0.30, 0.55, 0.15}, {0.65, 0.20, 0.15}, 0.07},
    {"deportista", 0.22, {0.25, 0.55, 0.20}, {0.10, 0.15, 0.75}, {0.15, 0.25, 0.60}, {0.40, 0.05, 0.55}, 0.05},
    {"viajero", 0.24, {0.10, 0.40, 0.50}, {0.15, 0.75, 0.10}, {0.25, 0.55, 0.20}, {0.20, 0.70, 0.10}, 0.05},
    {"audiofilo", 0.20, {0.05, 0.25, 0.70}, {0.70, 0.20, 0.10}, {0.75, 0.10, 0.15}, {0.15, 0.75, 0.10}, 0.04},
}};

void productCase(std::mt19937& random, std::size_t people) {
    std::array<std::size_t, 4> reached{};  // started, after phase 1, after phase 2, completed
    std::array<std::array<std::size_t, cells>, 3> counts{};
    // For every (phase-1 cell, phase-2 cell): how many people chose each product.
    std::vector<std::array<std::size_t, cells>> after(cells * cells);
    std::array<std::size_t, 4> segmentCompleted{};
    std::vector<std::string> sample;

    for (std::size_t person = 0; person < people; ++person) {
        std::size_t segment = 0;
        for (double point = uniform(random), acc = 0; segment < buyers.size() - 1; ++segment) {
            acc += buyers[segment].share;
            if (point < acc) break;
        }
        const Buyer& b = buyers[segment];
        scenarys::Run run;
        run.start();
        ++reached[0];
        std::array<std::size_t, 3> row{}, column{};
        bool left = false;
        for (std::size_t phase = 0; phase < 3; ++phase) {
            if (uniform(random) < b.exitRate) { run.finish("USER EXIT"); left = true; break; }
            if (phase == 0) { row[0] = pick(b.budget, random); column[0] = pick(b.use, random); }
            if (phase == 1) { row[1] = pick(b.priority, random); column[1] = pick(b.format, random); }
            if (phase == 2) {
                row[2] = pick(around(row[0], 0.30), random);
                // A sport use pulls the type to "sport"; otherwise it follows the format (open → sport).
                column[2] = column[0] == 2 && uniform(random) < 0.6 ? 2 : pick(around(column[1], 0.25), random);
            }
            run.select(row[phase], column[phase]);
            ++reached[phase + 1];
            ++counts[phase][row[phase] * 3 + column[phase]];
        }
        if (left) continue;
        ++segmentCompleted[segment];
        ++after[(row[0] * 3 + column[0]) * cells + (row[1] * 3 + column[1])][row[2] * 3 + column[2]];
        if (sample.size() < 60) sample.push_back(measurementsJson(run.measurements()));
    }

    // Self-check: everyone who completed is counted exactly once per earlier combination, and the funnel only narrows.
    std::size_t combined = 0;
    for (const auto& products : after) for (const auto n : products) combined += n;
    assert(combined == reached[3] && reached[0] >= reached[1] && reached[1] >= reached[2] && reached[2] >= reached[3]);
    std::cout << "  \"product\": {\n    \"people\": " << people << ",\n    \"funnel\": " << list(reached)
              << ",\n    \"segmentsCompleted\": {";
    for (std::size_t s = 0; s < buyers.size(); ++s) std::cout << (s ? "," : "") << "\"" << buyers[s].id << "\":" << segmentCompleted[s];
    std::cout << "},\n    \"counts\": [" << list(counts[0]) << "," << list(counts[1]) << "," << list(counts[2]) << "],\n"
              << "    \"after\": [";
    for (std::size_t i = 0; i < after.size(); ++i) std::cout << (i ? "," : "") << list(after[i]);
    std::cout << "],\n    \"sample\": [";
    for (std::size_t i = 0; i < sample.size(); ++i) std::cout << (i ? "," : "") << sample[i];
    std::cout << "]\n  }";
}

// ---------------------------------------------------------------------------------------------------------
// Case 2. Hiring: candidates answer the cube of a role and are compared with its reference run.
//   The reference is the run the company recorded for the role. Candidates come from four archetypes:
//   each archetype answers around its own cells; how close they land is a model, not merit.
// ---------------------------------------------------------------------------------------------------------
struct Archetype {
    const char* id;
    double share;
    std::array<std::array<std::size_t, 2>, 3> centre;  // 0-based (row, column) per phase
    double spread;                                       // neighbour weight around the centre
};

constexpr std::array<std::array<std::size_t, 2>, 3> roleReference{{{2, 0}, {1, 1}, {0, 2}}};

constexpr std::array<Archetype, 4> archetypes{{
    {"afin", 0.25, {{{2, 0}, {1, 1}, {0, 2}}}, 0.60},
    {"proximo", 0.30, {{{1, 0}, {1, 2}, {0, 1}}}, 0.55},
    {"distinto", 0.30, {{{0, 2}, {0, 0}, {2, 0}}}, 0.40},
    {"disperso", 0.15, {{{1, 1}, {1, 1}, {1, 1}}}, 0.95},
}};

void hiringCase(std::mt19937& random, std::size_t invited) {
    const double openRate = 0.72, exitRate = 0.06;
    std::array<std::size_t, 3> funnel{};  // invited, opened, completed
    funnel[0] = invited;
    std::vector<scenarys::Measurement> reference;
    for (std::size_t p = 0; p < 3; ++p) reference.push_back({p + 1, roleReference[p][0] + 1, roleReference[p][1] + 1});
    struct Result { std::size_t id; std::size_t archetype; scenarys::Comparison comparison; std::string run; };
    std::vector<Result> results;
    std::array<std::size_t, 10> histogram{};  // [0,10) … [90,100] %
    std::array<std::size_t, 3> phaseTotal{};

    for (std::size_t person = 0; person < invited; ++person) {
        std::size_t a = 0;
        for (double point = uniform(random), acc = 0; a < archetypes.size() - 1; ++a) {
            acc += archetypes[a].share;
            if (point < acc) break;
        }
        if (uniform(random) >= openRate) continue;
        ++funnel[1];
        scenarys::Run run;
        run.start();
        bool left = false;
        for (std::size_t phase = 0; phase < 3; ++phase) {
            if (uniform(random) < exitRate) { run.finish("USER EXIT"); left = true; break; }
            const auto& c = archetypes[a].centre[phase];
            const std::size_t row = pick(around(c[0], archetypes[a].spread), random);
            run.select(row, pick(around(c[1], archetypes[a].spread), random));
        }
        if (left) continue;
        ++funnel[2];
        const auto comparison = scenarys::compare(scenarys::defaultShape(), reference, run.measurements());
        ++histogram[comparison.tenths >= 1000 ? 9 : comparison.tenths / 100];
        for (std::size_t p = 0; p < 3; ++p) phaseTotal[p] += comparison.phases[p].tenths;
        results.push_back({person + 1, a, comparison, measurementsJson(run.measurements())});
    }
    // Ranking: higher similarity first; ties keep invitation order (stable and reproducible).
    std::vector<std::size_t> order(results.size());
    for (std::size_t i = 0; i < order.size(); ++i) order[i] = i;
    for (std::size_t i = 1; i < order.size(); ++i)
        for (std::size_t j = i; j > 0 && results[order[j]].comparison.tenths > results[order[j - 1]].comparison.tenths; --j)
            std::swap(order[j], order[j - 1]);

    std::cout << "  \"hiring\": {\n    \"funnel\": " << list(funnel) << ",\n    \"reference\": " << measurementsJson(reference)
              << ",\n    \"histogram\": " << list(histogram) << ",\n    \"phaseMean\": [";
    for (std::size_t p = 0; p < 3; ++p)
        std::cout << (p ? "," : "") << tenths(results.empty() ? 0 : (phaseTotal[p] + results.size() / 2) / results.size());
    std::cout << "],\n    \"ranking\": [";
    for (std::size_t k = 0; k < order.size() && k < 12; ++k) {
        const auto& r = results[order[k]];
        std::cout << (k ? "," : "") << "\n      {\"id\":" << r.id << ",\"archetype\":\"" << archetypes[r.archetype].id
                  << "\",\"similarity\":" << tenths(r.comparison.tenths) << ",\"phases\":[" << tenths(r.comparison.phases[0].tenths) << ","
                  << tenths(r.comparison.phases[1].tenths) << "," << tenths(r.comparison.phases[2].tenths) << "],\"run\":" << r.run << "}";
    }
    std::cout << "\n    ]\n  }\n";
}

}  // namespace

int main(int argc, char** argv) {
    const unsigned long seed = argc > 1 ? std::strtoul(argv[1], nullptr, 10) : 20261028UL;
    std::mt19937 random(static_cast<std::mt19937::result_type>(seed));
    std::cout << "{\n  \"seed\": " << seed << ",\n";
    productCase(random, 5000);
    std::cout << ",\n";
    hiringCase(random, 400);
    std::cout << "}\n";
    return 0;
}
