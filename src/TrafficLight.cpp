#include <iostream>
#include <random>
#include "TrafficLight.h"

/* Implementation of class "MessageQueue" */


template <typename T>
T MessageQueue<T>::receive()
{
    // DONE
    // FP.5a : The method receive should use std::unique_lock<std::mutex> and _condition.wait() 
    // to wait for and receive new messages and pull them from the queue using move semantics. 
    // The received object should then be returned by the receive function. 

    std::unique_lock<std::mutex> recvLock(_mutex);  // unique_lock can be moved to new owner
    
    _condition.wait(recvLock, [this]() { return !_queue.empty(); });    // wait can assign the lock immediately on resource being available
    
    T newMessage = std::move(_queue.back());    // first in first out
    
    _queue.pop_back();  // doesn't reutrn the element from the back

    return newMessage;
}

template <typename T>
void MessageQueue<T>::send(T &&msg)
{
    // DONE
    // FP.4a : The method send should use the mechanisms std::lock_guard<std::mutex> 
    // as well as _condition.notify_one() to add a new message to the queue and afterwards send a notification.

    std::lock_guard<std::mutex> sendLock(_mutex);   // release on scope loss
    _queue.emplace_back(std::move(msg));            // emplace over push_back 

    _condition.notify_one();                        // final step
}

/* Implementation of class "TrafficLight" */

TrafficLight::TrafficLight()
{
    _currentPhase = TrafficLightPhase::red;
}

void TrafficLight::waitForGreen()
{
    // DONE
    // FP.5b : add the implementation of the method waitForGreen, in which an infinite while-loop 
    // runs and repeatedly calls the receive function on the message queue. 
    // Once it receives TrafficLightPhase::green, the method returns.
    while(_messages.receive() == TrafficLightPhase::red)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

TrafficLightPhase TrafficLight::getCurrentPhase()
{
    return _currentPhase;
}

void TrafficLight::simulate()
{
    // DONE
    // FP.2b : Finally, the private method „cycleThroughPhases“ should be started in a thread when the public method „simulate“ is called.
    // To do this, use the thread queue in the base class. 
    
    // Threads is in TrafficObject
    // emplace rather than push_back
    threads.emplace_back(&TrafficLight::cycleThroughPhases, this);
}

// virtual function which is executed in a thread
void TrafficLight::cycleThroughPhases()
{
    // DONE
    // FP.2a : Implement the function with an infinite loop that measures the time between two loop cycles 
    // and toggles the current phase of the traffic light between red and green and sends an update method 
    // to the message queue using move semantics. The cycle duration should be a random value between 4 and 6 seconds. 
    // Also, the while-loop should use std::this_thread::sleep_for to wait 1ms between two cycles. 

    // need timers
    // Want accurate milliseconds that isn't affected by chaning computer time info
    // https://stackoverflow.com/questions/52421819/does-steady-clocknow-return-seconds
    auto startTime = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
    auto endTime = startTime;


    // interval tracking
    // auto used here to keep var type matching timers.
    // rand : https://stackoverflow.com/questions/12657962/how-do-i-generate-a-random-number-between-two-variables-that-i-have-stored#12657984
    auto targetCycleDuration = (endTime - startTime) + ((rand()%2001) + 4000);

    // Light color alternating loop
    while(true)
    {

        // while we need to keep waiting
        while(((endTime = std::chrono::duration_cast<std::chrono::milliseconds>
            (std::chrono::steady_clock::now().time_since_epoch()).count()) -
            startTime) < targetCycleDuration)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(targetCycleDuration - (endTime - startTime)));
        }

        // now update start comparison
        startTime = endTime;

        // alternate light
        if (_currentPhase == TrafficLightPhase::red)
            _currentPhase = TrafficLightPhase::green;
        else
            _currentPhase = TrafficLightPhase::red;
        
        _messages.send(std::move(_currentPhase));   // As per FP.4b

        // random millis for next delay
        targetCycleDuration = (rand()%2001) + 4000;

        // minimum 1ms delay between runs
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}