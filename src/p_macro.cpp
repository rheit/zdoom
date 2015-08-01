#include "p_macro.h"
#include "i_system.h"

IMPLEMENT_CLASS (DMacroThinker)

DMacroThinker::DMacroThinker() : DThinker(), currentsequence(0), currentstatus(0), started(false), 
								 delaycounter(0), pos(0), line(NULL), actor(NULL), backside(false),
								 playerfreeze(false)
{
}

DMacroThinker::~DMacroThinker()
{
	specials.Clear();
	specials.ShrinkToFit();
	// Todo: make something that works
}

void DMacroThinker::Serialize(FArchive &arc)
{
	Super::Serialize(arc);
	// Todo: make something that works
}

// Returns true if the special corresponds to a type of special that gives
// a sector a floordata, ceilingdata or lightingdata; false otherwise.
bool IsSectorWaitSpecial(int i)
{
	switch (i)
	{
	case Door_Close:
	case Door_Open:
	case Door_Raise:
	case Door_LockedRaise:
	case Door_Animated:
	case Floor_LowerByValue:
	case Floor_LowerToLowest:
	case Floor_LowerToNearest:
	case Floor_RaiseByValue:
	case Floor_RaiseToHighest:
	case Floor_RaiseToNearest:
	case Stairs_BuildDown:
	case Stairs_BuildUp:
	case Floor_RaiseAndCrush:
	case Pillar_Build:
	case Pillar_Open:
	case Stairs_BuildDownSync:
	case Stairs_BuildUpSync:
	case Floor_RaiseByValueTimes8:
	case Floor_LowerByValueTimes8:
	case Floor_MoveToValue:
	case Ceiling_Waggle:
	case Ceiling_LowerByValue:
	case Ceiling_RaiseByValue:
	case Ceiling_CrushAndRaise:
	case Ceiling_LowerAndCrush:
	case Ceiling_CrushStop:
	case Ceiling_CrushRaiseAndStay:
	case Floor_CrushStop:
	case Ceiling_MoveToValue:
	case Plat_PerpetualRaise:
	case Plat_DownWaitUpStay:
	case Plat_DownByValue:
	case Plat_UpWaitDownStay:
	case Plat_UpByValue:
	case Floor_LowerInstant:
	case Floor_RaiseInstant:
	case Floor_MoveToValueTimes8:
	case Ceiling_MoveToValueTimes8:
	case ACS_LockedExecuteDoor:
	case Pillar_BuildAndCrush:
	case FloorAndCeiling_LowerByValue:
	case FloorAndCeiling_RaiseByValue:
	case Door_Split:
	case Light_ForceLightning:
	case Light_RaiseByValue:
	case Light_LowerByValue:
	case Light_ChangeToValue:
	case Light_Fade:
	case Light_Glow:
	case Light_Flicker:
	case Light_Strobe:
	case Sector_Transform:
	case Generic_Crusher2:
	case Plat_UpNearestWaitDownStay:
	case Ceiling_LowerToHighestFloor:
	case Ceiling_LowerInstant:
	case Ceiling_RaiseInstant:
	case Ceiling_CrushRaiseAndStayA:
	case Ceiling_CrushAndRaiseA:
	case Ceiling_CrushAndRaiseSilentA:
	case Ceiling_RaiseByValueTimes8:
	case Ceiling_LowerByValueTimes8:
	case Generic_Floor:
	case Generic_Ceiling:
	case Generic_Door:
	case Generic_Lift:
	case Generic_Stairs:
	case Generic_Crusher:
	case Plat_DownWaitUpStayLip:
	case Plat_PerpetualRaiseLip:
	case Stairs_BuildUpDoom:
	case Plat_RaiseAndStayTx0:
	case Plat_UpByValueStayTx:
	case Plat_ToggleCeiling:
	case Light_StrobeDoom:
	case Light_MinNeighbor:
	case Light_MaxNeighbor:
	case Floor_RaiseToLowestCeiling:
	case Floor_RaiseByValueTxTy:
	case Floor_RaiseByTexture:
	case Floor_LowerToLowestTxTy:
	case Floor_LowerToHighest:
	case Elevator_RaiseToNearest:
	case Elevator_MoveToFloor:
	case Elevator_LowerToNearest:
	case Door_CloseWaitOpen:
	case FloorAndCeiling_LowerRaise:
	case Ceiling_RaiseToNearest:
	case Ceiling_LowerToLowest:
	case Ceiling_LowerToFloor:
	case Ceiling_CrushRaiseAndStaySilA:
		return true;
	default:
		break;
	}
	return false;
}

