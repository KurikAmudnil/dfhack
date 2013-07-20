// This plugin runs the growth / size bug fix

#include "Core.h"
#include <Console.h>
#include <Export.h>
#include <PluginManager.h>

#include "DataDefs.h"
#include "df/unit.h"
#include "df/world.h"
#include "MiscUtils.h"

using namespace DFHack;
using namespace df::enums;

// exported plugin function prototypes
DFhackCExport command_result plugin_init(color_ostream &out, std::vector <PluginCommand> &commands);
DFhackCExport command_result plugin_shutdown(color_ostream &out);
DFhackCExport command_result plugin_onstatechange(color_ostream &out, state_change_event event);
DFhackCExport command_result plugin_onupdate(color_ostream &out);

// local (static) function prototypes
static command_result growthfix_cmd(color_ostream &out, std::vector<std::string> &parameters);
static void printStatus(color_ostream &out);
static void doGrowthFix(color_ostream &out, bool now = false);



//------------------------------
// constants
//------------------------------
static const int TICKS_PER_DAY = 1200;
static const int TICKS_PER_WEEK = TICKS_PER_DAY*7;
static const int TICKS_PER_MONTH = TICKS_PER_DAY*28;

// these defines and enum are so that defaults can be set with preprocessor and allow
// preprocessor if elif to modify the default command example in the help string
#define _DAYS 0
#define _WEEKS 1
#define _MONTHS 2
#define _NOTSET 3
// IntervalType indexes intervalAmounts and intervalTypeStrs
static enum IntervalType { DAYS=_DAYS, WEEKS=_WEEKS, MONTHS=_MONTHS, NOTSET=_NOTSET };
static const int IntervalAmount[] = { TICKS_PER_DAY, TICKS_PER_WEEK, TICKS_PER_MONTH, 0 };
static const string IntervalTypeStr[] = { "day", "week", "month", "error" };

// default configuration options
#define ENABLED_DEFAULT 1
#define SILENT_DEFAULT 0    // silent = 1  ;  verbose = 0
#define ITYPE_DEFAULT _DAYS
#define IAMOUNT_DEFAULT 4

// to stringify IAMOUNT_DEFAULT into the help string
#define STRINGIFY2(x) #x
#define STRINGIFY(x) STRINGIFY2(x)
//------------------------------
//------------------------------


// global plugin state
static bool fix_active = false;
static bool fix_enabled = ENABLED_DEFAULT;
static bool fix_silent = SILENT_DEFAULT;
static IntervalType fix_interval_type = (IntervalType) ITYPE_DEFAULT;
static int fix_interval_type_amount = IAMOUNT_DEFAULT;                    // amount of iType
static int fix_interval = IntervalAmount[fix_interval_type] * fix_interval_type_amount;  // interval in fortress ticks


// Register plugin
DFHACK_PLUGIN("growthfix");


