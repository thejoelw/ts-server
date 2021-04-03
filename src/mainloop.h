#pragma once

#include "uWebSockets/src/Loop.h"

class MainLoop {
public:
    static MainLoop &getInstance() {
        static MainLoop inst;
        return inst;
    }

    uWS::Loop *loop = 0;
};
