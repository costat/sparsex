/*
 * Copyright (C) 2012-2013, Computing Systems Laboratory (CSLab), NTUA.
 * Copyright (C) 2010-2012, Vasileios Karakasis
 * Copyright (C) 2012-2013, Athena Elafrou
 * All rights reserved.
 *
 * This file is distributed under the BSD License. See LICENSE.txt for details.
 */

/**
 * \file Timer.hpp
 * \brief Timing utilities
 *
 * \author Computing Systems Laboratory (CSLab), NTUA
 * \date 2011&ndash;2014
 * \copyright This file is distributed under the BSD License. See LICENSE.txt
 * for details.
 */

#ifndef TIMER_HPP
#define TIMER_HPP

#include <sys/time.h>
#include <string>

using namespace std;

class Timer
{
public:

    Timer()
        : description_(),
          elapsed_time_(),
          timestamp_()
    {
        Clear();
    }

    Timer(string desc)
        : description_(desc), 
          elapsed_time_(), 
          timestamp_()  
    {
        Clear();
    }

    Timer(const char *desc, const char *desc2)
        : description_((string) desc), 
          elapsed_time_(), 
          timestamp_()
    {
        Clear();
    }

    ~Timer() {}

    void Start();
    void Pause();
    void Stop();
    void Clear();
    double ElapsedTime();

    void SetDescription(const char *desc)
    {
        description_ = (string) desc;
    }

    void SetDescription(string desc)
    {
        description_ = desc;
    }

    string GetDescription()
    {
        return description_;
    }

private:
    string description_;
    struct timeval elapsed_time_;
    struct timeval timestamp_;
};

#endif //TIMER_HPP
