#include "templates.h"
#include "doomtype.h"
#include "doomstat.h"
#include "p_local.h"
#include "actor.h"
#include "m_bbox.h"
#include "m_random.h"
#include "s_sound.h"
#include "a_sharedglobal.h"
#include "statnums.h"
#include "farchive.h"

static FRandom pr_quake ("Quake");

IMPLEMENT_POINTY_CLASS (DEarthquake)
 DECLARE_POINTER (m_Spot)
END_POINTERS

//==========================================================================
//
// DEarthquake :: DEarthquake private constructor
//
//==========================================================================

DEarthquake::DEarthquake()
: DThinker(STAT_EARTHQUAKE)
{
}

//==========================================================================
//
// DEarthquake :: DEarthquake public constructor
//
//==========================================================================

DEarthquake::DEarthquake (AActor *center, int intensityX, int intensityY, int intensityZ, int duration,
						  int damrad, int tremrad, FSoundID quakesound, int flags, 
						  double mulWaveX, double mulWaveY, double mulWaveZ)
						  : DThinker(STAT_EARTHQUAKE)
{
	m_QuakeSFX = quakesound;
	m_Spot = center;
	// Radii are specified in tile units (64 pixels)
	m_DamageRadius = damrad << (FRACBITS);
	m_TremorRadius = tremrad << (FRACBITS);
	m_IntensityX = intensityX;
	m_IntensityY = intensityY;
	m_IntensityZ = intensityZ;
	m_CountdownStart = duration;
	m_Countdown = duration;
	m_Flags = flags;
	m_mulWaveX = FLOAT2FIXED(mulWaveX);
	m_mulWaveY = FLOAT2FIXED(mulWaveY);
	m_mulWaveZ = FLOAT2FIXED(mulWaveZ);
}

//==========================================================================
//
// DEarthquake :: Serialize
//
//==========================================================================

void DEarthquake::Serialize (FArchive &arc)
{
	Super::Serialize (arc);
	arc << m_Spot << m_IntensityX << m_Countdown
		<< m_TremorRadius << m_DamageRadius
		<< m_QuakeSFX;
	if (SaveVersion < 4519)
	{
		m_IntensityY = m_IntensityX;
		m_IntensityZ = 0;
		m_Flags = 0;
	}
	else
	{
		arc << m_IntensityY << m_IntensityZ << m_Flags;
	}
	if (SaveVersion < 4520)
	{
		m_CountdownStart = 0;
	}
	else
	{
		arc << m_CountdownStart;
	}
	if (SaveVersion < 4521)
	{
		m_mulWaveX = m_mulWaveY = m_mulWaveZ = 0;
	}
	else
	{
		arc << m_mulWaveX << m_mulWaveY << m_mulWaveZ;
	}
}

//==========================================================================
//
// DEarthquake :: Tick
//
// Deals damage to any players near the earthquake and makes sure it's
// making noise.
//
//==========================================================================

void DEarthquake::Tick ()
{
	int i;

	if (m_Spot == NULL)
	{
		Destroy ();
		return;
	}
	
	if (!S_IsActorPlayingSomething (m_Spot, CHAN_BODY, m_QuakeSFX))
	{
		S_Sound (m_Spot, CHAN_BODY | CHAN_LOOP, m_QuakeSFX, 1, ATTN_NORM);
	}
	if (m_DamageRadius > 0)
	{
		for (i = 0; i < MAXPLAYERS; i++)
		{
			if (playeringame[i] && !(players[i].cheats & CF_NOCLIP))
			{
				AActor *victim = players[i].mo;
				fixed_t dist;

				dist = P_AproxDistance (victim->x - m_Spot->x, victim->y - m_Spot->y);
				// Check if in damage radius
				if (dist < m_DamageRadius && victim->z <= victim->floorz)
				{
					if (pr_quake() < 50)
					{
						P_DamageMobj (victim, NULL, NULL, pr_quake.HitDice (1), NAME_None);
					}
					// Thrust player around
					angle_t an = victim->angle + ANGLE_1*pr_quake();
					if (m_IntensityX == m_IntensityY)
					{ // Thrust in a circle
						P_ThrustMobj (victim, an, m_IntensityX << (FRACBITS-1));
					}
					else
					{ // Thrust in an ellipse
						an >>= ANGLETOFINESHIFT;
						// So this is actually completely wrong, but it ought to be good
						// enough. Otherwise, I'd have to use tangents and square roots.
						victim->velx += FixedMul(m_IntensityX << (FRACBITS-1), finecosine[an]);
						victim->vely += FixedMul(m_IntensityY << (FRACBITS-1), finesine[an]);
					}
				}
			}
		}
	}
	
	if (--m_Countdown == 0)
	{
		if (S_IsActorPlayingSomething(m_Spot, CHAN_BODY, m_QuakeSFX))
		{
			S_StopSound(m_Spot, CHAN_BODY);
		}
		Destroy();
	}
}

