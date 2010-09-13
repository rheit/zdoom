#include "p_macro.h"
#include "i_system.h"

IMPLEMENT_CLASS (DMacroThinker)

DMacroThinker::DMacroThinker() : DThinker(), currentsequence(0), currentstatus(0), started(false), 
								 delaycounter(0), pos(0), line(NULL), actor(NULL), backside(false)
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

	// Handle Macro_Delay special
	if (delaycounter)
	{
		delaycounter--;
		return;
	}

	if (started)
	{
		// Handle Macro_Delay
		if (delaycounter)
		{
			delaycounter--;
			return;
		}

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

					while ((secnum = P_FindSectorFromTag (specials[i]->tag, secnum)) >= 0)
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
					pos++;
					return;
				}
				else LineSpecials[specials[pos]->special](line, actor, backside, specials[pos]->args[0],
					specials[pos]->args[1], specials[pos]->args[2], specials[pos]->args[3], specials[pos]->args[4]);

				// Look if we need to bother waiting for more
				if (IsSectorWaitSpecial(specials[pos]->special)) needwait = true;

				// Have we reached the end? If so, stop
				pos++;
				if (pos == specials.Size())
					started = false;
			}
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
