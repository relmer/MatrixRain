#include "pch.h"

#include "RebuildCoalescer.h"




////////////////////////////////////////////////////////////////////////////////
//
//  RebuildCoalescer::RequestRebuild
//
////////////////////////////////////////////////////////////////////////////////

bool RebuildCoalescer::RequestRebuild()
{
    // test_and_set returns the PREVIOUS flag state.  If the flag was clear
    // (false) we are the first request since the last Consume() and return
    // true; if already set (true) we coalesce by returning false.
    return !m_pending.test_and_set (std::memory_order_acq_rel);
}




////////////////////////////////////////////////////////////////////////////////
//
//  RebuildCoalescer::Consume
//
////////////////////////////////////////////////////////////////////////////////

void RebuildCoalescer::Consume()
{
    m_pending.clear (std::memory_order_release);
}
