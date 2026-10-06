#pragma once

#include <array>
#include <cstddef>
#include <string>
#include <vector>

namespace scenarys {

constexpr std::size_t phaseCount = 3;
constexpr std::size_t rowCount = 3;
constexpr std::size_t columnCount = 3;

static_assert(phaseCount > 0);
static_assert(rowCount > 0);
static_assert(columnCount > 0);

struct Button {
    bool selected = false;
};

struct InputEvent {
    std::size_t phase = 0;
    std::string input;
    std::string result;
};

struct Session {
    bool visited = false;
    bool hasSelection = false;

    std::size_t attempts = 0;
    std::size_t emptyInputs = 0;
    std::size_t invalidInputs = 0;
    std::size_t outsideClicks = 0;

    std::size_t selectedRow = 0;
    std::size_t selectedColumn = 0;
};

// Public geometric observation: one stored selection, 1-based (phase, row, column).
struct Measurement {
    std::size_t phase;
    std::size_t row;
    std::size_t column;
};

using Row = std::array<Button, columnCount>;
using Phase = std::array<Row, rowCount>;
using Cube = std::array<Phase, phaseCount>;

class Run {
public:
    Cube cube{};

    std::array<Session, phaseCount> sessions{};

    std::vector<InputEvent> events;

    std::size_t currentPhase = 0;

    bool finished = false;

    std::string endReason;

    void start() {
        if (!finished && currentPhase == 0) {
            sessions[0].visited = true;
        }
    }

    bool select(
        std::size_t row,
        std::size_t column
    ) {
        if (!isActive()) {
            return false;
        }

        if (
            row >= rowCount ||
            column >= columnCount
        ) {
            recordInvalid(
                "Coordinates outside cube: " +
                std::to_string(row) +
                " " +
                std::to_string(column)
            );

            return false;
        }

        auto& session =
            sessions[currentPhase];

        ++session.attempts;

        session.hasSelection = true;
        session.selectedRow = row;
        session.selectedColumn = column;

        cube[currentPhase]
            [row]
            [column]
            .selected = true;

        events.push_back({
            currentPhase,
            std::to_string(row + 1) +
                " " +
                std::to_string(column + 1),
            "SELECTED"
        });

        ++currentPhase;

        if (currentPhase == phaseCount) {
            finished = true;
            endReason = "COMPLETED";
        } else {
            sessions[currentPhase].visited = true;
        }

        return true;
    }

    void recordEmpty() {
        if (!isActive()) {
            return;
        }

        auto& session =
            sessions[currentPhase];

        ++session.attempts;
        ++session.emptyInputs;

        events.push_back({
            currentPhase,
            "",
            "EMPTY INPUT - NO SELECTION"
        });
    }

    void recordInvalid(
        const std::string& input
    ) {
        if (!isActive()) {
            return;
        }

        auto& session =
            sessions[currentPhase];

        ++session.attempts;
        ++session.invalidInputs;

        events.push_back({
            currentPhase,
            input,
            "INVALID INPUT"
        });
    }

    void recordOutsideClick() {
        if (!isActive()) {
            return;
        }

        auto& session =
            sessions[currentPhase];

        ++session.attempts;
        ++session.outsideClicks;

        events.push_back({
            currentPhase,
            "",
            "OUTSIDE CLICK - NO SELECTION"
        });
    }

    void finish(
        const std::string& reason,
        bool countAttempt = true
    ) {
        if (!isActive()) {
            return;
        }

        if (countAttempt) {
            ++sessions[currentPhase].attempts;
        }

        const std::string result =
            reason == "USER EXIT"
                ? "EXIT WITHOUT SELECTION"
                : reason + " WITHOUT SELECTION";

        events.push_back({
            currentPhase,
            "",
            result
        });

        endReason = reason;
        finished = true;
    }

    std::size_t totalAttempts() const {
        std::size_t total = 0;

        for (const auto& session : sessions) {
            total += session.attempts;
        }

        return total;
    }

    std::size_t selectionCount() const {
        std::size_t total = 0;

        for (const auto& session : sessions) {
            if (session.hasSelection) {
                ++total;
            }
        }

        return total;
    }

    // Selections already stored by the sessions, in phase order; no derived values.
    std::vector<Measurement> measurements() const {
        std::vector<Measurement> result;

        for (std::size_t phase = 0; phase < phaseCount; ++phase) {
            const auto& session = sessions[phase];

            if (session.visited && session.hasSelection) {
                result.push_back({
                    phase + 1,
                    session.selectedRow + 1,
                    session.selectedColumn + 1
                });
            }
        }

        return result;
    }

    std::size_t completedSessions() const {
        std::size_t total = 0;

        for (const auto& session : sessions) {
            if (
                session.visited &&
                session.hasSelection
            ) {
                ++total;
            }
        }

        return total;
    }

private:
    bool isActive() const {
        return
            !finished &&
            currentPhase < phaseCount &&
            sessions[currentPhase].visited;
    }
};

} // namespace scenarys