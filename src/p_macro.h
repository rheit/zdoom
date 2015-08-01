/*
** p_macro.h
** Doom 64 script stuff
**
*/

#ifndef __P_MACRO_H__
#define __P_MACRO_H__

#include "dobject.h"
#include "dthinker.h"
#include "doomtype.h"
#include "g_level.h"
#include "p_lnspec.h"
#include "doomdata.h"
#include "r_defs.h"
#include "m_swap.h"
#include "p_spec.h"
#include "p_local.h"
#include "a_sharedglobal.h"
#include "gi.h"
#include "xlat/xlat.h"
#include "tarray.h"
#include "farchive.h"

struct macro_t
{
	int	special;
	int	args[5];
	int	sequence;
	int tag;
};

inline FArchive &operator<< (FArchive &arc, macro_t &spec)
{
	arc << spec.sequence << spec.tag << spec.special
		<< spec.args[0] << spec.args[1] << spec.args[2]
		<< spec.args[3] << spec.args[4];
	return arc;
}

class DMacroThinker : public DThinker
{
	DECLARE_CLASS (DMacroThinker, DThinker)
public:
	DMacroThinker ();
	~DMacroThinker ();

	void Serialize (FArchive &arc);
	void Tick ();
	bool Start(int tid, line_t * ln = NULL, AActor * it = NULL, bool back = false);
	bool Pause(int tid, line_t * ln = NULL, AActor * it = NULL, bool back = false);
	bool Restart(int tid, line_t * ln = NULL, AActor * it = NULL, bool back = false);
	void AddMacro(macro_t * special);
private:
	TArray<macro_t *> specials;
	int currentsequence;
	bool started;
	void NextSequence();
	enum MacroStatus
	{
		done = 0,
		active,
		waiting,
	};
	int currentstatus;
	line_t * line;
	AActor * actor;
	bool backside;
	size_t delaycounter;
	size_t pos;
	bool playerfreeze;
};

bool IsSectorWaitSpecial(int i);
//bool IsPolyWaitSpecial(int i);

class DMacroManager : public DThinker
{
	DECLARE_CLASS (DMacroManager, DThinker)
public:
	DMacroManager ();
	~DMacroManager ();

	void Serialize (FArchive &arc);
	void Tick ();
	static TObjPtr<DMacroManager> ActiveMacroManager;
	DMacroThinker * GetMacro(size_t index);
	void AddMacroSequence(size_t index, macro_t * special);
private:
	TArray<DMacroThinker *> macros;
};


// Not directly related to running macros, but these are usually
// used from within a macro so it's the best place for them anyway.

bool EV_Line_CopyFlag(int tag1, int tag2);
bool EV_Line_CopyTexture(int tag1, int tag2);
bool EV_Sector_CopyFlag(int tag1, int tag2);
bool EV_Sector_CopySpecial(int tag1, int tag2);
bool EV_Sector_CopyLight(int tag1, int tag2);
bool EV_Sector_CopyTexture(int tag1, int tag2);
bool EV_Sector_TransformLight(int tag1, int tag2);

#endif //__P_MACRO_H__
