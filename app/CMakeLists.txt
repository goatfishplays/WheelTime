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
    src/ActionItems.cpp
    include/App/ActionItems.hpp
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
