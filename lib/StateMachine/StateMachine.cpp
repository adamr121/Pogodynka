#include "StateMachine.h"
#include "logger.h"

void StateMachine::setState(State state)
{
    if (currentState != state)
    {
        LOG_DEBUG("Zmiana stanu: " << getStateString(currentState) << " -> " << getStateString(state));
        currentState = state;
        if(callback != nullptr){
            callback(currentState);
        }
    }
}

State StateMachine::getState()
{
    return currentState;
}

const char* StateMachine::getStateString(State state)
{
    switch (state)
    {
        case State::INIT:        return "INIT";
        case State::WEATHER:     return "WEATHER";
        case State::LLM:         return "LLM";
        case State::FETCH_AUDIO: return "FETCH_AUDIO";
        case State::PLAY_AUDIO:  return "PLAY_AUDIO";
        case State::DONE:        return "DONE";
        case State::ERROR:       return "ERROR";
        case State::WiFi_CONNECTION:       return "WiFi_CONNECTION";
    }
    return "UNKNOWN";
}

void StateMachine::setChangeStateCallback(void (*cb)(State))
{
    callback = cb;
}

