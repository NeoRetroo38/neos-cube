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

// Cubos de usuario: cada fase con su forma; fuera de rango no se mide; límites 1..10.
void testCustomShape() {
    scenarys::Run run({{1, 1}, {10, 10}, {2, 5}});
    run.start();
    assert(run.select(0, 0));
    assert(!run.select(10, 0));
    assert(run.select(9, 9));
    assert(!run.select(0, 5));
    assert(run.select(1, 4));
    assert(run.finished && run.endReason == "COMPLETED");
    const auto all = run.measurements();
    assert(all.size() == 3);
    assert(all[1].row == 10 && all[1].column == 10);
    assert(all[2].row == 2 && all[2].column == 5);

    assert(scenarys::Run().shape == scenarys::defaultShape());
    assert(!scenarys::validShape({}));
    assert(!scenarys::validShape({{0, 3}}));
    assert(!scenarys::validShape({{3, 11}}));
    assert(!scenarys::validShape(std::vector<scenarys::PhaseShape>(11, {1, 1})));
    assert(scenarys::validShape(std::vector<scenarys::PhaseShape>(10, {10, 10})));
    bool threw = false;
    try { scenarys::Run bad({{0, 1}}); } catch (const std::invalid_argument&) { threw = true; }
    assert(threw);
}

bool rejects(const std::vector<scenarys::PhaseShape>& shape, const std::vector<scenarys::Measurement>& a,
             const std::vector<scenarys::Measurement>& b) {
    try { scenarys::compare(shape, a, b); } catch (const std::invalid_argument&) { return true; }
    return false;
}

void testCompare() {
    using M = scenarys::Measurement;
    const auto grid = scenarys::defaultShape();
    const std::vector<M> corner{{1, 1, 1}, {2, 1, 1}, {3, 1, 1}};
    const std::vector<M> opposite{{1, 3, 3}, {2, 3, 3}, {3, 3, 3}};

    // Idénticos → 100 %, esquinas opuestas en todas las fases → 0 %.
    const auto same = scenarys::compare(grid, corner, corner);
    assert(same.tenths == 1000 && same.phases.size() == 3);
    for (const auto& phase : same.phases) assert(phase.tenths == 1000);
    assert(scenarys::compare(grid, corner, opposite).tenths == 0);

    // Centro frente a esquina: √2 / √8 = 0,5 → 50,0 %. Una casilla al lado: 1 − 1/√8 → 64,6 %.
    const std::vector<M> centre{{1, 2, 2}, {2, 2, 2}, {3, 2, 2}};
    assert(scenarys::compare(grid, corner, centre).tenths == 500);
    const std::vector<M> beside{{1, 1, 2}, {2, 1, 1}, {3, 1, 1}};
    const auto near = scenarys::compare(grid, corner, beside);
    assert(near.phases[0].tenths == 646 && near.phases[1].tenths == 1000);
    assert(near.tenths == 882);  // media de 0,3536, 0 y 0 → 1 − 0,1179

    // Simétrica.
    assert(scenarys::compare(grid, beside, corner).tenths == near.tenths);

    // Fases de forma distinta: una 1×1 siempre coincide; una fila 1×5 mide solo por columnas.
    const std::vector<scenarys::PhaseShape> mixed{{3, 3}, {1, 1}, {1, 5}};
    const auto m = scenarys::compare(mixed, {{1, 1, 1}, {2, 1, 1}, {3, 1, 1}}, {{1, 3, 3}, {2, 1, 1}, {3, 1, 3}});
    assert(m.phases[0].tenths == 0 && m.phases[1].tenths == 1000 && m.phases[2].tenths == 500);
    assert(m.tenths == 500);

    // Run incompleto, fase desordenada, fuera de rango o forma inválida → no comparable.
    assert(rejects(grid, corner, {{1, 1, 1}, {2, 1, 1}}));
    assert(rejects(grid, corner, {{2, 1, 1}, {1, 1, 1}, {3, 1, 1}}));
    assert(rejects(grid, corner, {{1, 4, 1}, {2, 1, 1}, {3, 1, 1}}));
    assert(rejects(grid, corner, {{1, 0, 1}, {2, 1, 1}, {3, 1, 1}}));
    assert(rejects({}, {}, {}));
    assert(rejects({{0, 3}}, {{1, 1, 1}}, {{1, 1, 1}}));
}

int main() {
    testCompare();
    testCompletedRun();
    testMeasurements();
    testInvalidInput();
    testEmptyInput();
    testOutsideClick();
    testEarlyExit();
    testSessionIsolation();
    testCustomShape();

    std::cout
        << "ALL NEO CUBE TESTS PASSED\n";

    return 0;
}