fixed_t DEarthquake::GetModWave(fixed_t intensity, fixed_t waveMultiplier) const
{
	intensity += intensity;
	//QF_WAVE converts intensity into amplitude and unlocks a new property, the wave length.
	//This is, in short, waves per second (full cycles, mind you, from 0 to 360.)
	//Named waveMultiplier because that's as the name implies: adds more waves per second.

	fixed_t wavesPerSecond = (waveMultiplier >> 15) * ((m_CountdownStart - m_Countdown)) % (TICRATE * 2);
	fixed_t index = (wavesPerSecond * (FINEANGLES / 2)) / (TICRATE);

	intensity = intensity * finesine[index];
	return intensity;
}

//==========================================================================
//
// DEarthquake :: GetModIntensity
//
// Given a base intensity, modify it according to the quake's flags.
//
//==========================================================================

fixed_t DEarthquake::GetModIntensity(fixed_t intensity) const
{
	assert(m_CountdownStart >= m_Countdown);
	intensity += intensity;		// always doubled

	if (m_Flags & (QF_SCALEDOWN | QF_SCALEUP))
	{
		int scalar;
		if ((m_Flags & (QF_SCALEDOWN | QF_SCALEUP)) == (QF_SCALEDOWN | QF_SCALEUP))
		{
			scalar = (m_Flags & QF_MAX) ? MAX(m_Countdown, m_CountdownStart - m_Countdown)
				: MIN(m_Countdown, m_CountdownStart - m_Countdown);

			if (m_Flags & QF_FULLINTENSITY)
			{
				scalar *= 2;
			}
		}
		else if (m_Flags & QF_SCALEDOWN)
		{
			scalar = m_Countdown;
		}
		else			// QF_SCALEUP
		{
			scalar = m_CountdownStart - m_Countdown;
		}
		assert(m_CountdownStart > 0);
		intensity = intensity * (scalar << FRACBITS) / m_CountdownStart;
	}
	return intensity;
}

//==========================================================================
//
// DEarthquake::StaticGetQuakeIntensity
//
// Searches for all quakes near the victim and returns their combined
// intensity.
//
//==========================================================================

