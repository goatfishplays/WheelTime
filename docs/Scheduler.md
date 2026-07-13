# Scheduler Design

## Overview

This library implements a multithreaded action scheduler for a desktop automation application.

The scheduler is responsible for executing Actions while maintaining ordering guarantees, delays, cancellation, and pause/resume functionality.

The implementation should prioritize:

- Clean ownership
- Correctness
- Modern C++20
- RAII
- Separation of responsibilities
- Extensibility
- Low lock contention
- High worker utilization

This project is being implemented incrementally. Do not implement future phases until requested.

---

# High-Level Architecture

There are four major components.

## Main Thread

Responsible for:

- Submitting Actions
- Requesting cancellation
- Requesting pause/resume

The main thread never executes ActionItems.

---

## Scheduler Thread

Responsible for:

- Owning every submitted Action
- Managing channels
- Managing delayed Actions
- Dispatching work to workers
- Processing ExecuteResults
- Processing scheduler requests
- Handling pause/resume
- Handling cancellation

The scheduler is the only component that makes scheduling decisions.

---

## Worker Threads

Responsible only for executing ActionItems.

Workers:

- Never sleep waiting for delays
- Never make scheduling decisions
- Never own Actions
- Return execution state back to the scheduler whenever scheduling is required
  (finish, DelayUntil, cancel between items, or pending scheduleAction /
  setCancelFlush side effects)

---

## ActionItems

ActionItems perform actual work.

Examples:

- Delay
- Script execution
- Keyboard input
- Mouse input
- Launch process

ActionItems should be small and focused.

Long-running work should generally be broken into multiple ActionItems.

---

# Actions

An Action contains:

- channel id
- sequence of ActionItems
- cancelable flag (default true)

Actions describe work.

They contain no execution state.

An Action may also carry library/UI metadata (name, id, icon, editor helpers).
That metadata is unrelated to scheduling; the scheduler only needs channel,
items, and cancelable.

---

# ActionExecutionContext

Workers execute ActionExecutionContexts rather than Actions directly.

The context owns:

- Action
- Current ActionItem index
- Runtime action id
- Shared cancel flag
- Scheduler requests generated during execution (`scheduleAction`, `setCancelFlush`)

The context represents the current execution state.

---

# Channels

Channels serialize execution.

Rules:

- Multiple Actions may exist in the same channel.
- Channels are strict FIFO.
- Only one Action from a channel may be in-flight at a time.
- Different channels may execute simultaneously.
- A delayed Action continues to hold its channel until it wakes
  (queued Actions behind it do not run early).
- Waiting Actions preserve their earliest start time so nested future
  schedules do not lose their wake time while the channel is busy.

Workers continue executing ActionItems until:

- the Action finishes

or

- an ActionItem returns DelayUntil(...)

or

- an ActionItem records pending scheduler requests (`scheduleAction` /
  `setCancelFlush`), in which case the worker yields so the scheduler can
  ingest them before more items run

---

# Channel 0

Channel 0 is special.

Every Action submitted to channel 0 behaves as though it owns its own private channel.

Actions submitted to channel 0 never block one another.

Channel 0 still flows through the scheduler.

Channel 0 is not automatically uncancelable; cancelability is per Action.

---

# ActionItem API

Every ActionItem implements:

```cpp
ExecuteResult execute(ActionExecutionContext&);
```

ActionItems never interact directly with the scheduler.

Instead they record scheduler requests inside the ActionExecutionContext.

Examples:

```cpp
context.scheduleAction(action, wakeTime);
context.scheduleAction(action, wakeTime, /*removeIfParentCancelled=*/true);
context.setCancelFlush(cleanupAction);
context.requestCancelMostRecent(channel);
context.requestCancelChannel(channel);
context.requestCancelAll();
```

The scheduler consumes those requests after execute() returns (or when the
worker yields Continue because requests are pending).

---

# ExecuteResult

ExecuteResult only describes the fate of the CURRENT Action.

Current operations:

- Continue
- DelayUntil(time_point)

Future operations may be added if necessary.

ExecuteResult should remain small.

WorkerResult (worker → scheduler) is separate and may also report Completed,
Continue (pending side effects), Delayed, or Cancelled.

---

# Delays

Workers never wait.

