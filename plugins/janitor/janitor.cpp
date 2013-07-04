
#include "Error.h"
#include "Core.h"
#include <Console.h>
#include <Export.h>
#include <PluginManager.h>

// DF data structure definition headers
#include "DataDefs.h"
#include "df/unit.h"
#include "df/job.h"
#include "df/job_item.h"
#include "df/job_list_link.h"
#include "df/general_ref.h"
#include "df/general_ref_unit_workerst.h"
#include "df/burrow.h"
#include "df/block_burrow.h"
#include "df/world.h"
#include "df/ui.h"
#include "modules/Units.h"
#include "modules/Burrows.h"
#include "MiscUtils.h"

using namespace DFHack;
using namespace df::enums;
using df::global::world;
using df::global::ui;
using df::global::job_next_id;

// our own, empty header.
#include "janitor.h"

static const string BURROW_NAME = "\17" " CLEANING DESIGNATIONS " "\17"; //0x0F or 15 decimal, dwarf money symbol

static df::burrow *BURROW = NULL;
static bool enabled = false;
static bool active = false;
static int LAST_UNIT_IDX = 0;


// Here go all the command declarations...
// mostly to allow having the mandatory stuff on top of the file and commands on the bottom
command_result janitor (color_ostream &out, std::vector <std::string> & parameters);

// A plugin must be able to return its name and version.
// The name string provided must correspond to the filename - janitor.plug.so or janitor.plug.dll in this case
DFHACK_PLUGIN("janitor");

