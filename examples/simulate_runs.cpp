// Simulates many Runs with the real Neo Cube library and prints one JSON document.
//
//   g++ -std=c++17 -Wall -Wextra -Wpedantic examples/simulate_runs.cpp -o build/simulate-runs
//   ./build/simulate-runs > runs-demo.json            (600 runs, fixed seed: always the same output)
//   ./build/simulate-runs 1000 42 > runs-demo.json    (runs, seed)
//
// The BEHAVIOUR is synthetic and only exists to demonstrate the library: three made-up profiles choose
// cells with different preferences and make a few mistakes. The library itself only RECORDS what it is
// given (selections, attempts, empty/invalid/outside inputs, early exits). It computes no inference, and
// nothing here describes real people.
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <random>
#include <string>
#include <vector>

#include "../src/core/cube.hpp"

namespace {

struct Profile {
    const char* id;
    std::array<double, 3> rowWeight;     // preference for top / middle / bottom
    std::array<double, 3> columnWeight;  // preference for left / middle / right
    double mistakeRate;                  // chance of a wrong input before choosing, per phase
    double exitRate;                     // chance of leaving before choosing, per phase
};

constexpr std::array<Profile, 3> profiles{{
    {"cautelosos", {0.25, 0.50, 0.25}, {0.25, 0.50, 0.25}, 0.05, 0.03},    // like the centre
    {"arriesgados", {0.45, 0.10, 0.45}, {0.45, 0.10, 0.45}, 0.10, 0.06},   // like the corners
    {"cambiantes", {1.0 / 3, 1.0 / 3, 1.0 / 3}, {1.0 / 3, 1.0 / 3, 1.0 / 3}, 0.08, 0.10},  // no preference
}};

std::size_t pick(const std::array<double, 3>& weights, std::mt19937& random) {
    std::discrete_distribution<std::size_t> distribution(weights.begin(), weights.end());
    return distribution(random);
}

void printRun(std::size_t id, const char* profile, const scenarys::Run& run) {
    std::cout << "    {\"id\":" << id << ",\"profile\":\"" << profile << "\""
              << ",\"completed\":" << (run.endReason == "COMPLETED" ? "true" : "false")
              << ",\"endReason\":\"" << run.endReason << "\""
              << ",\"totalAttempts\":" << run.totalAttempts()
              << ",\"completedSessions\":" << run.completedSessions() << ",\"sessions\":[";
    for (std::size_t phase = 0; phase < scenarys::phaseCount; ++phase) {
        const auto& s = run.sessions[phase];
        std::cout << (phase ? "," : "") << "{\"visited\":" << (s.visited ? "true" : "false")
                  << ",\"attempts\":" << s.attempts << ",\"empty\":" << s.emptyInputs
                  << ",\"invalid\":" << s.invalidInputs << ",\"outside\":" << s.outsideClicks << "}";
    }
    std::cout << "],\"measurements\":[";
    bool first = true;
    for (const auto& m : run.measurements()) {
        std::cout << (first ? "" : ",") << "{\"phase\":" << m.phase << ",\"row\":" << m.row << ",\"column\":" << m.column << "}";
        first = false;
    }
    std::cout << "]}";
}

}  // namespace

int main(int argc, char** argv) {
    const std::size_t total = argc > 1 ? static_cast<std::size_t>(std::strtoul(argv[1], nullptr, 10)) : 600;
    const std::uint32_t seed = argc > 2 ? static_cast<std::uint32_t>(std::strtoul(argv[2], nullptr, 10)) : 20261008;
    if (total == 0) { std::cerr << "usage: simulate-runs [runs>0] [seed]\n"; return 2; }

    std::mt19937 random(seed);
    std::cout << "{\n  \"generator\":\"examples/simulate_runs.cpp\",\"library\":\"neos-cube\",\"seed\":" << seed
              << ",\"synthetic\":true,\n  \"note\":\"Simulated behaviour to demonstrate the library; not real people.\",\n  \"runs\":[\n";

    for (std::size_t index = 0; index < total; ++index) {
        const Profile& profile = profiles[index % profiles.size()];
        scenarys::Run run;
        run.start();

        for (std::size_t phase = 0; phase < scenarys::phaseCount && !run.finished; ++phase) {
            std::bernoulli_distribution mistake(profile.mistakeRate);
            while (mistake(random) && !run.finished) {
                switch (std::uniform_int_distribution<int>(0, 2)(random)) {
                    case 0: run.recordEmpty(); break;
                    case 1: run.recordOutsideClick(); break;
                    default: run.select(scenarys::rowCount, 0); break;  // out of range: recorded as invalid
                }
            }
            if (std::bernoulli_distribution(profile.exitRate)(random)) {
                run.finish("USER EXIT");
                break;
            }
            run.select(pick(profile.rowWeight, random), pick(profile.columnWeight, random));
        }

        printRun(index + 1, profile.id, run);
        std::cout << (index + 1 < total ? ",\n" : "\n");
    }
    std::cout << "  ]\n}\n";
    return 0;
}
