/* -*- C++ -*-
 *
 * jit_util.h -- Jit-related utility functions
 *
 * Copyright (C) 2011, Computing Systems Laboratory (CSLab), NTUA.
 * Copyright (C) 2011, Vasileios Karakasis
 * All rights reserved.
 *
 * This file is distributed under the BSD License. See LICENSE.txt for details.
 */

#ifndef JIT_UTIL_H__
#define JIT_UTIL_H__

#include <iostream>
#include <string>
#include <cstdio>
#include <cstdlib>

std::string SourceFromFile(const char *filename);

std::string SourceFromFile(const std::string &filename)
{
    return SourceFromFile(filename.c_str());
}

void SourceToFile(const char *filename, const std::string &source);

void SourceToFile(const std::string &filename, const std::string &source)
{
    SourceToFile(filename.c_str(), source);
}

const char *UniqueFilename(std::string &tmpl);

void RemoveFile(const char *path)
{
    if (remove(path) < 0) {
        perror("RemoveFile()");
        exit(1);
    }
}

#endif  // JIT_UTIL_H__
