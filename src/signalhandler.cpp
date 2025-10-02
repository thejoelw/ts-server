#include "signalhandler.h"

#include <csignal>
#include <cstdlib>
#include <unistd.h>

static volatile std::sig_atomic_t stopFlag = false;

void handleSignal(int signal) {
  if (signal != SIGINT) {
    return;
  }

  if (stopFlag) {
    const char msg[] = "Caught a second SIGINT; exiting...\n";
    write(2, msg, sizeof(msg));

    std::_Exit(5);
  } else {
    const char msg[] = "Caught SIGINT; setting stop flag...\n";
    write(2, msg, sizeof(msg));

    stopFlag = true;
  }
}

SignalHandler::SignalHandler() { std::signal(SIGINT, handleSignal); }

SignalHandler::~SignalHandler() { std::signal(SIGINT, SIG_DFL); }

bool SignalHandler::shouldExit() { return stopFlag; }
