#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <windows.h>
#include <windowsx.h>
#include <gdiplus.h>

#include <algorithm>
#include <optional>
#include <string>

#include "../src/core/cube.hpp"

namespace choisys_ui {

using namespace Gdiplus;

constexpr float canvasWidth = 720.0f;
constexpr float canvasHeight = 1280.0f;

const Color ink(255, 0, 0, 0);
const Color paper(255, 255, 255, 255);
const Color green(255, 0, 196, 112);

enum class Screen {
    Home,
    Phase,
    Results
};

enum class ResultDirection {
    Previous,
    Next
};

struct Layout {
    float scale;
    float left;
    float top;

    Layout(int width, int height) {
        scale = std::max(
            0.001f,
            std::min(
                width / canvasWidth,
                height / canvasHeight
            )
        );

        left =
            (width - canvasWidth * scale) / 2.0f;

        top =
            (height - canvasHeight * scale) / 2.0f;
    }

    PointF local(float x, float y) const {
        return PointF(
            (x - left) / scale,
            (y - top) / scale
        );
    }
};

PointF buttonCenter(
    std::size_t row,
    std::size_t column
) {
    return PointF(
        174.0f +
            static_cast<float>(column) *
                186.0f,

        527.0f +
            static_cast<float>(row) *
                197.5f +
            (
                row == 1 &&
                column == 2
                    ? 8.0f
                    : 0.0f
            )
    );
}

bool insideCircle(
    PointF point,
    PointF center,
    float radius
) {
    const float dx =
        point.X - center.X;

    const float dy =
        point.Y - center.Y;

    return
        dx * dx +
        dy * dy <=
        radius * radius;
}

std::optional<std::size_t>
hitButton(PointF point) {
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
            if (
                insideCircle(
                    point,
                    buttonCenter(
                        row,
                        column
                    ),
                    75.0f
                )
            ) {
                return
                    row *
                    scenarys::columnCount +
                    column;
            }
        }
    }

    return std::nullopt;
}

struct App {
    Screen screen = Screen::Home;

    scenarys::Run run;

    std::size_t resultPhase = 0;

    std::optional<std::size_t>
        focusedButton;

    void start() {
        run = scenarys::Run{};

        run.start();

        screen = Screen::Phase;

        resultPhase = 0;

        focusedButton.reset();
    }

    void choose(
        std::size_t index
    ) {
        if (
            screen !=
            Screen::Phase
        ) {
            return;
        }

        run.select(
            index /
                scenarys::columnCount,

            index %
                scenarys::columnCount
        );

        focusedButton.reset();

        if (run.finished) {
            screen =
                Screen::Results;
        }
    }

    void finish() {
        if (
            screen !=
            Screen::Phase
        ) {
            return;
        }

        run.finish(
            "USER EXIT"
        );

        screen =
            Screen::Results;
    }

    void changeResult(
        ResultDirection direction
    ) {
        if (
            direction ==
            ResultDirection::Previous
        ) {
            resultPhase =
                (
                    resultPhase +
                    scenarys::phaseCount -
                    1
                ) %
                scenarys::phaseCount;

        } else {
            resultPhase =
                (
                    resultPhase +
                    1
                ) %
                scenarys::phaseCount;
        }
    }

    void click(
        PointF point
    ) {
        if (
            screen ==
            Screen::Home
        ) {
            if (
                insideCircle(
                    point,
                    PointF(
                        360.0f,
                        716.0f
                    ),
                    75.0f
                )
            ) {
                start();
            }

            return;
        }

        if (
            screen ==
            Screen::Phase
        ) {
            const auto index =
                hitButton(point);

            if (
                index.has_value()
            ) {
                choose(
                    *index
                );

            } else {
                run.recordOutsideClick();
            }

            return;
        }

        if (
            point.Y >= 800 &&
            point.Y <= 950
        ) {
            if (
                point.X >= 180 &&
                point.X <= 270
            ) {
                changeResult(
                    ResultDirection::Previous
                );
            }

            if (
                point.X >= 450 &&
                point.X <= 540
            ) {
                changeResult(
                    ResultDirection::Next
                );
            }
        }
    }
};