DFhackCExport command_result plugin_init ( color_ostream &out, std::vector <PluginCommand> &commands)
{
    /* default state already set
    fix_enabled = ENABLED_DEFAULT;
    fix_active = false;
    fix_silent = SILENT_DEFAULT;
    fix_interval_type = (IntervalType) ITYPE_DEFAULT;
    fix_interval_type_amount = IAMOUNT_DEFAULT;
    fix_interval = IntervalAmount[fix_interval_type] * iAmount;
    */
    //out.print("GrowthFix: initialized with default settings");

    // Register commands and help string
    commands.push_back(PluginCommand(
        "growthfix", "fix growth bug by rounding unit's birth time down to multiple of 10.",
        growthfix_cmd, false, /* true means that the command can't be used from non-interactive user interface */
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
        "  growthfix -m 1 "
// set silent/verbose example to opposite of default
#if SILENT_DEFAULT
        "verbose"
#else
        "silent"
#endif
        " enable\n"
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


DFhackCExport command_result plugin_shutdown(color_ostream &out)
{
    // nothing to cleanup
    return CR_OK;
}


DFhackCExport command_result plugin_onstatechange(color_ostream &out, state_change_event event)
{
    switch (event) {
    case DFHack::state_change_event::SC_MAP_LOADED:
        fix_active = fix_enabled;
        break;
    case DFHack::state_change_event::SC_MAP_UNLOADED:
        fix_active = false;
        break;
    default:
        break;
    }
    return CR_OK;
}


DFhackCExport command_result plugin_onupdate(color_ostream &out)
{
    static int32_t last_tick = 0;
    // active (implies enabled and world loaded)
    if (fix_active && last_tick != *df::global::cur_year_tick)
    {
        last_tick = *df::global::cur_year_tick;
        // offset by 1 to avoid processing at the same time
        // as other things that run at the start of the day
        if ( (last_tick + 1) % fix_interval == 0 )
        {
            doGrowthFix(out);
        }
    }

    return CR_OK;
}


// command proccessor
static command_result growthfix_cmd(color_ostream &out, std::vector <std::string> &parameters)
{
    if (parameters.empty())
        return CR_WRONG_USAGE;  // CR_WRONG_USAGE = tell dfhack to print help string

    enum { DISABLE=false, ENABLE=true, DEFAULT } 
        cmd_enable = DEFAULT, cmd_silent = DEFAULT;
    IntervalType cmd_type = NOTSET;  // interval type
    int cmd_amount;                  // interval amount
    
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
                cmd_enable = DISABLE;
            else if (cmd == "enable")
                cmd_enable = ENABLE;
            else if (cmd == "silent" || cmd == "-s")
                cmd_silent = ENABLE;
            else if (cmd == "speak"  || cmd == "+s" || cmd == "verbose" || cmd == "-v")
                cmd_silent = DISABLE;
            else if (parameters.size() > i)
            {
                if (cmd == "-d")
                    cmd_type = DAYS;
                else if (cmd == "-w")
                    cmd_type = WEEKS;
                else if (cmd == "-m")
                    cmd_type = MONTHS;

                stringstream ss(parameters[++i]);
                if (cmd_type == NOTSET || !(ss >> cmd_amount) || cmd_amount == 0)
                    return CR_WRONG_USAGE;
            }
            else
                return CR_WRONG_USAGE;
        }

        // if successfully parsed command options, set the changes
        if (cmd_type != NOTSET)
        {
            fix_interval_type = cmd_type;
            fix_interval_type_amount = cmd_amount;
            fix_interval = IntervalAmount[fix_interval_type] * fix_interval_type_amount;
        }

        if (cmd_enable != DEFAULT)
        {
            fix_enabled = cmd_enable;
            fix_active = fix_enabled ? Core::getInstance().isMapLoaded() : false;
        }

        if (cmd_silent != DEFAULT)
            fix_silent = cmd_silent;

        if (cmd_type != NOTSET || cmd_enable != DEFAULT || cmd_silent != DEFAULT)
            printStatus(out);
    }
    return CR_OK;
}


static void printStatus(color_ostream &out)
{
    /*
    // I don't understand why this prints a (null) for the %s%s
    // and another (null) for the last %s
    // or crashes
    out.print("GrowthFix:  %s with interval set to %d %s%s (%s).\n" ,
            (fix_enabled ? "enabled" : "disabled"),
            fix_interval_type_amount, 
            IntervalTypeStr[fix_interval_type],
            (fix_interval_type_amount > 1 ? "s" : ""),
            (fix_silent ? "silent" : "verbose")  );
    */
    out << "GrowthFix:  " << (fix_enabled ? "enabled" : "disabled") << " with interval set to " 
        << fix_interval_type_amount << " " << IntervalTypeStr[fix_interval_type]
        << ((fix_interval_type_amount > 1) ? "s" : "") << (fix_silent ? " (silent)" : " (verbose)") << endl;
}


static void doGrowthFix(color_ostream &out, bool now)
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
    if ( (!fix_silent && count > 0) || now)
        out.print("GrowthFix:  Fixed %d units.\n", count);
        //out << "GrowthFix: Fixed " << count << " units." << endl;
}