// Mandatory init function. If you have some global state, create it here.
DFhackCExport command_result plugin_init ( color_ostream &out, std::vector <PluginCommand> &commands)
{
    // Fill the command list with your commands.
    commands.push_back(PluginCommand(
        "janitor", "Assign idle dwarfs to clean designated tiles on a regular basis.",
        janitor, false, /* true means that the command can't be used from non-interactive user interface */
        // Extended help string. Used by CR_WRONG_USAGE and the help command:
		"  This plugin assigns idle dwarfs to clean tiles (designated \n"
		"  with a special burrow), on a regular basis.\n"
        "Example:\n"
        "  janitor enable\n"
        "     enables the auto assign\n"
        "  janitor disable\n"
        "     disable the auto assign\n"
        "  janitor status\n"
        "     reports enable/disable status\n"
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
        // initialize from the world just loaded
		if (enabled)
		{
			BURROW = Burrows::findByName(BURROW_NAME);
			if (!BURROW)
			{
				BURROW = new df::burrow();
				BURROW->name = BURROW_NAME;
				BURROW->tile = 15;
				BURROW->fg_color = 11;
				BURROW->bg_color = 3;
				BURROW->id = ui->burrows.next_id; //df.global.ui.burrows.next_id
				ui->burrows.list.push_back(BURROW);
				++ui->burrows.next_id;
				out << "Janitor created burrow " << BURROW_NAME << endl;
			}
			active = true;
			out.print("Janitor is active\n");
		}
        break;
	case DFHack::state_change_event::SC_MAP_UNLOADED:
        // cleanup
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
	// active (implies enabled and map loaded)
	if (active && last_tick != *df::global::cur_year_tick)
	{
		last_tick = *df::global::cur_year_tick;
		out << last_tick << endl;
		if ( (last_tick + 600) % 1200 == 0 ) // middle of each day
		{
			out.print("call doJanitor");
			doJanitor(out);
		}
	}

    return CR_OK;
}

// The command processor
command_result janitor(color_ostream &out, std::vector <std::string> & parameters)
{
    // It's nice to print a help message you get invalid options
    // from the user instead of just acting strange.
    // This can be achieved by adding the extended help string to the
    // PluginCommand registration as show above, and then returning
    // CR_WRONG_USAGE from the function. The same string will also
    // be used by 'help your-command'.
    if (parameters.empty())
        return CR_WRONG_USAGE;

    // Commands are called from threads other than the DF one.
    // Suspend this thread until DF has time for us. If you
    // use CoreSuspender, it'll automatically resume DF when
    // execution leaves the current scope.
	CoreSuspender suspend;

    string cmd = toLower(parameters[0]);

    if (cmd == "enable")
    {
		enabled = true;
		out.print("Janitor enabled\n");
		// if map loaded, call init burrow
		if ( Core::getInstance().isMapLoaded() )	plugin_onstatechange(out, SC_MAP_LOADED);
		//plugin_onstatechange(color_ostream &out, state_change_event event)  DFHack::state_change_event::SC_MAP_LOADED
    }
    else if (cmd == "disable")
    {
		enabled = false;
		active = false;
		out.print("Janitor disabled\n");
		// if map loaded ...?
    }
    else if (cmd == "status")
    {
		out << "Janitor is currently " << (enabled ? "enabled" : "disabled") << " and " << (active ? "active" : "inactive") << endl;
    }
    else
        return CR_WRONG_USAGE;

    return CR_OK;

	//CoreSuspender suspend;
    // Give control back to DF.
    //return CR_OK;

}


/*

--------------------------------------------------
---------------  command arguments  --------------
--------------------------------------------------
-- burrow tile?
-- burrow color?
-- burrow name?  if already enabled in a save, then the original name 
--						would need to migrate or ignore new name
-- enable implied
-- how often?
-- max idle dwarfs? -- this should remain as it is for performace reasons (uses a static size at the moment)
-- max scanned dwarfs?
-- max blocks? -- this should remain as it is for performace reasons (uses a static size at the moment)
-- disable?
local args = {...}
do
	for i,arg in ipairs(args) do
		if arg == "disable" then
			ENABLE = false
		elseif arg == "enable" then
			ENABLE = true
		end
		--elseif arg
	end
end
--------------------------------------------------
--------------------------------------------------

--------------------------------------------------
----------------  Protect Burrow  ----------------
--------------------------------------------------
burrow_plugin.onBurrowRename.protectJanitorBurrow = function(burrow)
	if BURROW == burrow and BURROW.name ~= BURROW_NAME then
		-- if not allowed -- BURROW.name = BURROW_NAME
		-- make new burrow for cleaning designations
		-- copy units and tiles, clear units, tiles and name of the old burrow
		BURROW = df.burrow:new()
		BURROW.name = BURROW_NAME
		BURROW.tile = burrow.tile
		BURROW.fg_color = burrow.fg_color
		BURROW.bg_color = burrow.bg_color
		BURROW.id = df.global.ui.burrows.next_id
		df.global.ui.burrows.list:insert('#', BURROW)
		df.global.ui.burrows.next_id = df.global.ui.burrows.next_id + 1
		
		if str_starts(burrow.name,string.char(15))  then burrow.name = "" end
		
		--don't bother copying units at this time because they really shouldn't be assigned to the cleaning designations burrow
		--burrow_plugin.copyUnits(BURROW,burrow,true) -- target, source, add to target
		--dfhack.burrows.clearUnits(burrow)
		
		burrow_plugin.copyTiles(BURROW,burrow,true) -- target, source, add to target
		dfhack.burrows.clearTiles(burrow)
	end
end
--------------------------------------------------
--------------------------------------------------

*/


//--------------------------------------------------
//------------  create and assign job  -------------
//--------------------------------------------------
df::job_list_link * assignCleanJob(df::job_list_link *jobslinklist, df::unit *unit, df::coord pos)//jobslinklist, unit, pos
{
	// create job
	df::job * job = new df::job();
	job->id = *job_next_id;
	job->pos = pos; // shallow copy?

	job->job_type = df::job_type::Clean;
	job->flags.bits.special = true;
	//job.unk4 = 0--1129462340
	*job_next_id++;
	
	// create joblink
	df::job_list_link * joblink = new df::job_list_link();
	job->list_link = joblink;
	joblink->item = job;
	
	// add joblink to list
	jobslinklist->next = joblink;
	joblink->prev = jobslinklist;
	
	// assign job to unit
	df::general_ref_unit_workerst *ref = new df::general_ref_unit_workerst();
	ref->unit_id = unit->id;
	job->general_refs.push_back(ref); // is this right?
	unit->job.current_job = job;
	unit->path.dest = pos; // shallow copy?
	
	// clear current path so that it can recalculate
	if ( unit->path.path.x.size() > 0 )
	{
		unit->path.path.x.resize(0);
		unit->path.path.y.resize(0);
		unit->path.path.z.resize(0);
	}
//print('created job')
	return joblink;
}
//--------------------------------------------------
//--------------------------------------------------

// inlines a rewrite of Burrows::isAssignedUnit, to use burrow id instead of a burrow object

bool isUnitBurrowTile(df::map_block *block, df::unit *unit, int x,int y)
{
	for (	df::block_burrow_link *burrowlink = block->block_burrows.next; 
			burrowlink && (binsearch_index(unit->burrows, burrowlink->item->id) >= 0) && burrowlink->item->getassignment(x,y); 
			burrowlink = burrowlink->next	)
		return true;
	return false;
}





//--------------------------------------------------
//  doJanitor assign idle units to cleaning designations
//--------------------------------------------------
void doJanitor(color_ostream &out)
{
out.print("doJanitor started");
	//CHECK_NULL_POINTER(BURROW);
	if ( !BURROW ) //= NULL
	{
		// shouldn't happen
		//printerr('Janitor: burrow missing');
		out.printerr("Janitor: burrow missing");
		return;
	}
out.print("doJanitor has burrow");
	if ( BURROW->block_x.size() > 0 )
	{
		return;
	}
out.print("doJanitor has blocks");
	// find end of job list
	df::job_list_link *jobslinklist = world->job_list.next;
	for (; jobslinklist->next; jobslinklist = jobslinklist->next)
		;
out.print("doJanitor found end of jobs list");


	//local found = false
out.print("doJanitor initializing iterator");
		/*
			Init Iterator
-- max idle units 10, max units searched 30, max blocks 5
print('init iterator', last_unit_idx)	
	-- if at least 20 units to scan at end of df_units, then scan from where we left off
	-- otherwise start from the beginning
	--local last_unit_idx = (last_unit_idx and last_unit_idx < #df_units - 20) and last_unit_idx or 0

	--local max_unit_idx = (last_unit_idx + 30) < #df_units and (last_unit_idx + 30) or #df_units
		*/
	auto df_units = world->units.active;
	int last_unit_idx = (LAST_UNIT_IDX > df_units.size() - 20) ? 0 : LAST_UNIT_IDX ;
	int max_unit_idx = df_units.size(); //(last_unit_idx + 200) < #df_units and (last_unit_idx + 200) or 

	df::unit *units[] = { false, false, false, false, false, false, false, false, false, false };
	int matrix[5][10] = {  // [block][unit] -- distance weight matrix
		{ 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000 },
		{ 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000 },
		{ 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000 },
		{ 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000 },
		{ 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000 }
	};
	//std::fill_n(matrix, 5, 10000);
	
//print(last_unit_idx, max_unit_idx)
	int idle_count = 0;
	// GET IDLE UNITS
	{
		int i = last_unit_idx;
		for (; i < max_unit_idx ; ++i)
		{
			if	(	Units::isCitizen(df_units[i]) && !df_units[i]->job.current_job && 
					df_units[i]->status.labors[df::unit_labor::CLEAN] && !df_units[i]->flags1.bits.caged )
			{
				units[idle_count] = df_units[i];
				++idle_count;
				if (idle_count >= 10) break ;
			}
		}
		if (i < df_units.size())
			LAST_UNIT_IDX = i; // save last index
		else
			LAST_UNIT_IDX = 0; // next time, scan from the begining
		//end
	}

	while ( idle_count > 0 && BURROW->block_x.size() > 0 )	// iterate next unit
	{
out.print("iterate next unit");
		// populate the distance matrix, this has to be
		// done for each iteration because a block may 
		// have been removed in the previous iteration.
		// find the best pair while we are at it.
		int idx_block = 0;
		int idx_unit = 0;
		int idx_weight = 10000;

		std::vector<df::map_block*> blocks;
		Burrows::listBlocks(&blocks, BURROW);	//	fill blocks with map blocks with burrow designations
		df::map_block* block;

		int num_blocks = (blocks.size() > 5) ? 5 : blocks.size();
		for (int b = 0; b < num_blocks; ++b)
		{
			for (int u=0; u < idle_count; ++u)
			{
				matrix[b][u] = 	std::abs(blocks[b]->map_pos.x + 8 - units[u]->pos.x) + 
								std::abs(blocks[b]->map_pos.y + 8 - units[u]->pos.y) + 
								std::abs(blocks[b]->map_pos.z - units[u]->pos.z);
				if (matrix[b][u] < idx_weight)
				{
					idx_weight = matrix[b][u];
					idx_block = b; idx_unit = u;
				}
			}
		}
		if (num_blocks > 1)
		{
			int swap = matrix[0][idx_unit];
			block = blocks[0];

			matrix[0][idx_unit] = matrix[idx_block][idx_unit];
			blocks[0] = blocks[idx_block];

			matrix[idx_block][idx_unit] = swap;
			blocks[idx_block] = block;

			if (num_blocks > 2)
			{
				// insertion sort, excluding index 0 which is already minimum
				for (int i = 2; i < num_blocks; ++i) // numb_blocks - 1 ?
				{
					// value to insert
					swap = matrix[i][idx_unit];
					block = blocks[i];
					int holePos = i;
					while (holePos > 1 && swap < matrix[holePos - 1][idx_unit])
					{
						matrix[holePos][idx_unit] = matrix[holePos - 1][idx_unit];
						blocks[holePos] = blocks[holePos - 1];
						--holePos;
					}
					matrix[holePos][idx_unit] = swap;
					blocks[holePos] = block;
				}
			}
		}

		// extract unit
		df::unit *unit = units[idx_unit];
		units[idx_unit] = units[idle_count];
		--idle_count;
		bool burrowed = (unit->burrows.size() > 0);

		// iteration: unit, sorted blocks, num_blocks, burrowed

		for (idx_block=0; idx_block < num_blocks; ++idx_block)
		{
			// get the bitmasks for the designations burrow in the block
			block = blocks[idx_block];
			
			//auto block_burrow = Burrows::getBlockMask(BURROW, block, false);
			//auto *tiles = &block_burrow->tile_bitmask;
			auto *tiles = &Burrows::getBlockMask(BURROW, block, false)->tile_bitmask;
			for (int y = 0; y < 16; ++y)
			{
				// does row have tiles assigned? if not, skip 16 pointless itterations
				if (tiles->bits[y] > 0)
				{
					for (int x = 0; x < 16; ++x)
					{
						if (tiles->getassignment(x,y))
						{
							if (block->walkable[x][y] > 0)
							{
								df::coord pos = df::coord(block->map_pos.x + x, block->map_pos.y + y, block->map_pos.z);

								// is the tile accessible to the unit?
								// is the unit in a burrow that is not present on the tile?
								if (Maps::canWalkBetween(unit->pos, pos) && ( !burrowed || ( burrowed && isUnitBurrowTile(block,unit, x, y) ) ) )
								{
									// assign job, receive new job link
									jobslinklist = assignCleanJob(jobslinklist, unit, pos);
out << "unit assigned" << endl;
									// cleanup some unneccessary assignments
									if ( x+2 < 16 && tiles->getassignment(x+2,y) && block->walkable[x+2][y] > 0 )
										tiles->setassignment(x+1,y,false);
									if ( y+2 < 16 && tiles->getassignment(x,y+2) && block->walkable[x][y+2] > 0 )
										tiles->setassignment(x,y+1,false);
									if ( x+2 < 16 && y+2 < 16 && tiles->getassignment(x+2,y+2) && block->walkable[x+2][y+2] > 0 )
									{
										tiles->setassignment(x+1,y+1,false);
										tiles->setassignment(x+2,y+1,false);
										tiles->setassignment(x+1,y+2,false);
									}
									// un-assign the tile from burrow and cleanup the
									// burrow <-> block assignment if necessary
									tiles->setassignment(x,y,false);
									if (!tiles->has_assignments()) Burrows::deleteBlockMask(BURROW, block);
									goto next_unit;
								//else // tile outside of unit burrow, or not currently pathable, continue
									//found = true -- note that we found a valid but inaccesible  designation
								}
								else // not walkable == wall/open space/blocked by building
								{	// un-assign the tile and keep searching if not last assigned tile in block
									tiles->setassignment(x,y,false);
									if (!tiles->has_assignments())
									{
										//cleanup the burrow <-> block assignment
										Burrows::deleteBlockMask(BURROW, block);
										goto next_block;	// break out of for x and for y and continue with next block if present
									}
								}
							}
						}
					} // for [x]
				}
			} // for [y]
			next_block:;
		}	// for block
		next_unit:;
	}
}
