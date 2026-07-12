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

Actions describe work.

They contain no execution state.

---

# ActionExecutionContext

Workers execute ActionExecutionContexts rather than Actions directly.

The context owns:

- Action
- Current ActionItem index
- Execution metadata
- Scheduler requests generated during execution

The context represents the current execution state.

---

# Channels

Channels serialize execution.

Rules:

- Multiple Actions may exist in the same channel.
- Channels are strict FIFO.
- Only one Action from a channel may execute at a time.
- Different channels may execute simultaneously.

Workers continue executing ActionItems until:

- the Action finishes

or

- an ActionItem returns DelayUntil(...)

---

# Channel 0

Channel 0 is special.

Every Action submitted to channel 0 behaves as though it owns its own private channel.

Actions submitted to channel 0 never block one another.

Channel 0 still flows through the scheduler.

---

# ActionItem API

Every ActionItem implements:

```cpp
ExecuteResult execute(ActionExecutionContext&);
```

ActionItems never interact directly with the scheduler.

Instead they record scheduler requests inside the ActionExecutionContext.

Example:

```cpp
context.scheduleAction(...);
```

The scheduler consumes those requests after execute() returns.

---

# ExecuteResult

ExecuteResult only describes the fate of the CURRENT Action.

Current operations:

- Continue
- DelayUntil(time_point)

Future operations may be added if necessary.

ExecuteResult should remain small.

---

# Delays

Workers never wait.

If an Action delays:

- Worker returns ActionExecutionContext to scheduler.
- Scheduler stores it in a priority queue ordered by wake time.
- Scheduler resumes it later.

---

# Cancellation

Cancellation exists at three levels.

## Cancel Action

Queued:

- remove immediately

Delayed:

- discard

Executing:

- finish current ActionItem
- terminate Action afterward

---

## Cancel Channel

- finish current ActionItem
- terminate current Action
- remove queued Actions
- remove delayed Actions

---

## Cancel All

Equivalent to cancelling every user Action.

ActionItems are atomic.

Cancellation only occurs between ActionItems.

---

# Pause

The scheduler supports pause/resume.

While paused:

- scheduler dispatches no new work
- delayed Actions stop progressing
- workers finish their current ActionItem
- workers then sleep

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

- tracks execution state

ActionItem

- performs work

ExecuteResult

- tells scheduler whether the current Action should continue or delay

Scheduler

- scheduling decisions only

WorkerPool

- execution only

---

# Implementation Roadmap

The scheduler should be implemented incrementally.

Each phase should compile and be tested before moving on.

---

## Phase 1 - Core Types

No threading.

Implement:

- Action
- ActionItem
- ExecuteResult
- ActionExecutionContext

Goal:

Define the core execution model and public APIs.

---

## Phase 2 - Thread-Safe Queue

Implement a reusable MPMC thread-safe queue.

Features:

- push
- pop
- clear
- stop
- condition variable

This queue will later be reused by the WorkerPool and Scheduler.

---

## Phase 3 - WorkerPool

Implement the worker thread pool.

Responsibilities:

- Create N worker threads
- Wait for ActionExecutionContexts
- Execute ActionItems
- Return execution results to the scheduler

Workers should contain no scheduling logic.

Initially the Scheduler may be mocked if necessary.

---

## Phase 4 - Scheduler

Implement the scheduler thread.

Responsibilities:

- Accept submitted Actions
- Own ActionExecutionContexts
- Dispatch work to workers
- Process ExecuteResults
- Process scheduler requests
- Manage channel ownership

No cancellation or pause yet.

Goal:

Basic scheduling should function.

---

## Phase 5 - Delay Queue

Implement delayed execution.

Requirements:

- Priority queue ordered by wake time
- Scheduler sleeps until:
    - new Action
    - earliest wake time
    - stop request

Workers never wait for delays.

---

## Phase 6 - Channel Management

Implement strict FIFO channel execution.

Requirements:

- One Action executing per channel
- Multiple queued Actions per channel
- Channel 0 behaves as though every Action has its own private channel
- Multiple channels execute in parallel

---

## Phase 7 - Cancellation

Implement cancellation.

Support:

- Cancel Action
- Cancel Channel
- Cancel All

Cancellation occurs only between ActionItems.

ActionItems always run to completion once started.

---

## Phase 8 - Pause / Resume

Implement scheduler pause.

Requirements:

- Scheduler dispatches no new work
- Delayed Actions stop progressing
- Workers finish current ActionItem
- Workers sleep until resumed

---

## Phase 9 - Built-in ActionItems

Implement common ActionItems.

Examples:

- DelayItem
- ScriptItem
- LaunchProcessItem
- KeyPressItem
- MouseMoveItem
- MouseButtonItem

These should use the existing Platform library.

---

## Phase 10 - Testing

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