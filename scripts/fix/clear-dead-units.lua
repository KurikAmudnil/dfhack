-- Remove uninteresting dead units from the unit list. Use option 'restore' to put them back 
local utils = require('utils')

local restore = false
for _,arg in pairs({...}) do
    if arg == "restore" then restore = true end
end

local SAVE_FILE = 'data/save/' .. df.global.world.cur_savegame.save_dir .. '/cleared-dead-units.txt'

if restore then
	file = assert(io.open(SAVE_FILE, "r"))
	local units = df.global.world.units
	local count = 0
	local ids = {}
	for n in file:lines() do
		ids[n] = tonumber(n) -- prevent duplicates, not an array
	end
	file:close()
	file = io.open(SAVE_FILE, "w") -- empty file, re-add failures
	for _,id in pairs(ids) do
		local unit = utils.binsearch(units.all,id,'id')
		if unit then
			units.active:insert('#',unit)
			count = count + 1
		else
			file:write(id,NEWLINE)
		end
		id = table.remove(ids,#ids) -- pop laste element
	end
	file:close()
	print('Restored dead units to active from saved list: '..count)
else
	file = assert(io.open(SAVE_FILE, "a"))
	local dwarf_race = df.global.ui.race_id
	local dwarf_civ = df.global.ui.civ_id
	local units = df.global.world.units.active
	local count = 0
	for i=#units-1,0,-1 do
		local unit = units[i]
		local flags1 = unit.flags1
		local flags2 = unit.flags2
		if flags1.dead then
			local remove = false
			if flags2.slaughter then
				remove = true
			elseif unit.enemy.undead then
				remove = true
			elseif not unit.name.has_name then
				remove = true
			elseif unit.civ_id ~= dwarf_civ and not (flags1.merchant or flags1.diplomat) then
				remove = true
			end
			if remove then
				count = count + 1
				file:write(units[i].id, NEWLINE)
				units:erase(i)
			end
		end
	end
	file:close()
	print('Removed dead units from active to saved list: '..count)
end
