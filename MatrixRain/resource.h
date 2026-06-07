//{{NO_DEPENDENCIES}}
// Microsoft Visual C++ generated include file.
// Used by MatrixRain.rc
//
#define IDI_MATRIXRAIN                  101
#define IDD_MATRIXRAIN_SAVER_CONFIG     102
#define IDD_VISUALS_PAGE                103
#define IDD_PERFORMANCE_PAGE            104

// v1.5 timer IDs
#define IDT_PERF_TITLE_TIMER            5001
#define IDT_COLOR_CYCLE_TIMER           5002

// v1.5 string resources
#define IDS_VISUALS_TAB_TITLE           40001
#define IDS_PERFORMANCE_TAB_TITLE_INITIAL 40002
#define IDS_PERFTAB_TITLE_FORMAT        40003

#define IDC_DENSITY_SLIDER              1001
#define IDC_DENSITY_LABEL               1002
#define IDC_ANIMSPEED_SLIDER            1003
#define IDC_ANIMSPEED_LABEL             1004
#define IDC_GLOWINTENSITY_SLIDER        1005
#define IDC_GLOWINTENSITY_LABEL         1006
#define IDC_GLOWSIZE_SLIDER             1007
#define IDC_GLOWSIZE_LABEL              1008
#define IDC_COLORSCHEME_COMBO           1009
#define IDC_STARTFULLSCREEN_CHECK       1010
#define IDC_SHOWDEBUG_CHECK             1011
// 1012 - reserved (was UpdateShowFadeTimers / fade-timer overlay control - removed in v1.5 US4)
// 1013 - reserved (was IDC_RESET_BUTTON, page-scoped pushbutton - replaced
//        in mini-phase 2.5 by frame-scope IDC_RESET_DEFAULTS=1051)
#define IDC_MULTIMONITOR_CHECK          1014
#define IDC_MULTIMONITOR_INFO           1015
#define IDC_GPU_COMBO                   1016
#define IDC_GPU_INFO                    1017
#define IDC_QUALITY_PRESET_SLIDER       1018
#define IDC_QUALITY_PRESET_INFO         1019
#define IDC_GRAPHICS_ADVANCED_CHECK     1020
#define IDC_GRAPHICS_ADVANCED_INFO      1021
#define IDC_GLOWPASSES_SLIDER           1022
#define IDC_GLOWPASSES_LABEL            1023
#define IDC_GLOWPASSES_INFO             1024
#define IDC_GLOWRES_SLIDER              1025
#define IDC_GLOWRES_LABEL               1026
#define IDC_GLOWRES_INFO                1027
#define IDC_GLOWSMOOTH_SLIDER           1028
#define IDC_GLOWSMOOTH_LABEL            1029
#define IDC_GLOWSMOOTH_INFO             1030
#define IDC_GLOWINTENSITY_INFO          1031
#define IDC_GLOWSIZE_INFO               1032
// 1033 - reserved (was IDC_QUALITY_GROUPBOX, removed in mini-phase 2.5
//        layout flattening; the quality cluster is no longer wrapped)
#define IDC_GLOWPASSES_PROMPT           1034
#define IDC_GLOWRES_PROMPT              1035
#define IDC_GLOWSMOOTH_PROMPT           1036
#define IDC_QUALITY_PRESET_LABEL        1037

// v1.5 new control IDs.
// Note: existing v1.4 colour combo is IDC_COLORSCHEME_COMBO (1009) — DO NOT
// introduce a new IDC_COLOR_COMBO; the Custom… item is appended to the
// existing combo per Phase 6.
#define IDC_GLOW_ENABLED_CHECK              1038
#define IDC_SCANLINES_GROUPBOX              1039
#define IDC_SCANLINES_ENABLED_CHECK         1040
#define IDC_SCANLINES_INTENSITY_SLIDER      1041
#define IDC_SCANLINES_INTENSITY_LABEL       1042
#define IDC_SCANLINES_INTENSITY_VALUE       1043
#define IDC_SCANLINES_INTENSITY_INFO        1044
#define IDC_SCANLINES_INTENSITY_PROMPT      1045
#define IDC_SCANLINES_STYLE_SLIDER          1046
#define IDC_SCANLINES_STYLE_LABEL           1047
#define IDC_SCANLINES_STYLE_VALUE           1048
#define IDC_SCANLINES_STYLE_INFO            1049
#define IDC_SCANLINES_STYLE_PROMPT          1050

// Mini-phase 2.5: cross-page "Reset to defaults" pushbutton, hosted on the
// property-sheet frame (not a page) so a single click resets every control
// on both tabs.  Created programmatically in PSCB_INITIALIZED — property
// sheets don't expose a native Reset button.
#define IDC_RESET_DEFAULTS                  1051

// v1.5: per-page bottom-right FPS / GPU readout (replaces the live FPS/GPU
// suffix on the Performance tab title).  Same control ID on both pages so
// the timer proc can update them uniformly via GetDlgItem on each page.
#define IDC_FPS_GPU_READOUT                 1052

// v1.5: owner-draw colour swatch shown to the right of IDC_COLORSCHEME_-
// COMBO on the Visuals page.  Reflects the currently-selected scheme:
// fills with the static palette entry, the custom colour, or the
// animated cycle colour (driven at ~30Hz by IDT_COLOR_CYCLE_TIMER).
#define IDC_COLOR_SWATCH                    1053

// Next default values for new objects
// 
#ifdef APSTUDIO_INVOKED
#ifndef APSTUDIO_READONLY_SYMBOLS
#define _APS_NEXT_RESOURCE_VALUE        105
#define _APS_NEXT_COMMAND_VALUE         40004
#define _APS_NEXT_CONTROL_VALUE         1054
#define _APS_NEXT_SYMED_VALUE           101
#endif
#endif