void DMacroThinker::Tick()
{
	Super::Tick();

	// Handle Macro_Delay
	if (delaycounter)
	{
		delaycounter--;
		if (playerfreeze && !delaycounter)
		{
			players[consoleplayer].cheats &= ~CF_TOTALLYFROZEN;
			playerfreeze = false;
		}
		return;
	}

	if (started)
	{
		// Move to next sequence if needed
		if (currentstatus == DMacroThinker::done)
		{
			NextSequence();
			currentstatus = DMacroThinker::active;
		}

		// Find first special in current sequence if needed
		if (pos >= specials.Size() || specials[pos]->sequence != currentsequence)
			for (pos = 0; pos < specials.Size(); ++pos)
				if (specials[pos]->sequence == currentsequence)
					break;
	
		// if waiting, check for existing sector effects that still have to stop
		if (currentstatus == DMacroThinker::waiting)
		{
			for (size_t i = pos; i < specials.Size() && specials[i]->sequence == currentsequence; ++i)
			{
				if (IsSectorWaitSpecial(specials[i]->special))
				{
					int secnum = -1;

					FSectorTagIterator it (specials[i]->tag);
					while ((secnum = it.Next ()) >= 0)
					{
						if (sectors[secnum].floordata   || // Are all these different
							sectors[secnum].ceilingdata || // categories really needed?
							sectors[secnum].lightingdata|| // Should lighting be removed?
							0)
						{
							// Unfinished business, so no need to continue this tick
							return;
						}
					}
				}
			}
			// If we arrived there, then none of the tagged sectors had an ongoing effect, so we're done
			currentstatus = DMacroThinker::done;
		}

		// Process the current macro sequence
		if (currentstatus == DMacroThinker::active)
		{
			bool needwait = false;
			while (pos < specials.Size() && specials[pos]->sequence == currentsequence)
			{
				if (specials[pos]->special == Macro_Delay)
				{
					delaycounter = specials[pos]->tag;
					// Player freeze?
					if (specials[pos]->args[1])
					{
						playerfreeze = true;
						players[consoleplayer].cheats |= CF_TOTALLYFROZEN;
					}
				}
				else
				{
					LineSpecials[specials[pos]->special](line, actor, backside, specials[pos]->args[0],
						specials[pos]->args[1], specials[pos]->args[2], 
						specials[pos]->args[3], specials[pos]->args[4]);

					// Look if we need to bother waiting for more
					if (IsSectorWaitSpecial(specials[pos]->special)) needwait = true;
				}
				pos++;
			}
			// Have we reached the end? If so, stop
			if (pos == specials.Size()) started = false;
			if (needwait) currentstatus = DMacroThinker::waiting;
			else currentstatus = DMacroThinker::done;
		}
	}
}

// Advances to the next sequence
void DMacroThinker::NextSequence()
{
	for (size_t s = 0; s < specials.Size(); ++s)
	{
		if (specials[s]->sequence > currentsequence)
		{
			currentsequence = specials[s]->sequence;
			return;
		}
	}
}

bool DMacroThinker::Start(int tid, line_t * ln, AActor * it, bool back)
{
	if (!started)
	{
		started = true;
	}

	line = ln;
	actor = it;
	backside = back;

	// Todo: make something that works
	return true;
}

bool DMacroThinker::Pause(int tid, line_t * ln, AActor * it, bool back)
{
	started = false;
	line = ln;
	actor = it;
	backside = back;
	// Todo: make something that works
	return true;
}

bool DMacroThinker::Restart(int tid, line_t * ln, AActor * it, bool back)
{
	currentsequence = 0;
	return Start(tid, ln, it, back);
}

void DMacroThinker::AddMacro(macro_t * special)
{
	specials.Push(special);
}

IMPLEMENT_CLASS (DMacroManager)

TObjPtr<DMacroManager> DMacroManager::ActiveMacroManager;

DMacroManager::DMacroManager () : DThinker()
{
	if (ActiveMacroManager)
	{
		I_Error ("Only one MacroManager is allowed to exist at a time.\nCheck your code.");
	}
	else
	{
		ActiveMacroManager = this;
		macros.Clear();
		macros.ShrinkToFit();
	}
}

DMacroManager::~DMacroManager ()
{
	macros.Clear();
	macros.ShrinkToFit();
	if (ActiveMacroManager == this)
		ActiveMacroManager = NULL;
	// Todo: make something that works
}

void DMacroManager::Serialize(FArchive &arc)
{
	Super::Serialize(arc);
	// Todo: make something that works
}

void DMacroManager::Tick()
{
	Super::Tick();
	// Todo: make something that works
}

DMacroThinker * DMacroManager::GetMacro(size_t index)
{
	if (index < macros.Size())
		return macros[index];
	else return NULL;
}

void DMacroManager::AddMacroSequence(size_t index, macro_t * special)
{
	if (index > macros.Size())
	{
		// Allow for an empty macro, since apparently some are used
		if (index == macros.Size() + 1)
			macros.Push(new DMacroThinker());
		else
			I_Error("Missing macro numbers between %i and %i (this shouldn't happen)!\n", macros.Size(), index);
	}

	DMacroThinker * macro;
	if (index == macros.Size())
	{
		macro = new DMacroThinker();
		macros.Push(macro);
	}
	else macro = macros[index];

	if (macro != NULL)
		macro->AddMacro(special);
}




