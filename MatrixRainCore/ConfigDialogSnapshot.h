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
