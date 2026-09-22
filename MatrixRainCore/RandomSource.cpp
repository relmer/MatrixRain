#include "pch.h"

#include "RandomSource.h"





namespace RandomSource
{


////////////////////////////////////////////////////////////////////////////////
//
//  RandomSource::Engine
//
////////////////////////////////////////////////////////////////////////////////

std::mt19937 & Engine() noexcept
{
    //  Constructed on first use per thread, so a Reseed that runs before any
    //  draw still costs one random_device read and nothing more.
    static thread_local std::random_device s_randomDevice;
    static thread_local std::mt19937       s_engine (s_randomDevice());



    return s_engine;
}





////////////////////////////////////////////////////////////////////////////////
//
//  RandomSource::Reseed
//
////////////////////////////////////////////////////////////////////////////////

void Reseed (uint32_t seed) noexcept
{
    Engine().seed (seed);
}


}
