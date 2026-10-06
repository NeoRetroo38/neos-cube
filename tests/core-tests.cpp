#include <cassert>
#include <iostream>

#include "../src/core/cube.hpp"

void testCompletedRun() {
    scenarys::Run run;

    run.start();

    assert(run.select(0, 2));
    assert(run.select(2, 2));
    assert(run.select(1, 0));

    assert(run.finished);
    assert(run.endReason == "COMPLETED");

    assert(run.selectionCount() == 3);
    assert(run.completedSessions() == 3);
    assert(run.totalAttempts() == 3);

    assert(run.sessions[0].visited);
    assert(run.sessions[0].hasSelection);
    assert(run.sessions[0].selectedRow == 0);
    assert(run.sessions[0].selectedColumn == 2);

    assert(run.sessions[1].visited);
    assert(run.sessions[1].hasSelection);
    assert(run.sessions[1].selectedRow == 2);
    assert(run.sessions[1].selectedColumn == 2);

    assert(run.sessions[2].visited);
    assert(run.sessions[2].hasSelection);
    assert(run.sessions[2].selectedRow == 1);
    assert(run.sessions[2].selectedColumn == 0);
}

void testInvalidInput() {
    scenarys::Run run;

    run.start();

    assert(
        !run.select(
            scenarys::rowCount,
            0
        )
    );

    assert(run.currentPhase == 0);

    assert(
        run.sessions[0].invalidInputs == 1
    );

    assert(
        run.sessions[0].attempts == 1
    );

    assert(
        !run.sessions[0].hasSelection
    );

    assert(!run.finished);
}

void testEmptyInput() {
    scenarys::Run run;

    run.start();

    run.recordEmpty();

    assert(run.currentPhase == 0);

    assert(
        run.sessions[0].emptyInputs == 1
    );

    assert(
        run.sessions[0].attempts == 1
    );

    assert(
        !run.sessions[0].hasSelection
    );
}

void testOutsideClick() {
    scenarys::Run run;

    run.start();

    run.recordOutsideClick();

    assert(run.currentPhase == 0);

    assert(
        run.sessions[0].outsideClicks == 1
    );

    assert(
        run.sessions[0].attempts == 1
    );

    assert(
        !run.sessions[0].hasSelection
    );
}

void testEarlyExit() {
    scenarys::Run run;

    run.start();

    assert(run.select(0, 0));

    run.finish("USER EXIT");

    assert(run.finished);

    assert(
        run.endReason == "USER EXIT"
    );

    assert(
        run.selectionCount() == 1
    );

    assert(
        run.completedSessions() == 1
    );

    assert(
        run.sessions[0].hasSelection
    );

    assert(
        run.sessions[1].visited
    );

    assert(
        !run.sessions[1].hasSelection
    );

    assert(
        !run.sessions[2].visited
    );
}

void testSessionIsolation() {
    scenarys::Run run;

    run.start();

    run.recordEmpty();
    run.recordOutsideClick();

    assert(run.select(1, 1));

    assert(
        run.sessions[0].attempts == 3
    );

    assert(
        run.sessions[0].emptyInputs == 1
    );

    assert(
        run.sessions[0].outsideClicks == 1
    );

    assert(
        run.sessions[0].hasSelection
    );

    assert(
        run.sessions[1].attempts == 0
    );

    assert(
        run.sessions[1].visited
    );

    assert(
        !run.sessions[1].hasSelection
    );
}

void testMeasurements() {
    scenarys::Run run;

    run.start();

    assert(run.measurements().empty());

    assert(run.select(0, 2));
    assert(run.select(2, 2));

    // Partial run exposes only stored selections, 1-based and in phase order.
    auto partial = run.measurements();
    assert(partial.size() == 2);
    assert(partial[0].phase == 1 && partial[0].row == 1 && partial[0].column == 3);
    assert(partial[1].phase == 2 && partial[1].row == 3 && partial[1].column == 3);

    assert(run.select(1, 0));

    auto all = run.measurements();
    assert(all.size() == 3);
    assert(all[2].phase == 3 && all[2].row == 2 && all[2].column == 1);

    // Invalid, empty and outside input never create measurements.
    scenarys::Run noisy;
    noisy.start();
    noisy.recordEmpty();
    noisy.recordOutsideClick();
    assert(!noisy.select(scenarys::rowCount, 0));
    assert(noisy.measurements().empty());
}

int main() {
    testCompletedRun();
    testMeasurements();
    testInvalidInput();
    testEmptyInput();
    testOutsideClick();
    testEarlyExit();
    testSessionIsolation();

    std::cout
        << "ALL NEO CUBE TESTS PASSED\n";

    return 0;
}
