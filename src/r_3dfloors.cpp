/*
**	r_3dfloors.cpp
**	software 3D floors addon
**
**	by kgsws
*/

#include "templates.h"
#include "doomdef.h"
#include "p_local.h"
#include "c_dispatch.h"
#include "r_local.h"
#include "r_bsp.h"
#include "r_plane.h"
#include "c_cvars.h"
#include "r_3dfloors.h"

// external variables
int fake3D;
F3DFloor *fakeFloor;
fixed_t fakeHeight;
fixed_t fakeAlpha;
int fakeActive = 0;
double sclipBottom;
double sclipTop;
HeightLevel *height_top = nullptr;
HeightLevel *height_cur = nullptr;
int CurrentMirror = 0;
int CurrentSkybox = 0;

CVAR(Int, r_3dfloors, true, 0);

// private variables
int height_max = -1;
TArray<HeightStack> toplist;
ClipStack *clip_top = nullptr;
ClipStack *clip_cur = nullptr;

void R_3D_DeleteHeights()
{
	height_cur = height_top;
	while(height_cur) {
		height_top = height_cur;
		height_cur = height_cur->next;
		M_Free(height_top);
	}
	height_max = -1;
	height_top = height_cur = nullptr;
}

void R_3D_AddHeight(secplane_t *add, sector_t *sec)
{
	HeightLevel *near;
	HeightLevel *curr;

	double height = add->ZatPoint(ViewPos);
	if(height >= sec->CenterCeiling()) return;
	if(height <= sec->CenterFloor()) return;

	fakeActive = 1;

	if(height_max >= 0) {
		near = height_top;
		while(near && near->height < height) near = near->next;
		if(near) {
			if(near->height == height) return;
			curr = (HeightLevel*)M_Malloc(sizeof(HeightLevel));
			curr->height = height;
			curr->prev = near->prev;
			curr->next = near;
			if(near->prev) near->prev->next = curr;
			else height_top = curr;
			near->prev = curr;
		} else {
			curr = (HeightLevel*)M_Malloc(sizeof(HeightLevel));
			curr->height = height;
			curr->prev = height_cur;
			curr->next = nullptr;
			height_cur->next = curr;
			height_cur = curr;
		}
	} else {
		height_top = height_cur = (HeightLevel*)M_Malloc(sizeof(HeightLevel));
		height_top->height = height;
		height_top->prev = nullptr;
		height_top->next = nullptr;
	}
	height_max++;
}

void R_3D_NewClip()
{
	ClipStack *curr;
//	extern short floorclip[MAXWIDTH];
//	extern short ceilingclip[MAXWIDTH];

	curr = (ClipStack*)M_Malloc(sizeof(ClipStack));
	curr->next = 0;
	memcpy(curr->floorclip, floorclip, sizeof(short) * MAXWIDTH);
	memcpy(curr->ceilingclip, ceilingclip, sizeof(short) * MAXWIDTH);
	curr->ffloor = fakeFloor;
	assert(fakeFloor->floorclip == nullptr);
	assert(fakeFloor->ceilingclip == nullptr);
	fakeFloor->floorclip = curr->floorclip;
	fakeFloor->ceilingclip = curr->ceilingclip;
	if(clip_top) {
		clip_cur->next = curr;
		clip_cur = curr;
	} else {
		clip_top = clip_cur = curr;
	}
}

void R_3D_ResetClip()
{
	clip_cur = clip_top;
	while(clip_cur) 
	{
		assert(clip_cur->ffloor->floorclip != nullptr);
		assert(clip_cur->ffloor->ceilingclip != nullptr);
		clip_cur->ffloor->ceilingclip = clip_cur->ffloor->floorclip = nullptr;
		clip_top = clip_cur;
		clip_cur = clip_cur->next;
		M_Free(clip_top);
	}
	clip_cur = clip_top = nullptr;
}

void R_3D_EnterSkybox()
{
	HeightStack current;

	current.height_top = height_top;
	current.height_cur = height_cur;
	current.height_max = height_max;

	toplist.Push(current);

	height_top = nullptr;
	height_cur = nullptr;
	height_max = -1;

	CurrentSkybox++;
}

void R_3D_LeaveSkybox()
{
	HeightStack current;

	current.height_top = nullptr;
	current.height_cur = nullptr;
	current.height_max = -1;

	toplist.Pop(current);

	height_top = current.height_top;
	height_cur = current.height_cur;
	height_max = current.height_max;

	CurrentSkybox--;
}