bool EV_Line_CopyFlag(int tag1, int tag2)
{
	// Let's use the first line we find as the model
	int im = P_FindFirstLineFromID(tag2);
	if (im > numlines || im < 0)
		return false;

	// Model flags
	DWORD newflags = lines[im].flags;

	// Now look for lines to change
	int linenum = -1;
	FLineIdIterator it (tag1);
	while ((linenum = it.Next ()) >= 0)
	{
		lines[linenum].flags = newflags;
	}
	return true;
}

bool EV_Line_CopyTexture(int tag1, int tag2)
{
	// Let's use the first line we find as the model
	int im = P_FindFirstLineFromID(tag2);
	if (im > numlines || im < 0)
		return false;

	// Model sides
	side_t * side0 = lines[im].sidedef[0];
	side_t * side1 = lines[im].sidedef[1];
	if (side1 == NULL) side1 = side0;
	if (side0 == NULL) return false;

	// Now look for lines to change
	int linenum = -1;
	FLineIdIterator it (tag1);
	while ((linenum = it.Next ()) >= 0)
	{
		side_t *sidedef;
		for (int i = 0; i < 2; ++i)
		{
			sidedef = lines[linenum].sidedef[i];
			if (sidedef == NULL)
				continue;

			sidedef->SetTexture(side_t::top,	(i?side1:side0)->GetTexture(side_t::top));
			sidedef->SetTexture(side_t::mid,	(i?side1:side0)->GetTexture(side_t::mid));
			sidedef->SetTexture(side_t::bottom,	(i?side1:side0)->GetTexture(side_t::bottom));
		}
	}
	return true;
}

bool EV_Sector_CopyFlag(int tag1, int tag2)
{
	// Let's use the first sector we find as the model
	int im = P_FindFirstSectorFromTag(tag2);
	if (im > numsectors || im < 0)
		return false;

	// Now look for sectors to change
	int secnum = -1;
	FSectorTagIterator it (tag1);
	while ((secnum = it.Next ()) >= 0)
	{
		DPrintf("Changing flags for sector number %i\n", secnum);
		sectors[secnum].Flags = sectors[im].Flags;
	}
	return true;
}

bool EV_Sector_CopySpecial(int tag1, int tag2)
{
	// Let's use the first sector we find as the model
	int im = P_FindFirstSectorFromTag(tag2);
	if (im > numsectors || im < 0)
		return false;

	// Now look for sectors to change
	int secnum = -1;
	FSectorTagIterator it (tag1);
	while ((secnum = it.Next ()) >= 0)
	{
		DPrintf("Changing special for sector number %i\n", secnum);
		if (!sectors[im].special)
		{
			sectors[secnum].special = sectors[im].oldspecial;
			P_SpawnSectorSpecial(&sectors[secnum]);
		}
		else sectors[secnum].special = sectors[im].special;
	}
	return true;
}

bool EV_Sector_CopyLight(int tag1, int tag2)
{
	// Let's use the first sector we find as the model
	int im = P_FindFirstSectorFromTag(tag2);
	if (im > numsectors || im < 0)
		return false;

	// Now look for sectors to change
	int secnum = -1;
	FSectorTagIterator it (tag1);
	while ((secnum = it.Next ()) >= 0)
	{
		DPrintf("Changing light for sector number %i\n", secnum);
		sectors[secnum].lightlevel = sectors[im].lightlevel;
		for (int i = LIGHT_GLOBAL; i < LIGHT_MAX; ++i)
			sectors[secnum].ColorMaps[i] = sectors[im].ColorMaps[i];
	}
	return true;
}

bool EV_Sector_CopyTexture(int tag1, int tag2)
{
	// Let's use the first sector we find as the model
	int im = P_FindFirstSectorFromTag(tag2);
	if (im > numsectors || im < 0)
		return false;

	// Now look for sectors to change
	int secnum = -1;
	FSectorTagIterator it (tag1);
	while ((secnum = it.Next ()) >= 0)
	{
		DPrintf("Changing textures for sector number %i\n", secnum);
		sectors[secnum].SetTexture(sector_t::floor, sectors[im].GetTexture(sector_t::floor));
		sectors[secnum].SetTexture(sector_t::ceiling, sectors[im].GetTexture(sector_t::ceiling));
	}
	return true;
}

bool EV_Sector_TransformLight(int tag1, int tag2)
{
	// Let's use the first sector we find as the model
	int im = P_FindFirstSectorFromTag(tag2);
	Printf("Using sector %i as the model for gradual light changes\n", im);
	if (im > numsectors || im < 0)
		return false;

	// Now look for sectors to change
	int secnum = -1;
	FSectorTagIterator it (tag1);
	while ((secnum = it.Next ()) >= 0)
	{
		Printf("Gradual transforms for sector %i\n", secnum);
		new DLightGradualTransform(&sectors[secnum], &sectors[im]);
	}
	return true;
}
