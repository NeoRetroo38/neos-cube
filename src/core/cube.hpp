#pragma once

#include <array>
#include <cstddef>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace scenarys {

constexpr std::size_t phaseCount = 3;
constexpr std::size_t rowCount = 3;
constexpr std::size_t columnCount = 3;

static_assert(phaseCount > 0);
static_assert(rowCount > 0);
static_assert(columnCount > 0);

// Cubos de usuario: cada fase con su propio alto × ancho, todo entre 1 y maxDimension.
constexpr std::size_t maxDimension = 10;

struct PhaseShape {
    std::size_t rows;
    std::size_t columns;
    bool operator==(const PhaseShape& other) const { return rows == other.rows && columns == other.columns; }
};

inline std::vector<PhaseShape> defaultShape() {
    return std::vector<PhaseShape>(phaseCount, PhaseShape{rowCount, columnCount});
}

inline bool validShape(const std::vector<PhaseShape>& shape) {
    if (shape.empty() || shape.size() > maxDimension) return false;
    for (const auto& phase : shape) {
        if (phase.rows < 1 || phase.rows > maxDimension || phase.columns < 1 || phase.columns > maxDimension) return false;
    }
    return true;
}

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

using Row = std::vector<Button>;
using Phase = std::vector<Row>;
using Cube = std::vector<Phase>;

class Run {
public:
    std::vector<PhaseShape> shape;

    Cube cube;

    std::vector<Session> sessions;

    std::vector<InputEvent> events;

    std::size_t currentPhase = 0;

    bool finished = false;

    std::string endReason;

    Run() : Run(defaultShape()) {}

    explicit Run(std::vector<PhaseShape> phases) : shape(std::move(phases)) {
        if (!validShape(shape)) throw std::invalid_argument("invalid cube shape");
        sessions.resize(shape.size());
        for (const auto& phase : shape) cube.emplace_back(phase.rows, Row(phase.columns));
    }

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
            row >= shape[currentPhase].rows ||
            column >= shape[currentPhase].columns
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

        if (currentPhase == shape.size()) {
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

        for (std::size_t phase = 0; phase < sessions.size(); ++phase) {
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
            currentPhase < sessions.size() &&
            sessions[currentPhase].visited;
    }
};

} // namespace scenarys