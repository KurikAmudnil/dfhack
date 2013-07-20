#pragma once

#include <PluginManager.h>

// the four exported plugin functions

// init plugin state, register commands, hooks
DFhackCExport DFHack::command_result plugin_init( DFHack::color_ostream &out, std::vector<DFHack::PluginCommand> &commands );

// cleanup plugin state
DFhackCExport DFHack::command_result plugin_shutdown( DFHack::color_ostream &out );

// optional event for when dwarf fortress changes state (world loaded / map loaded / etc).
// Invoked with DF suspended, called before plugin_onupdate.
DFhackCExport DFHack::command_result plugin_onstatechange( DFHack::color_ostream &out, DFHack::state_change_event event );

// optional event, called every dwarf fortress frame, also invoked with DF suspended.
DFhackCExport DFHack::command_result plugin_onupdate( DFHack::color_ostream &out );


//static PersistentDataItem config;
//#include "LuaTools.h"
//#include "DataFuncs.h"
