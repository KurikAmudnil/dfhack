// This is a generic plugin that does nothing useful apart from acting as an example... of a plugin that does nothing :D

// some headers required for a plugin. Nothing special, just the basics.
#include "Core.h"
#include <Console.h>
#include <Export.h>
#include <PluginManager.h>

// DF data structure definition headers
#include "DataDefs.h"
#include "df/unit.h"
#include "df/world.h"
#include "MiscUtils.h"

using namespace DFHack;
using namespace df::enums;

// our own, empty header.
#include "growthfix.h"

#define TICKS_PER_DAY 1200
#define TICKS_PER_WEEK TICKS_PER_DAY*7
#define TICKS_PER_MONTH TICKS_PER_DAY*28

// Here go all the command declarations...
// mostly to allow having the mandatory stuff on top of the file and commands on the bottom
command_result growthfix (color_ostream &out, std::vector <std::string> & parameters);

// A plugin must be able to return its name and version.
// The name string provided must correspond to the filename - skeleton.plug.so or skeleton.plug.dll in this case
DFHACK_PLUGIN("growthfix");


// these defines and enum are so that defaults can be set with preprocessor and allow
// preprocessor if elif to modify the default command example in the help string
#define _DAYS 0
#define _WEEKS 1
#define _MONTHS 2
#define _NOTSET 3
// IntervalType indexes intervalAmounts and intervalTypeStrs
static enum IntervalType { DAYS=_DAYS, WEEKS=_WEEKS, MONTHS=_MONTHS, NOTSET=_NOTSET };
static const int intervalAmounts[] = { TICKS_PER_DAY, TICKS_PER_WEEK, TICKS_PER_MONTH, 0 };
static const string intervalTypeStrs[] = { " days", " weeks", " months", " " };

// default configuration options
#define ENABLED_DEFAULT 1
#define SILENT_DEFAULT 1
#define ITYPE_DEFAULT _DAYS
#define IAMOUNT_DEFAULT 4

// to stringify IAMOUNT_DEFAULT
#define STRINGIFY2(x) #x
#define STRINGIFY(x) STRINGIFY2(x)

// state
static bool active = false;
static bool enabled = ENABLED_DEFAULT;
static bool silent = SILENT_DEFAULT;
static IntervalType iType = (IntervalType) ITYPE_DEFAULT;
static int iAmount = IAMOUNT_DEFAULT;                   // amount of iType
static int interval = intervalAmounts[iType] * iAmount; // interval in fortress ticks



void doGrowthFix(color_ostream &out, bool now = false)
{
    int count = 0;
    auto units = df::global::world->units.active;
    for (int i=0, r=0; i < units.size(); ++i)
    {
        r = units[i]->relations.birth_time % 10;
        if (r > 0 && units[i]->relations.birth_time > 0 )
        {
            units[i]->relations.birth_time -= r;
            ++count;
        }
    }
    if ( (!silent && count > 0) || now)
        out << "GrowthFix: Fixed " << count << " units." << endl;
}

void printStatus(color_ostream &out)
{
    out << "GrowthFix: " << (enabled ? "enabled" : "disabled") << " with interval set to " 
        << iAmount << intervalTypeStrs[iType] << (silent ? " (silent)" : " (verbose)") << endl;
    //" (silent mode: " << (silent ? "on)" : "off)")
}

// Mandatory init function. If you have some global state, create it here.
DFhackCExport command_result plugin_init ( color_ostream &out, std::vector <PluginCommand> &commands)
{
    /* default state already set
    enabled = ENABLED_DEFAULT;
    active = false;
    silent = SILENT_DEFAULT;
    iType = (IntervalType) ITYPE_DEFAULT;
    iAmount = IAMOUNT_DEFAULT;
    interval = intervalAmounts[iType] * iAmount;
    */
    //out.print("GrowthFix: initialized with default settings");

    // Fill the command list with your commands.
    commands.push_back(PluginCommand(
        "growthfix", "fix growth bug by rounding unit's birth time down to multiple of 10.",
        growthfix, false, /* true means that the command can't be used from non-interactive user interface */
        // Extended help string. Used by CR_WRONG_USAGE and the help command:
        "  Fix growth/size bug by rounding unit's birth time down to a multiple of 10.\n"
        "Options (case insensitive):\n"
        "  status\n"
        "     print current auto fix configuration\n"
        "  now\n"
        "     run growthfix once, right now (always reports count of units fixed)\n"
        "(if none of the above, the following can be together in any order)\n"
        "  enable | disable\n"
        "     enable or disable the fix to run periodically.\n"
        "  silent | verbose\n"
        "     auto fix silently or print number of units fixed (if any).\n"
        "  -d number | -w number| -m number\n"
        "     sets the interval between auto fix runs to number of days (-d) \n"
        "     or weeks (-w) or months (-m) where number is a whole number >= 1 \n"
        "Examples:\n"
        "  growthfix status\n"
        "  growthfix now\n"
        "  growthfix -w 2\n"
        "  growthfix disable\n"
        "  growthfix -m 1 verbose enable\n"
        "Default configuration is equivilent to:\n"
        "  growthfix"
#if ENABLED_DEFAULT
        " enable"
#else
        " disable"
#endif
#if ITYPE_DEFAULT == _DAYS
        " -d "
#elif ITYPE_DEFAULT == _WEEKS
        " -w "
#elif ITYPE_DEFAULT == _MONTHS
        " -m "
#endif
        STRINGIFY(IAMOUNT_DEFAULT)
#if SILENT_DEFAULT
        " silent"
#else
        " verbose"
#endif
        "\n\n"
    ));
    return CR_OK;
}

