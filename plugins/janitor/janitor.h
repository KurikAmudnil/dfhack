#pragma once

df::job_list_link * assignCleanJob(df::job_list_link *jobslinklist, df::unit *unit, df::coord pos);
bool isUnitBurrowTile(df::map_block *block, df::unit *unit, int x,int y);
void doJanitor(color_ostream &out);


//static PersistentDataItem config;
//#include "LuaTools.h"
//#include "DataFuncs.h"
