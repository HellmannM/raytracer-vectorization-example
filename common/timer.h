// This file is distributed under the MIT license.
// See the LICENSE file for details.

#pragma once

#ifndef VSNRAY_COMMON_TIMER_H
#define VSNRAY_COMMON_TIMER_H 1

#include <chrono>

namespace visionaray
{

//-------------------------------------------------------------------------------------------------
// Timer class using std::chrono's high-resolution clock
//

class timer
{
public:

    typedef std::chrono::high_resolution_clock clock;
    typedef clock::time_point time_point;
    typedef clock::duration duration;

    timer()
        : start_(clock::now())
    {
    }

    void reset()
    {
        start_ = clock::now();
    }

    double elapsed() const
    {
        return std::chrono::duration<double>(clock::now() - start_).count();
    }

private:

    time_point start_;

};


} // visionaray

#endif // VSNRAY_COMMON_TIMER_H
