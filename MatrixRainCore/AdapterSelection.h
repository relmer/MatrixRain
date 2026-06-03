#pragma once

#include "IAdapterProvider.h"

#include <optional>




////////////////////////////////////////////////////////////////////////////////
//
//  ResolveAdapter
//
//  Maps a persisted adapter description string to the LUID of the matching
//  enumerated adapter, or std::nullopt if no match.
//
//  - savedDescription empty                  -> nullopt (use system default)
//  - savedDescription not present in adapters -> nullopt (saved GPU vanished;
//                                                          fall back to default)
//  - savedDescription matches m_description   -> the matching adapter's LUID
//
//  The provider is responsible for excluding software adapters; this helper
//  does NOT re-filter them.
//
////////////////////////////////////////////////////////////////////////////////

std::optional<LUID> ResolveAdapter (const std::vector<AdapterInfo> & adapters,
                                    const std::wstring             & savedDescription);




////////////////////////////////////////////////////////////////////////////////
//
//  FormatAdapterLabel
//
//  Builds the user-facing combobox label for an adapter:
//   - default adapter:     "<description> (default)"
//   - non-default adapter: "<description>"
//
////////////////////////////////////////////////////////////////////////////////

std::wstring FormatAdapterLabel (const AdapterInfo & adapter);
