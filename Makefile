# POSIX build (macOS, Linux). Windows uses build.ps1.
#   make test      build and run the core tests
#   make service   build build/neo-cube-service
#   make smoke     run the service and exercise it end to end (needs curl, openssl)
CXX ?= c++
CXXFLAGS ?= -std=c++17 -Wall -Wextra -Wpedantic -O2
BUILD := build

.PHONY: all tests service test smoke cases clean

all: tests service

$(BUILD):
	mkdir -p $(BUILD)

tests: $(BUILD)/neo-cube-tests
service: $(BUILD)/neo-cube-service

$(BUILD)/neo-cube-tests: tests/core-tests.cpp src/core/cube.hpp | $(BUILD)
	$(CXX) $(CXXFLAGS) tests/core-tests.cpp -o $@

$(BUILD)/neo-cube-service: src/api/server.cpp src/core/cube.hpp | $(BUILD)
	$(CXX) $(CXXFLAGS) src/api/server.cpp -o $@

test: $(BUILD)/neo-cube-tests
	./$(BUILD)/neo-cube-tests

# Simulated populations for the choisys landing (docs/SIMULATED-CASES.md); same seed, same bytes.
cases: $(BUILD)/simulate-cases
	./$(BUILD)/simulate-cases > $(BUILD)/cases.json
	./$(BUILD)/simulate-cases | cmp - $(BUILD)/cases.json
	@echo "build/cases.json (deterministic)"

$(BUILD)/simulate-cases: examples/simulate_cases.cpp src/core/cube.hpp | $(BUILD)
	$(CXX) $(CXXFLAGS) examples/simulate_cases.cpp -o $@

smoke: $(BUILD)/neo-cube-service
	sh tests/smoke-posix.sh

clean:
	rm -rf $(BUILD)
