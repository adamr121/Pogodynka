#ifndef STATEMACHINE_H
#define STATEMACHINE_H

enum class State{
    INIT,
    WEATHER,
    LLM,
    FETCH_AUDIO,
    PLAY_AUDIO,
    DONE,
    ERROR,
    WiFi_CONNECTION
};

//using StateCallback = void (*)(State);

class StateMachine{
public:
    void setState(State state);
    State getState();
    const char* getStateString(State state);
    void setChangeStateCallback(void (*cb)(State)); // jaka funkcja ma się wywoływać na zmianie stanu
private:
    State currentState;
    void (*callback)(State) = nullptr;
};
#endif