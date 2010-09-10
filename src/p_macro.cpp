#include "p_macro.h"
#include "i_system.h"

IMPLEMENT_CLASS (DMacroThinker)

DMacroThinker::DMacroThinker() : DThinker(), currentsequence(0)
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

void DMacroThinker::Tick()
{
	Super::Tick();
	// Todo: make something that works
}

bool DMacroThinker::Start(int tid, line_t * ln, AActor * it, bool backside)
{
	Printf("Macro started! (Well, except that it doesn't work.)\n");

	// This is not how macros actually work, but just a temporary workaround
	size_t s = 0;
	while (s < specials.Size())
	{
		if (specials[s]->sequence > currentsequence)
			break;
		s++;
	}
	currentsequence = specials[s]->sequence;
	do
	{
		LineSpecials[specials[s]->special](ln, it, backside, specials[s]->args[0],
			specials[s]->args[1], specials[s]->args[2], specials[s]->args[3], specials[s]->args[5]);
		s++;
	} while (s < specials.Size() && specials[s]->sequence == currentsequence);

	// Todo: make something that works
	return true;
}

bool DMacroThinker::Pause(int tid, line_t * ln, AActor * it, bool backside)
{
	// Todo: make something that works
	return true;
}

bool DMacroThinker::Restart(int tid, line_t * ln, AActor * it, bool backside)
{
	// Todo: make something that works
	return true;
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
		I_Error("Missing macro numbers between %i and %i (this shouldn't happen)!\n", macros.Size(), index);

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
