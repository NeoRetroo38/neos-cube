#include <iostream>
#include <sstream>
#include <string>

#include "../src/core/cube.hpp"

void showSummary(const scenarys::Run& run) {
    std::size_t emptyInputs = 0;
    std::size_t invalidInputs = 0;
    std::size_t outsideClicks = 0;

    for (const auto& session : run.sessions) {
        emptyInputs += session.emptyInputs;
        invalidInputs += session.invalidInputs;
        outsideClicks += session.outsideClicks;
    }

    std::cout
        << "\n========== RUN RESULTS ==========\n"
        << "End reason: " << run.endReason << '\n'
        << "Input attempts: " << run.totalAttempts() << '\n'
        << "Selections: " << run.selectionCount() << '\n'
        << "Completed sessions: " << run.completedSessions() << '\n'
        << "Empty submissions: " << emptyInputs << '\n'
        << "Invalid submissions: " << invalidInputs << '\n'
        << "Outside clicks: " << outsideClicks << '\n'
        << "\nROUTE\n";

    for (
        std::size_t phase = 0;
        phase < scenarys::phaseCount;
        ++phase
    ) {
        const auto& session = run.sessions[phase];

        if (phase > 0) {
            std::cout << " -> ";
        }

        std::cout << "P" << phase + 1 << ':';

        if (session.hasSelection) {
            std::cout
                << '['
                << session.selectedRow + 1
                << ','
                << session.selectedColumn + 1
                << ']';
        } else {
            std::cout
                << (
                    session.visited
                        ? "NO SELECTION"
                        : "NOT REACHED"
                );
        }
    }

    std::cout << "\n\nSESSION DETAILS\n";

    for (
        std::size_t phase = 0;
        phase < scenarys::phaseCount;
        ++phase
    ) {
        const auto& session = run.sessions[phase];

        std::cout
            << "Session " << phase + 1
            << " | Attempts: " << session.attempts
            << " | Empty: " << session.emptyInputs
            << " | Invalid: " << session.invalidInputs
            << " | Outside: " << session.outsideClicks
            << '\n';
    }

    std::cout << "\nEVENT HISTORY\n";

    for (
        std::size_t index = 0;
        index < run.events.size();
        ++index
    ) {
        const auto& event = run.events[index];

        std::cout
            << index + 1
            << ". Phase "
            << event.phase + 1
            << " | "
            << event.result;

        if (!event.input.empty()) {
            std::cout
                << " | Input: "
                << event.input;
        }

        std::cout << '\n';
    }
}

int main() {
    scenarys::Run run;

    run.start();

    while (!run.finished) {
        std::cout
            << "\nPhase "
            << run.currentPhase + 1
            << " / "
            << scenarys::phaseCount
            << '\n';

        for (
            std::size_t row = 0;
            row < scenarys::rowCount;
            ++row
        ) {
            for (
                std::size_t column = 0;
                column < scenarys::columnCount;
                ++column
            ) {
                std::cout
                    << '['
                    << row + 1
                    << ','
                    << column + 1
                    << "] ";
            }

            std::cout << '\n';
        }

        std::cout
            << "Enter row and column "
            << "(example: 2 3), or q to end: ";

        std::string input;

        if (!std::getline(std::cin, input)) {
            run.finish(
                "INPUT ENDED",
                false
            );

            break;
        }

        if (input == "q") {
            run.finish("USER EXIT");
            break;
        }

        if (
            input.find_first_not_of(
                " \t\r"
            ) == std::string::npos
        ) {
            run.recordEmpty();

            std::cout
                << "No selection recorded. "
                << "Phase unchanged.\n";

            continue;
        }

        std::istringstream parser(input);

        std::size_t row = 0;
        std::size_t column = 0;
        std::string extra;

        if (
            !(parser >> row >> column) ||
            (parser >> extra) ||
            row < 1 ||
            row > scenarys::rowCount ||
            column < 1 ||
            column > scenarys::columnCount
        ) {
            run.recordInvalid(input);

            std::cout
                << "Invalid position. Try again.\n";

            continue;
        }

        run.select(
            row - 1,
            column - 1
        );
    }

    showSummary(run);

    std::cout
        << "\nPress Enter to close.";

    std::string closingInput;

    std::getline(
        std::cin,
        closingInput
    );

    return 0;
}