void text(
    Graphics& graphics,
    const std::wstring& value,
    float x,
    float y,
    float width,
    float height,
    float size,
    const wchar_t* family,
    Color color,
    bool centered = false
) {
    Font font(
        family,
        size,
        FontStyleRegular,
        UnitPixel
    );

    SolidBrush brush(
        color
    );

    StringFormat format(
        StringFormat::
            GenericTypographic()
    );

    format.SetAlignment(
        centered
            ? StringAlignmentCenter
            : StringAlignmentNear
    );

    format.SetLineAlignment(
        StringAlignmentCenter
    );

    format.SetFormatFlags(
        StringFormatFlagsNoWrap
    );

    graphics.DrawString(
        value.c_str(),
        static_cast<int>(
            value.size()
        ),
        &font,
        RectF(
            x,
            y,
            width,
            height
        ),
        &format,
        &brush
    );
}

std::wstring phaseChoice(
    const scenarys::Session& session
) {
    if (!session.visited) {
        return L"not reached";
    }

    if (!session.hasSelection) {
        return L"no selection";
    }

    return
        L"[" +
        std::to_wstring(
            session.selectedRow + 1
        ) +
        L"," +
        std::to_wstring(
            session.selectedColumn + 1
        ) +
        L"]";
}

void drawResults(
    Graphics& graphics,
    const App& app
) {
    text(
        graphics,
        L"this wasn\u2019t a game",
        0,
        241,
        720,
        48,
        32,
        L"Consolas",
        green,
        true
    );

    text(
        graphics,
        L"it is data",
        0,
        363,
        720,
        48,
        32,
        L"Consolas",
        green,
        true
    );

    text(
        graphics,
        L"apply it to any problem",
        0,
        463,
        720,
        48,
        32,
        L"Consolas",
        green,
        true
    );

    text(
        graphics,
        L"you\u2019re the solution",
        0,
        583,
        720,
        48,
        32,
        L"Consolas",
        green,
        true
    );

    const auto& session =
        app.run.sessions[
            app.resultPhase
        ];

    text(
        graphics,
        L"session " +
            std::to_wstring(
                app.resultPhase + 1
            ) +
            L" / " +
            std::to_wstring(
                scenarys::phaseCount
            ),
        0,
        757,
        720,
        32,
        22,
        L"Consolas",
        green,
        true
    );

    text(
        graphics,
        phaseChoice(
            session
        ),
        0,
        791,
        720,
        28,
        18,
        L"Consolas",
        green,
        true
    );

    Pen outline(
        green,
        2
    );

    SolidBrush selected(
        green
    );

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
            const float x =
                316.0f +
                static_cast<float>(
                    column
                ) *
                    32.0f;

            const float y =
                839.0f +
                static_cast<float>(
                    row
                ) *
                    32.0f;

            if (
                app.run
                    .cube[app.resultPhase]
                         [row]
                         [column]
                    .selected
            ) {
                graphics.FillEllipse(
                    &selected,
                    x,
                    y,
                    24.0f,
                    24.0f
                );

            } else {
                graphics.DrawEllipse(
                    &outline,
                    x,
                    y,
                    24.0f,
                    24.0f
                );
            }
        }
    }

    graphics.DrawLine(
        &outline,
        238,
        863,
        224,
        879
    );

    graphics.DrawLine(
        &outline,
        224,
        879,
        238,
        895
    );

    graphics.DrawLine(
        &outline,
        482,
        863,
        496,
        879
    );

    graphics.DrawLine(
        &outline,
        496,
        879,
        482,
        895
    );

    text(
        graphics,
        L"attempts " +
            std::to_wstring(
                session.attempts
            ) +
            L"   outside clicks " +
            std::to_wstring(
                session.outsideClicks
            ),
        0,
        949,
        720,
        26,
        17,
        L"Consolas",
        green,
        true
    );

    std::wstring route;

    for (
        std::size_t phase = 0;
        phase <
            scenarys::phaseCount;
        ++phase
    ) {
        if (phase > 0) {
            route += L" > ";
        }

        route +=
            L"S" +
            std::to_wstring(
                phase + 1
            ) +
            L":" +
            phaseChoice(
                app.run.sessions[
                    phase
                ]
            );
    }

    text(
        graphics,
        route,
        24,
        1001,
        672,
        28,
        15,
        L"Consolas",
        green,
        true
    );

    text(
        graphics,
        L"run attempts " +
            std::to_wstring(
                app.run.totalAttempts()
            ) +
            L"   sessions " +
            std::to_wstring(
                app.run.completedSessions()
            ),
        0,
        1035,
        720,
        26,
        17,
        L"Consolas",
        green,
        true
    );

    text(
        graphics,
        L"arrows: explore   D: details   R: restart",
        0,
        1091,
        720,
        24,
        15,
        L"Consolas",
        green,
        true
    );

    text(
        graphics,
        L"neo.",
        0,
        1163,
        720,
        46,
        32,
        L"Consolas",
        green,
        true
    );
}