// This is called right before the plugin library is removed from memory.
DFhackCExport command_result plugin_shutdown ( color_ostream &out )
{
    // You *MUST* kill all threads you created before this returns.
    // If everything fails, just return CR_FAILURE. Your plugin will be
    // in a zombie state, but things won't crash.
    return CR_OK;
}


// Called to notify the plugin about important state changes.
// Invoked with DF suspended, and always before the matching plugin_onupdate.
// More event codes may be added in the future.
DFhackCExport command_result plugin_onstatechange(color_ostream &out, state_change_event event)
{
    switch (event) {
    case DFHack::state_change_event::SC_MAP_LOADED:
        active = enabled;
        break;
    case DFHack::state_change_event::SC_MAP_UNLOADED:
        active = false;
        break;
    default:
        break;
    }
    return CR_OK;
}

// Whatever you put here will be done in each game step. Don't abuse it.
// It's optional, so you can just comment it out like this if you don't need it.
DFhackCExport command_result plugin_onupdate ( color_ostream &out )
{
    static int32_t last_tick = 0;
    // active (implies enabled and world loaded)
    if (active && last_tick != *df::global::cur_year_tick)
    {
        last_tick = *df::global::cur_year_tick;
        // offset by 1 to avoid processing at the same time
        // as other things that run at the start of the day
        if ( (last_tick + 1) % interval == 0 )
        {
            doGrowthFix(out);
        }
    }

    return CR_OK;
}

// command proccessor
command_result growthfix (color_ostream &out, std::vector <std::string> & parameters)
{
    // It's nice to print a help message you get invalid options
    // from the user instead of just acting strange.
    // This can be achieved by adding the extended help string to the
    // PluginCommand registration as show above, and then returning
    // CR_WRONG_USAGE from the function. The same string will also
    // be used by 'help your-command'.
    if (parameters.empty())
        return CR_WRONG_USAGE;

    enum { DISABLE=false, ENABLE=true, DEFAULT } cmdEnable = DEFAULT, cmdSilent = DEFAULT;
    IntervalType cmdiType = NOTSET;
    int cmdiAmount;
    
    string cmd = toLower(parameters[0]);
    if (cmd == "status")
    {
        printStatus(out);
        return CR_OK;
    }
    else if (cmd == "now")
    {
        // using isMapLoaded so that "now" can supposedly work in arena mode
        // and to perhaps exclude legends mode
        if ( Core::getInstance().isMapLoaded() )
        {
            CoreSuspender suspend;  // Suspend the commands thread until DF is ready.
            doGrowthFix(out, true);
            return CR_OK;           // Give control back to DF
        }
        else
        {
            out.printerr("GrowthFix \"now\":  World not loaded.\n");
            return CR_FAILURE;
        }
    }
    else
    {
        for (int i = 0; i < parameters.size(); ++i)
        {
            cmd = toLower(parameters[i]);

            if (cmd == "disable")
                cmdEnable = DISABLE;
            else if (cmd == "enable")
                cmdEnable = ENABLE;
            else if (cmd == "silent" || cmd == "-s")
                cmdSilent = ENABLE;
            else if (cmd == "speak"  || cmd == "+s" || cmd == "verbose" || cmd == "-v")
                cmdSilent = DISABLE;
            else if (parameters.size() > i)
            {
                if (cmd == "-d")
                    cmdiType = DAYS;
                else if (cmd == "-w")
                    cmdiType = WEEKS;
                else if (cmd == "-m")
                    cmdiType = MONTHS;

                stringstream ss(parameters[++i]);
                if (cmdiType == NOTSET || !(ss >> cmdiAmount) || cmdiAmount == 0)
                    return CR_WRONG_USAGE;
            }
            else
                return CR_WRONG_USAGE;
        }

        // if successfully parsed command options, set the changes
        if (cmdiType != NOTSET)
        {
            iType = cmdiType;
            iAmount = cmdiAmount;
            interval = intervalAmounts[iType] * iAmount;
        }

        if (cmdEnable != DEFAULT)
        {
            enabled = cmdEnable;
            // if enabled, activate if map already loaded
            active = enabled ? Core::getInstance().isMapLoaded() : false;
        }

        if (cmdSilent != DEFAULT)
            silent = cmdSilent;

        if (cmdiType != NOTSET || cmdEnable != DEFAULT || cmdSilent != DEFAULT)
            printStatus(out);
    }
    return CR_OK;
}