If an Action delays:

- Worker returns ActionExecutionContext to scheduler.
- Scheduler stores it in a DelayQueue (priority queue ordered by wake time).
- The Action's channel remains busy until wake.
- Scheduler resumes it later.

---

# Cancellation

Cancellation exists at three levels.

`submit` returns a runtime action id used with `cancelAction`.

ActionItems are atomic. Cancellation only occurs between ActionItems.
The current ActionItem always runs to completion once started.

## Cancel Action

Queued:

- remove immediately

Delayed:

- discard (channel freed)

Executing:

- finish current ActionItem
- terminate Action afterward

Also:

- run any registered cancel-flush Actions immediately
- discard linked delayed children scheduled with `removeIfParentCancelled`

## Cancel Channel

For every cancelable Action whose logical channel matches:

- finish current ActionItem
- terminate current Action
- remove queued Actions
- remove delayed Actions
- run cancel-flush cleanups as above

Uncancelable Actions on that channel are left alone.
Channel 0 cancels all logical channel-0 Actions (each still has a private key).

## Cancel All

Equivalent to cancelling every cancelable Action.

Uncancelable Actions are skipped.

## Uncancelable Actions

Actions may opt out of cancellation via `Action::setCancelable(false)`.

Rules:

- `cancelAction` / `cancelChannel` / `cancelAll` skip uncancelable Actions
- Uncancelable delayed work stays on the DelayQueue, keeps its channel, and runs normally
- Intended for cleanup work that must not be stranded (e.g. delayed KeyUp)

## Cancel Flush

An ActionItem may register cleanup that must run immediately when its owning
Action is cancelled:

```cpp
context.setCancelFlush(cleanupAction);
```

Typical KeyDown / KeyUp pattern (wired later in built-in items):

1. `keyDown`
2. `setCancelFlush(KeyUp)` so cancel releases the key now
3. `scheduleAction([Delay, KeyUp], ..., removeIfParentCancelled=true)` with
   `cancelable=false` as a safety net if cancel never happens

On cancel of the parent:

- flush Actions are submitted immediately (forced uncancelable)
- linked delayed cleanup is discarded so KeyUp is not sent twice

---

# Pause

The scheduler supports pause/resume.

While paused:

- scheduler dispatches no new work
- delayed Actions stop progressing (wake times shift forward on resume so
  remaining delay is preserved)
- workers finish their current ActionItem
- workers then sleep until resume
- submits are still admitted but do not start until resume
- cancel still works; cancel-flush cleanups bypass pause and run immediately

Future UI work may allow automation to continue while editing, but scheduler pause support should remain.

---

# Ownership

Scheduler owns every submitted Action.

Workers operate only on ActionExecutionContexts.

Workers never own Actions.

---

# Design Principles

Each class should have one responsibility.

Action

- describes work

ActionExecutionContext

- tracks execution state (index, cancel flag, pending requests)

ActionItem

- performs work

ExecuteResult

- tells scheduler whether the current Action should continue or delay

Scheduler

- scheduling decisions only (channels, delays, cancel, dispatch)

WorkerPool

- execution only

DelayQueue / ChannelManager

- scheduler-thread-only helpers extracted for delay and FIFO policy

---

# Implementation Roadmap

The scheduler should be implemented incrementally.

Each phase should compile and be tested before moving on.

---

## Phase 1 - Core Types — Done

No threading.

Implement:

- Action (channel + items; library/UI fields allowed)
- ActionItem (`execute` returns ExecuteResult)
- ExecuteResult (Continue / DelayUntil)
- ActionExecutionContext (ownership, index, scheduleAction, cancel flag)

Goal:

Define the core execution model and public APIs.

---

## Phase 2 - Thread-Safe Queue — Done

Implement a reusable MPMC thread-safe queue.

Features:

- push
- pop
- clear
- stop
- condition variable

This queue is reused by the WorkerPool and Scheduler mailbox.

---

## Phase 3 - WorkerPool — Done

Implement the worker thread pool.

Responsibilities:

- Create N worker threads
- Wait for ActionExecutionContexts
- Execute ActionItems
- Return WorkerResults to the scheduler

Workers contain no scheduling logic. They yield Continue when an item
records pending `scheduleAction` / `setCancelFlush` so the scheduler can
ingest side effects promptly.

