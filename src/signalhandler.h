#pragma once

class SignalHandler {
public:
  static SignalHandler &getInstance() {
    static SignalHandler inst;
    return inst;
  }

  SignalHandler();
  ~SignalHandler();

  bool shouldExit();
};