void render(
    Graphics& graphics,
    int width,
    int height,
    const App& app
) {
    graphics.Clear(
        app.screen ==
                Screen::Results
            ? ink
            : paper
    );

    graphics.SetSmoothingMode(
        SmoothingModeAntiAlias
    );

    graphics.SetTextRenderingHint(
        TextRenderingHintAntiAliasGridFit
    );

    const Layout layout(
        width,
        height
    );

    const auto state =
        graphics.Save();

    graphics.TranslateTransform(
        layout.left,
        layout.top
    );

    graphics.ScaleTransform(
        layout.scale,
        layout.scale
    );

    if (
        app.screen ==
        Screen::Home
    ) {
        text(
            graphics,
            L"scenarys",
            0,
            390,
            720,
            80,
            64,
            L"Arial",
            ink,
            true
        );

        SolidBrush black(
            ink
        );

        SolidBrush white(
            paper
        );

        graphics.FillEllipse(
            &black,
            285,
            641,
            150,
            150
        );

        for (
            int index = 0;
            index < 3;
            ++index
        ) {
            graphics.FillEllipse(
                &white,
                351.5f,
                678.5f +
                    index *
                        28.0f,
                17.0f,
                17.0f
            );
        }

    } else if (
        app.screen ==
        Screen::Phase
    ) {
        static const wchar_t*
            titles[] = {
                L"1. fase one.",
                L"2. stage two.",
                L"3. phase three."
            };

        text(
            graphics,
            titles[
                app.run.currentPhase
            ],
            99,
            200,
            620,
            86,
            64,
            L"Arial",
            ink
        );

        SolidBrush black(
            ink
        );

        Pen focus(
            Color(
                255,
                120,
                120,
                120
            ),
            2
        );

        for (
            std::size_t row = 0;
            row <
                scenarys::rowCount;
            ++row
        ) {
            for (
                std::size_t column = 0;
                column <
                    scenarys::columnCount;
                ++column
            ) {
                const auto center =
                    buttonCenter(
                        row,
                        column
                    );

                graphics.FillEllipse(
                    &black,
                    center.X - 75,
                    center.Y - 75,
                    150.0f,
                    150.0f
                );

                const auto index =
                    row *
                        scenarys::columnCount +
                    column;

                if (
                    app.focusedButton
                        .has_value() &&
                    *app.focusedButton ==
                        index
                ) {
                    graphics.DrawEllipse(
                        &focus,
                        center.X - 81,
                        center.Y - 81,
                        162.0f,
                        162.0f
                    );
                }
            }
        }

    } else {
        drawResults(
            graphics,
            app
        );
    }

    graphics.Restore(
        state
    );
}

void showDetails(
    HWND window,
    const App& app
) {
    std::wstring details =
        L"Current run only.\n\n";

    for (
        std::size_t phase = 0;
        phase <
            scenarys::phaseCount;
        ++phase
    ) {
        const auto& session =
            app.run.sessions[
                phase
            ];

        details +=
            L"Session " +
            std::to_wstring(
                phase + 1
            ) +
            L": " +
            phaseChoice(
                session
            ) +
            L"\nAttempts: " +
            std::to_wstring(
                session.attempts
            ) +
            L"  Outside clicks: " +
            std::to_wstring(
                session.outsideClicks
            ) +
            L"\n\n";
    }

    details +=
        L"Recent events:\n";

    const std::size_t first =
        app.run.events.size() >
                12
            ? app.run.events.size() -
                12
            : 0;

    for (
        std::size_t index = first;
        index <
            app.run.events.size();
        ++index
    ) {
        const auto& event =
            app.run.events[
                index
            ];

        details +=
            std::to_wstring(
                index + 1
            ) +
            L". S" +
            std::to_wstring(
                event.phase + 1
            ) +
            L" " +
            std::wstring(
                event.result.begin(),
                event.result.end()
            ) +
            L"\n";
    }

    details +=
        L"\nSession = phase-level data.\n"
        L"Run = total set of sessions.\n"
        L"Data is kept in memory until restart or close.";

    MessageBoxW(
        window,
        details.c_str(),
        L"scenarys | run data",
        MB_OK
    );
}

