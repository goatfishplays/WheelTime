/**
 * @file ChannelManager.hpp
 * @brief Per-channel FIFO gate for the scheduler thread.
 */

#pragma once

#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <queue>
#include <unordered_map>

#include "App/Action.hpp"
#include "App/ActionExecutionContext.hpp"

namespace Application
{

/**
 * @brief Result of admitting or promoting work on a channel.
 *
 * Not thread-safe — owned and used only by the scheduler thread.
 */
struct ChannelDispatch
{
    enum class Type
    {
        None,      ///< Nothing to run (queued behind another Action, or channel idle).
        RunNow,    ///< Hand @ref context to WorkerPool immediately.
        ParkUntil  ///< Channel stays busy; park @ref context on DelayQueue until @ref wakeTime.
    };

    Type type = Type::None;
    std::unique_ptr<ActionExecutionContext> context;
    std::chrono::steady_clock::time_point wakeTime{};
};

/**
 * @brief Enforces strict one-in-flight-per-channel FIFO (hold through delays).
 *
 * Channel 0 maps each Action to a private key so those Actions never block
 * one another. Waiting entries preserve earliestStart so future wake times
 * are not lost when the channel is busy.
 */
class ChannelManager
{
public:
    ChannelManager() = default;

    ChannelManager(const ChannelManager &) = delete;
    ChannelManager &operator=(const ChannelManager &) = delete;

    /**
     * @brief Maps Action::channel() to an internal key (private id for channel 0).
     */
    [[nodiscard]] uint32_t resolveKey(const Action &action);

    /**
     * @brief Records which channel key owns @p actionId.
     */
    void bind(uint64_t actionId, uint32_t channelKey);

    /// @brief Channel key for a runtime action id, if still bound.
    [[nodiscard]] std::optional<uint32_t> channelFor(uint64_t actionId) const;

    /// @brief Drops the actionId → channel binding.
    void unbind(uint64_t actionId);

    /**
     * @brief Admits @p context onto its channel.
     *
     * If the channel is free: marks busy and returns RunNow or ParkUntil.
     * If busy: queues {context, earliestStart} and returns None.
     */
    [[nodiscard]] ChannelDispatch admit(
        std::unique_ptr<ActionExecutionContext> context,
        std::chrono::steady_clock::time_point earliestStart,
        std::chrono::steady_clock::time_point now);

    /**
     * @brief Marks the channel idle after Completed/Cancelled and promotes the next waiter.
     */
    [[nodiscard]] ChannelDispatch releaseAndTakeNext(
        uint32_t channelKey,
        std::chrono::steady_clock::time_point now);

    /**
     * @brief Removes waiting contexts for which @p predicate returns true.
     * Does not change busy state of channels.
     */
    [[nodiscard]] std::vector<std::unique_ptr<ActionExecutionContext>> removeWaitingIf(
        const std::function<bool(const ActionExecutionContext &)> &predicate);

    /**
     * @brief Adds @p delta to every waiter's earliestStart (used when resuming from pause).
     */
    void shiftWaitingStarts(std::chrono::steady_clock::duration delta);

    [[nodiscard]] bool isBusy(uint32_t channelKey) const;

private:
    struct WaitingEntry
    {
        std::unique_ptr<ActionExecutionContext> context;
        std::chrono::steady_clock::time_point earliestStart{};
    };

    struct ChannelState
    {
        bool busy = false;
        std::queue<WaitingEntry> waiting;
    };

    [[nodiscard]] ChannelDispatch takeNextLocked(
        ChannelState &channel,
        std::chrono::steady_clock::time_point now);

    std::unordered_map<uint32_t, ChannelState> m_channels;
    std::unordered_map<uint64_t, uint32_t> m_channelByActionId;
    uint32_t m_nextPrivateChannel = 0x80000000u;
};

} // namespace Application
