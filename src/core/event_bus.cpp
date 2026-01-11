/*
 * SODA Player - Event Bus Implementation
 * Copyright (C) 2026 SODA Project Contributors
 * License: GPL-3.0-or-later
 */

#include <soda/event_bus.hpp>

namespace soda {

SubscriptionId EventBus::subscribe(EventType type, EventCallback callback) {
    std::lock_guard<std::mutex> lock(subscriptionsMutex_);
    SubscriptionId id = nextId_++;
    subscriptions_.push_back({id, type, std::move(callback)});
    return id;
}

SubscriptionId EventBus::subscribeAll(EventCallback callback) {
    std::lock_guard<std::mutex> lock(subscriptionsMutex_);
    SubscriptionId id = nextId_++;
    subscriptions_.push_back({id, std::nullopt, std::move(callback)});
    return id;
}

void EventBus::unsubscribe(SubscriptionId id) {
    std::lock_guard<std::mutex> lock(subscriptionsMutex_);
    subscriptions_.erase(
        std::remove_if(subscriptions_.begin(), subscriptions_.end(),
                       [id](const Subscription& sub) { return sub.id == id; }),
        subscriptions_.end());
}

void EventBus::publish(const Event& event) {
    std::vector<EventCallback> callbacksToCall;

    {
        std::lock_guard<std::mutex> lock(subscriptionsMutex_);
        for (const auto& sub : subscriptions_) {
            if (!sub.type.has_value() || sub.type.value() == event.type) {
                callbacksToCall.push_back(sub.callback);
            }
        }
    }

    // Call callbacks outside of lock to avoid deadlocks
    for (const auto& callback : callbacksToCall) {
        try {
            callback(event);
        } catch (...) {
            // Ignore callback exceptions to prevent one bad handler from breaking others
        }
    }
}

void EventBus::publishAsync(const Event& event) {
    std::lock_guard<std::mutex> lock(queueMutex_);
    eventQueue_.push(event);
}

void EventBus::processQueue() {
    std::queue<Event> toProcess;

    {
        std::lock_guard<std::mutex> lock(queueMutex_);
        std::swap(toProcess, eventQueue_);
    }

    while (!toProcess.empty()) {
        publish(toProcess.front());
        toProcess.pop();
    }
}

void EventBus::emitPlaybackStarted(const TrackInfo& track) {
    Event event;
    event.type = EventType::PlaybackStarted;
    event.data = track;
    publish(event);
}

void EventBus::emitPlaybackPaused() {
    Event event;
    event.type = EventType::PlaybackPaused;
    publish(event);
}

void EventBus::emitPlaybackStopped() {
    Event event;
    event.type = EventType::PlaybackStopped;
    publish(event);
}

void EventBus::emitPlaybackProgress(Duration position, Duration total) {
    Event event;
    event.type = EventType::PlaybackProgress;
    // Encode both values as a single double (position.count() + total.count() * 1e9)
    // This is a simplification; in production you'd use a proper struct
    event.data = static_cast<double>(position.count());
    publish(event);
}

void EventBus::emitTrackChanged(const TrackInfo& track) {
    Event event;
    event.type = EventType::TrackChanged;
    event.data = track;
    publish(event);
}

void EventBus::emitQueueChanged() {
    Event event;
    event.type = EventType::QueueChanged;
    publish(event);
}

void EventBus::emitVolumeChanged(float volume) {
    Event event;
    event.type = EventType::VolumeChanged;
    event.data = static_cast<double>(volume);
    publish(event);
}

void EventBus::emitError(const std::string& message) {
    Event event;
    event.type = EventType::Error;
    event.data = message;
    publish(event);
}

} // namespace soda