---

## Phase 4 - Scheduler — Done

Implement the scheduler thread.

Responsibilities:

- Accept submitted Actions (`submit` returns runtime action id)
- Own ActionExecutionContexts
- Dispatch work to workers
- Process WorkerResults
- Process nested scheduleAction requests
- Manage channel ownership (including hold-through-delay)

No cancellation or pause yet in this phase.

Goal:

Basic end-to-end scheduling functions.

---

## Phase 5 - Delay Queue — Done

Extract DelayQueue and formalize delayed execution.

Requirements:

- Priority queue ordered by wake time (stable FIFO for equal wakes)
- Scheduler sleeps until:
    - new Action
    - earliest wake time
    - stop request
- Delayed Actions hold their channel until wake

Workers never wait for delays.

---

## Phase 6 - Channel Management — Done

Extract ChannelManager and harden strict FIFO channel execution.

Requirements:

- One Action in-flight per channel
- Multiple queued Actions per channel (waiters keep earliestStart)
- Channel 0 behaves as though every Action has its own private channel
- Multiple channels execute in parallel
- Hold channel across DelayUntil

---

## Phase 7 - Cancellation — Done

Implement cancellation.

Support:

- Cancel Action / Cancel Channel / Cancel All
- `submit` → runtime action id for cancelAction
- Shared cancel flag visible to workers
- Uncancelable Actions (`Action::setCancelable(false)`)
- Cancel flush (`setCancelFlush`) for immediate cleanup on cancel
- Linked delayed cleanup discard (`removeIfParentCancelled`)

Cancellation occurs only between ActionItems.

ActionItems always run to completion once started.

Uncancelable Actions are skipped by all cancel APIs.

---

## Phase 8 - Pause / Resume — Done

Implement scheduler pause.

Requirements:

- `pause()` / `resume()` / `isPaused()`
- Scheduler dispatches no new work while paused
- Submits while paused are admitted but do not start until resume
- Delayed Actions stop progressing; wake times shift by pause duration on resume
- Workers finish current ActionItem then sleep (WorkerPool pause gate)
- Cancel still works while paused; cancel-flush bypasses pause

---

## Phase 9 - Built-in ActionItems — Done

Implement common ActionItems (split under `App/ActionItems/`).

Types:

- AI_Delay, AI_Script, AI_LaunchApp, AI_Menu, AI_Close
- AI_Keystroke / AI_KeyRelease (hold uses cancel-flush + scheduled release;
  Platform keyDown/keyUp stubbed with logs until available)
- AI_MouseMove (`setMousePos`)
- AI_MouseButton (Platform stub + log)
- AI_Cancel (Cancel Latest by channel/0, Cancel Channel, Cancel All via context → scheduler)
- AI_nthRecent / AI_nthFrequent (1-based; wheel-launch history in `action_history.json`)
- AI_Socket (stub + log)

Settings/JSON support mouse_move, mouse_button, cancel, nth_recent, and nth_frequent.

---

## Phase 10 - Testing — Done

Create comprehensive tests.

Include:

- FIFO correctness
- Channel isolation
- Delay correctness
- Cancellation correctness
- Pause/resume
- Heavy multithreaded stress testing
- Large Action queues
- Randomized scheduling

The scheduler should remain deadlock-free and preserve ordering guarantees.

Runnable as `phase10_scheduler_tests` (`app/tests/phase10_scheduler_tests.cpp`).

---

## Future Enhancements

The architecture should remain flexible enough to support future additions such as:

- Action priorities
- Recurring Actions
- Action dependencies
- Profiling
- Metrics
- Debug visualization
- Resource management
- Background-safe execution policies

The live app uses `Scheduler` for launcher macros (`App::executeAction` submits
a copy). Settings opens with `pause()` and closes with `resume()`.

# Development Rules

When implementing this project:

- Never skip ahead to later phases.
- Keep commits small and focused.
- Implement one class at a time.
- Do not change public APIs without explaining why.
- Prefer clarity over cleverness.
- Favor RAII and standard library facilities.
- Use modern C++20.
- Keep headers lightweight.
- Avoid premature optimization.
- Build after every completed class.
- If an architectural issue is discovered, stop and discuss it before implementing a workaround.