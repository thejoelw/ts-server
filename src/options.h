#pragma once

#include <string>

class Options {
public:
  static Options &getMutableOptions() {
    static Options options;
    return options;
  }
  static const Options &getOptions() { return getMutableOptions(); }

  std::string dataPath = ".";
};
