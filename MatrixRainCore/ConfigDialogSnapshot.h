#pragma once

#include "ScreenSaverSettings.h"





class ApplicationState;





////////////////////////////////////////////////////////////////////////////////
//
//  ConfigDialogSnapshot
//
//  Captures configuration dialog state for live overlay mode, enabling
//  immediate preview updates and Cancel rollback behavior.
//
//  Used by ConfigDialogController when dialog operates in live mode:
//  - Snapshots current settings when dialog opens
//  - Holds reference to ApplicationState for real-time updates
//  - Enables revert-on-Cancel by restoring snapshot values
//
//  v1.5 (data-model.md §3, FR-004, FR-044): the 5 new rollback-eligible
//  v1.5 fields (m_glowEnabled, m_scanlinesEnabled, m_scanlinesIntensity,
//  m_scanlinesStyle, m_customColor) are captured automatically because
//  snapshotSettings is the whole ScreenSaverSettings struct.
//
//  INTENTIONALLY NOT in the snapshot per FR-035: m_customColorPalette is
//  persisted unconditionally on chooser-OK and is NOT rolled back on
//  Cancel.  Because the snapshot copy-restores the whole settings struct,
//  this guarantee is enforced at the controller boundary (CancelLiveMode
//  must re-copy the live palette into the restored snapshot before
//  pushing back into ApplicationState).
//
////////////////////////////////////////////////////////////////////////////////

struct ConfigDialogSnapshot
{
    ScreenSaverSettings   snapshotSettings;
    bool                  isLiveMode;
    ApplicationState    * applicationStateRef;




    ConfigDialogSnapshot() :
        isLiveMode          (false),
        applicationStateRef (nullptr)
    {
    }
};