LRESULT CALLBACK
windowProcedure(
    HWND window,
    UINT message,
    WPARAM key,
    LPARAM coordinates
) {
    auto* app =
        reinterpret_cast<App*>(
            GetWindowLongPtrW(
                window,
                GWLP_USERDATA
            )
        );

    if (
        message ==
        WM_NCCREATE
    ) {
        const auto* creation =
            reinterpret_cast<
                CREATESTRUCTW*
            >(coordinates);

        app =
            static_cast<App*>(
                creation->
                    lpCreateParams
            );

        SetWindowLongPtrW(
            window,
            GWLP_USERDATA,
            reinterpret_cast<
                LONG_PTR
            >(app)
        );
    }

    if (!app) {
        return DefWindowProcW(
            window,
            message,
            key,
            coordinates
        );
    }

    switch (message) {

    case WM_ERASEBKGND:
        return 1;

    case WM_SIZE:
        InvalidateRect(
            window,
            nullptr,
            FALSE
        );

        return 0;

    case WM_PAINT: {
        PAINTSTRUCT paint;

        HDC context =
            BeginPaint(
                window,
                &paint
            );

        RECT bounds;

        GetClientRect(
            window,
            &bounds
        );

        const int width =
            bounds.right;

        const int height =
            bounds.bottom;

        if (
            width > 0 &&
            height > 0
        ) {
            Bitmap buffer(
                width,
                height,
                PixelFormat32bppARGB
            );

            Graphics offscreen(
                &buffer
            );

            render(
                offscreen,
                width,
                height,
                *app
            );

            Graphics onscreen(
                context
            );

            onscreen.DrawImage(
                &buffer,
                0,
                0
            );
        }

        EndPaint(
            window,
            &paint
        );

        return 0;
    }

    case WM_LBUTTONUP: {
        RECT bounds;

        GetClientRect(
            window,
            &bounds
        );

        const Layout layout(
            bounds.right,
            bounds.bottom
        );

        app->click(
            layout.local(
                static_cast<float>(
                    GET_X_LPARAM(
                        coordinates
                    )
                ),
                static_cast<float>(
                    GET_Y_LPARAM(
                        coordinates
                    )
                )
            )
        );

        InvalidateRect(
            window,
            nullptr,
            FALSE
        );

        return 0;
    }

    case WM_MOUSEWHEEL:
        if (
            app->screen ==
            Screen::Results
        ) {
            if (
                GET_WHEEL_DELTA_WPARAM(
                    key
                ) > 0
            ) {
                app->changeResult(
                    ResultDirection::
                        Previous
                );
            } else {
                app->changeResult(
                    ResultDirection::
                        Next
                );
            }

            InvalidateRect(
                window,
                nullptr,
                FALSE
            );
        }

        return 0;

    case WM_KEYDOWN: {
        if (
            (coordinates &
             (1LL << 30)) != 0
        ) {
            return 0;
        }

        if (
            app->screen ==
            Screen::Home
        ) {
            if (
                key ==
                    VK_RETURN ||
                key ==
                    VK_SPACE
            ) {
                app->start();
            }

        } else if (
            app->screen ==
            Screen::Phase
        ) {
            constexpr std::size_t
                buttonCount =
                    scenarys::rowCount *
                    scenarys::columnCount;

            if (
                key ==
                VK_ESCAPE
            ) {
                app->finish();

            } else if (
                key ==
                    VK_TAB ||
                key ==
                    VK_RIGHT ||
                key ==
                    VK_DOWN
            ) {
                if (
                    app->
                        focusedButton
                        .has_value()
                ) {
                    app->
                        focusedButton =
                            (
                                *app->
                                    focusedButton +
                                1
                            ) %
                            buttonCount;
                } else {
                    app->
                        focusedButton =
                            0;
                }

            } else if (
                key ==
                    VK_LEFT ||
                key ==
                    VK_UP
            ) {
                if (
                    app->
                        focusedButton
                        .has_value()
                ) {
                    app->
                        focusedButton =
                            (
                                *app->
                                    focusedButton +
                                buttonCount -
                                1
                            ) %
                            buttonCount;
                } else {
                    app->
                        focusedButton =
                            buttonCount -
                            1;
                }

            } else if (
                (
                    key ==
                        VK_RETURN ||
                    key ==
                        VK_SPACE
                ) &&
                app->
                    focusedButton
                    .has_value()
            ) {
                app->choose(
                    *app->
                        focusedButton
                );
            }

        } else {
            if (
                key ==
                VK_LEFT
            ) {
                app->changeResult(
                    ResultDirection::
                        Previous
                );
            }

            if (
                key ==
                VK_RIGHT
            ) {
                app->changeResult(
                    ResultDirection::
                        Next
                );
            }

            if (
                key == 'D'
            ) {
                showDetails(
                    window,
                    *app
                );
            }

            if (
                key == 'R'
            ) {
                app->screen =
                    Screen::Home;

                app->run =
                    scenarys::Run{};

                app->
                    focusedButton
                    .reset();
            }
        }

        InvalidateRect(
            window,
            nullptr,
            FALSE
        );

        return 0;
    }

    case WM_CLOSE:
        if (
            app->screen ==
            Screen::Phase
        ) {
            app->finish();

            InvalidateRect(
                window,
                nullptr,
                FALSE
            );

        } else {
            DestroyWindow(
                window
            );
        }

        return 0;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;

    default:
        return DefWindowProcW(
            window,
            message,
            key,
            coordinates
        );
    }
}

} // namespace choisys_ui

