/*
 * SODA Player - Event Bus
 * Copyright (C) 2026 SODA Project Contributors
 * License: GPL-3.0-or-later
 */

#ifndef SODA_EVENT_BUS_HPP
#define SODA_EVENT_BUS_HPP

#include "soda.hpp"
#include <unordered_map>

namespace soda {

using SubscriptionId = uint64_t;

class EventBus {
public:
    EventBus() = default;
    ~EventBus() = default;

    // Subscribe to specific event type
    SubscriptionId subscribe(EventType type, EventCallback callback);

    // Subscribe to all events
    SubscriptionId subscribeAll(EventCallback callback);

    // Unsubscribe
    void unsubscribe(SubscriptionId id);

    // Publish event (synchronous)
    void publish(const Event& event);

    // Publish event (asynchronous - queued)
    void publishAsync(const Event& event);

    // Process queued events (call from main loop)
    void processQueue();

    // Helper methods to create and publish events
    void emitPlaybackStarted(const TrackInfo& track);
    void emitPlaybackPaused();
    void emitPlaybackStopped();
    void emitPlaybackProgress(Duration position, Duration total);
    void emitTrackChanged(const TrackInfo& track);
    void emitQueueChanged();
    void emitVolumeChanged(float volume);
    void emitError(const std::string& message);

private:
    struct Subscription {
        SubscriptionId id;
        std::optional<EventType> type; // nullopt means all events
        EventCallback callback;
    };

    std::vector<Subscription> subscriptions_;
    std::queue<Event> eventQueue_;
    std::mutex queueMutex_;
    std::mutex subscriptionsMutex_;
    SubscriptionId nextId_ = 1;
};

} // namespace soda

#endif // SODA_EVENT_BUS_HPP
