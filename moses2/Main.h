/*
 * Main.h
 *
 *  Created on: 1 Apr 2016
 *      Author: hieu
 */
#pragma once
#include <iostream>

namespace Moses2
{
class Parameter;
class System;
class ThreadPool;
}

std::istream &GetInputStream(Moses2::Parameter &params);
void batch_run(Moses2::Parameter &params, Moses2::System &system, Moses2::ThreadPool &pool);
void run_as_server(Moses2::System &system);

void Temp();


