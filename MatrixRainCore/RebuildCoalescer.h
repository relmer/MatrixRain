#pragma once




////////////////////////////////////////////////////////////////////////////////
//
//  RebuildCoalescer
//
//  Collapses a burst of "I need a context rebuild" requests from multiple
//  threads/sources into a single rebuild action.  WM_DISPLAYCHANGE is
//  broadcast to every top-level window, so on a multi-monitor system one
//  topology change can fire N notifications; without coalescing we would
//  tear down and recreate every render context N times.
//
//  Usage from the message pump:
//      case WM_DISPLAYCHANGE:
//          if (m_rebuildCoalescer.RequestRebuild())
//              PostMessage (m_mainHwnd, WM_APP_REBUILD_CONTEXTS, 0, 0);
//          return 0;
//
//      case WM_APP_REBUILD_CONTEXTS:
//          m_rebuildCoalescer.Consume();
//          RebuildContextsForCurrentMode();
//          return 0;
//
//  Thread-safe: backed by std::atomic_flag.  Exactly one concurrent caller
//  of RequestRebuild() will receive true between a given pair of Consume()
//  calls; all others receive false.
//
////////////////////////////////////////////////////////////////////////////////

class RebuildCoalescer
{
    public:
        RebuildCoalescer() = default;

        // Returns true iff this is the first request since construction or
        // the last Consume() call.  Subsequent requests return false until
        // Consume() is called.
        bool RequestRebuild();

        // Resets the pending state so a future RequestRebuild() will once
        // again return true.  Idempotent.
        void Consume();

    private:
        std::atomic_flag m_pending = ATOMIC_FLAG_INIT;
};