int DEarthquake::StaticGetQuakeIntensities(AActor *victim,
	fixed_t &intensityX, fixed_t &intensityY, fixed_t &intensityZ,
	fixed_t &relIntensityX, fixed_t &relIntensityY, fixed_t &relIntensityZ, 
	bool &sineOriented, fixed_t &mulWaveX, fixed_t &mulWaveY, fixed_t &mulWaveZ)
{
	if (victim->player != NULL && (victim->player->cheats & CF_NOCLIP))
	{
		return 0;
	}
	sineOriented = false;
	intensityX = intensityY = intensityZ = relIntensityX = relIntensityY = relIntensityZ = 
		mulWaveX = mulWaveY = mulWaveZ = 0;
	

	TThinkerIterator<DEarthquake> iterator(STAT_EARTHQUAKE);
	DEarthquake *quake;
	int count = 0;

	while ( (quake = iterator.Next()) != NULL)
	{
		if (quake->m_Spot != NULL)
		{
			fixed_t dist = P_AproxDistance (victim->x - quake->m_Spot->x,
				victim->y - quake->m_Spot->y);
			if (dist < quake->m_TremorRadius)
			{
				++count;
				sineOriented = (quake->m_Flags & QF_WAVE) ? true : false;
				//fixed_t x = sineOriented ? quake->GetModWave(quake->m_IntensityX, quake->m_mulWaveX) : quake->GetModIntensity(quake->m_IntensityX);
				//fixed_t y = sineOriented ? quake->GetModWave(quake->m_IntensityY, quake->m_mulWaveY) : quake->GetModIntensity(quake->m_IntensityY);
				//fixed_t z = sineOriented ? quake->GetModWave(quake->m_IntensityZ, quake->m_mulWaveZ) : quake->GetModIntensity(quake->m_IntensityZ);
				fixed_t x = quake->GetModIntensity(quake->m_IntensityX);
				fixed_t y = quake->GetModIntensity(quake->m_IntensityY);
				fixed_t z = quake->GetModIntensity(quake->m_IntensityZ);

				if (quake->m_Flags & QF_RELATIVE)
				{
					relIntensityX = (x > 0) ? MAX(x, relIntensityX) : MIN(x, relIntensityX);
					relIntensityY = (y > 0) ? MAX(y, relIntensityY) : MIN(y, relIntensityY);
					relIntensityZ = (z > 0) ? MAX(z, relIntensityZ) : MIN(z, relIntensityZ);
				}
				else
				{
					intensityX = (x > 0) ? MAX(x, intensityX) : MIN(x, intensityX);
					intensityY = (y > 0) ? MAX(y, intensityY) : MIN(y, intensityY);
					intensityZ = (z > 0) ? MAX(z, intensityZ) : MIN(z, intensityZ);
				}
				if (sineOriented)
				{
					x = quake->GetModWave(quake->m_IntensityX, quake->m_mulWaveX); //Actually returns a little differently.
					y = quake->GetModWave(quake->m_IntensityY, quake->m_mulWaveY);
					z = quake->GetModWave(quake->m_IntensityZ, quake->m_mulWaveZ);
					mulWaveX = x;//(x > 0) ? MAX(x, quake->m_mulWaveX) : MIN(x, quake->m_mulWaveX);
					mulWaveY = y;//(y > 0) ? MAX(y, quake->m_mulWaveY) : MIN(y, quake->m_mulWaveY);
					mulWaveZ = z;//(z > 0) ? MAX(z, quake->m_mulWaveZ) : MIN(z, quake->m_mulWaveZ);
				}
			}
		}
	}
	return count;
}

//==========================================================================
//
// P_StartQuake
//
//==========================================================================

bool P_StartQuakeXYZ(AActor *activator, int tid, int intensityX, int intensityY, int intensityZ, int duration, int damrad, int tremrad, FSoundID quakesfx, int flags,
	double mulWaveX, double mulWaveY, double mulWaveZ)
{
	AActor *center;
	bool res = false;

	if (intensityX)		intensityX = clamp(intensityX, 1, 9);
	if (intensityY)		intensityY = clamp(intensityY, 1, 9);
	if (intensityZ)		intensityZ = clamp(intensityZ, 1, 9);

	if (tid == 0)
	{
		if (activator != NULL)
		{
			new DEarthquake(activator, intensityX, intensityY, intensityZ, duration, damrad, tremrad, quakesfx, flags, mulWaveX, mulWaveY, mulWaveZ);
			return true;
		}
	}
	else
	{
		FActorIterator iterator (tid);
		while ( (center = iterator.Next ()) )
		{
			res = true;
			new DEarthquake(center, intensityX, intensityY, intensityZ, duration, damrad, tremrad, quakesfx, flags, mulWaveX, mulWaveY, mulWaveZ);
		}
	}
	
	return res;
}

bool P_StartQuake(AActor *activator, int tid, int intensity, int duration, int damrad, int tremrad, FSoundID quakesfx)
{	//Maintains original behavior by passing 0 to intensityZ, and flags.
	return P_StartQuakeXYZ(activator, tid, intensity, intensity, 0, duration, damrad, tremrad, quakesfx, 0, 0, 0, 0);
}