#ifndef SCENARYS_TEST

int WINAPI WinMain(
    HINSTANCE instance,
    HINSTANCE,
    LPSTR,
    int showCommand
) {
    SetProcessDPIAware();

    Gdiplus::
        GdiplusStartupInput
            startupInput;

    ULONG_PTR token = 0;

    if (
        Gdiplus::
            GdiplusStartup(
                &token,
                &startupInput,
                nullptr
            ) !=
        Gdiplus::Ok
    ) {
        return 1;
    }

    const wchar_t*
        className =
            L"ScenarysCubeWindow";

    WNDCLASSW
        windowClass{};

    windowClass.lpfnWndProc =
        choisys_ui::
            windowProcedure;

    windowClass.hInstance =
        instance;

    windowClass.lpszClassName =
        className;

    windowClass.hCursor =
        LoadCursor(
            nullptr,
            IDC_ARROW
        );

    if (
        !RegisterClassW(
            &windowClass
        )
    ) {
        Gdiplus::
            GdiplusShutdown(
                token
            );

        return 1;
    }

    RECT workArea;

    SystemParametersInfoW(
        SPI_GETWORKAREA,
        0,
        &workArea,
        0
    );

    const float scale =
        std::min(
            0.75f,
            std::min(
                (
                    workArea.bottom -
                    workArea.top -
                    80
                ) /
                    1280.0f,
                (
                    workArea.right -
                    workArea.left -
                    80
                ) /
                    720.0f
            )
        );

    RECT bounds{
        0,
        0,
        static_cast<LONG>(
            720 * scale
        ),
        static_cast<LONG>(
            1280 * scale
        )
    };

    AdjustWindowRectEx(
        &bounds,
        WS_OVERLAPPEDWINDOW,
        FALSE,
        0
    );

    choisys_ui::App app;

    const int width =
        bounds.right -
        bounds.left;

    const int height =
        bounds.bottom -
        bounds.top;

    HWND window =
        CreateWindowExW(
            0,
            className,
            L"scenarys",
            WS_OVERLAPPEDWINDOW,

            workArea.left +
                (
                    workArea.right -
                    workArea.left -
                    width
                ) /
                    2,

            workArea.top +
                (
                    workArea.bottom -
                    workArea.top -
                    height
                ) /
                    2,

            width,
            height,

            nullptr,
            nullptr,
            instance,
            &app
        );

    if (!window) {
        Gdiplus::
            GdiplusShutdown(
                token
            );

        return 1;
    }

    ShowWindow(
        window,
        showCommand
    );

    MSG message{};

    int status = 0;

    while (
        (
            status =
                GetMessageW(
                    &message,
                    nullptr,
                    0,
                    0
                )
        ) > 0
    ) {
        TranslateMessage(
            &message
        );

        DispatchMessageW(
            &message
        );
    }

    Gdiplus::
        GdiplusShutdown(
            token
        );

    if (status < 0) {
        return 1;
    }

    return 0;
}

#endif
