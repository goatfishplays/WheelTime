add_library(App 
    src/App.cpp
    include/App/App.hpp
    src/HoverButton.cpp
    include/App/HoverButton.hpp
    src/RadialMenuWidget.cpp
    include/App/RadialMenuWidget.hpp
    src/Gui.cpp
    include/App/Gui.hpp
    src/Menu.cpp
    include/App/Menu.hpp
    src/Action.cpp
    include/App/Action.hpp
    src/ActionItems/ActionItem.cpp
    src/ActionItems/Delay.cpp
    src/ActionItems/Script.cpp
    src/ActionItems/LaunchApp.cpp
    src/ActionItems/Keystroke.cpp
    src/ActionItems/Menu.cpp
    src/ActionItems/Close.cpp
    src/ActionItems/Socket.cpp
    src/ActionItems/NthRecent.cpp
    src/ActionItems/NthFrequent.cpp
    src/ActionItems/MouseMove.cpp
    src/ActionItems/MouseButton.cpp
    src/ActionItems/Cancel.cpp
    src/ActionHistory.cpp
    include/App/ActionItems.hpp
    include/App/ActionItems/ActionItem.hpp
    include/App/ActionItems/Delay.hpp
    include/App/ActionItems/Script.hpp
    include/App/ActionItems/LaunchApp.hpp
    include/App/ActionItems/Keystroke.hpp
    include/App/ActionItems/Menu.hpp
    include/App/ActionItems/Close.hpp
    include/App/ActionItems/Socket.hpp
    include/App/ActionItems/NthRecent.hpp
    include/App/ActionItems/NthFrequent.hpp
    include/App/ActionItems/MouseMove.hpp
    include/App/ActionItems/MouseButton.hpp
    include/App/ActionItems/Cancel.hpp
    include/App/ActionHistory.hpp
    src/ActionExecutionContext.cpp
    include/App/ActionExecutionContext.hpp
    src/ExecuteResult.cpp
    include/App/ExecuteResult.hpp
    src/SynchronousActionRunner.cpp
    include/App/SynchronousActionRunner.hpp
    include/App/ThreadSafeQueue.hpp
    src/WorkerPool.cpp
    include/App/WorkerPool.hpp
    src/Scheduler.cpp
    include/App/Scheduler.hpp
    src/DelayQueue.cpp
    include/App/DelayQueue.hpp
    src/ChannelManager.cpp
    include/App/ChannelManager.hpp
    src/MenuConfigLoader.cpp
    include/App/MenuConfigLoader.hpp
    src/SettingsWindow.cpp
    include/App/SettingsWindow.hpp
)

target_include_directories(App 
    PUBLIC 
        include
)

target_link_libraries(App
    PUBLIC 
        Platform
        Qt6::Widgets
        Qt6::Core
        Qt6::Gui
)

# Disposable Phase 2 smoke test (header-only queue; no Qt / App runtime).
add_executable(phase2_queue_smoke
    tests/phase2_queue_smoke.cpp
)
target_include_directories(phase2_queue_smoke
    PRIVATE
        include
)
target_compile_features(phase2_queue_smoke
    PRIVATE
        cxx_std_20
)

# Disposable Phase 3 smoke test (needs App for Action / context / WorkerPool).
add_executable(phase3_worker_smoke
    tests/phase3_worker_smoke.cpp
)
target_link_libraries(phase3_worker_smoke
    PRIVATE
        App
)
target_compile_features(phase3_worker_smoke
    PRIVATE
        cxx_std_20
)

# Disposable Phase 4 smoke test.
add_executable(phase4_scheduler_smoke
    tests/phase4_scheduler_smoke.cpp
)
target_link_libraries(phase4_scheduler_smoke
    PRIVATE
        App
)
target_compile_features(phase4_scheduler_smoke
    PRIVATE
        cxx_std_20
)

# Disposable Phase 5 smoke test.
add_executable(phase5_delay_queue_smoke
    tests/phase5_delay_queue_smoke.cpp
)
target_link_libraries(phase5_delay_queue_smoke
    PRIVATE
        App
)
target_compile_features(phase5_delay_queue_smoke
    PRIVATE
        cxx_std_20
)

# Disposable Phase 6 smoke test.
add_executable(phase6_channel_smoke
    tests/phase6_channel_smoke.cpp
)
target_link_libraries(phase6_channel_smoke
    PRIVATE
        App
)
target_compile_features(phase6_channel_smoke
    PRIVATE
        cxx_std_20
)

# Disposable Phase 7 smoke test.
add_executable(phase7_cancel_smoke
    tests/phase7_cancel_smoke.cpp
)
target_link_libraries(phase7_cancel_smoke
    PRIVATE
        App
)
target_compile_features(phase7_cancel_smoke
    PRIVATE
        cxx_std_20
)

# Disposable Phase 8 smoke test.
add_executable(phase8_pause_smoke
    tests/phase8_pause_smoke.cpp
)
target_link_libraries(phase8_pause_smoke
    PRIVATE
        App
)
target_compile_features(phase8_pause_smoke
    PRIVATE
        cxx_std_20
)

# Disposable Phase 9 smoke test.
add_executable(phase9_actionitems_smoke
    tests/phase9_actionitems_smoke.cpp
)
target_link_libraries(phase9_actionitems_smoke
    PRIVATE
        App
)
target_compile_features(phase9_actionitems_smoke
    PRIVATE
        cxx_std_20
)

# Phase 10 comprehensive scheduler tests.
add_executable(phase10_scheduler_tests
    tests/phase10_scheduler_tests.cpp
)
target_link_libraries(phase10_scheduler_tests
    PRIVATE
        App
)
target_compile_features(phase10_scheduler_tests
    PRIVATE
        cxx_std_20
)

# ActionHistory smoke test.
add_executable(action_history_smoke
    tests/action_history_smoke.cpp
)
target_link_libraries(action_history_smoke
    PRIVATE
        App
)
target_compile_features(action_history_smoke
    PRIVATE
        cxx_std_20
)
