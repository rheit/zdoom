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

fixed_t DEarthquake::GetModCountdown(int countdown, bool up) const
{
	if (up)
		return m_CountdownStart - countdown;
	else
		return countdown;
}

fixed_t DEarthquake::GetModWave(fixed_t waveMultiplier, fixed_t time) const
{
	//QF_WAVE converts intensity into amplitude and unlocks a new property, the wave length.
	//This is, in short, waves per second (full cycles, mind you, from 0 to 360.)
	//Named waveMultiplier because that's as the name implies: adds more waves per second.
	fixed_t wavesPerSecond;
	fixed_t index;
	assert(m_CountdownStart >= m_Countdown);
	if (m_Flags & (QF_SCALEDOWN | QF_SCALEUP))
	{
		fixed_t scalar;
		if ((m_Flags & (QF_SCALEDOWN | QF_SCALEUP)) == (QF_SCALEDOWN | QF_SCALEUP))
		{
			scalar = (m_Flags & QF_MAX) ? MAX(time, m_CountdownStart - time)
				: MIN(time, m_CountdownStart - time);

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
		//assert(scalar > 0);
		//Need some help here...
		wavesPerSecond = (waveMultiplier >> 15) * time % (TICRATE * 2);
		index = ((wavesPerSecond * (FINEANGLES / 2)) / (TICRATE));
	}
	else
	{
		wavesPerSecond = (waveMultiplier >> 15) * time % (TICRATE * 2);
		index = ((wavesPerSecond * (FINEANGLES / 2)) / (TICRATE));
	}
	
	return finesine[index];
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
	bool &sineOriented, 
	fixed_t &mulWaveX, fixed_t &mulWaveY, fixed_t &mulWaveZ, 
	fixed_t &relmulWaveX, fixed_t &relmulWaveY, fixed_t &relmulWaveZ, int &countdown)
{
	if (victim->player != NULL && (victim->player->cheats & CF_NOCLIP))
	{
		return 0;
	}
	sineOriented = false;
	intensityX = intensityY = intensityZ = relIntensityX = relIntensityY = relIntensityZ = 
		mulWaveX = mulWaveY = mulWaveZ = relmulWaveX = relmulWaveY = relmulWaveZ = 
		countdown = 0;
	

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
				fixed_t x = quake->GetModIntensity(quake->m_IntensityX);
				fixed_t y = quake->GetModIntensity(quake->m_IntensityY);
				fixed_t z = quake->GetModIntensity(quake->m_IntensityZ);
				if (!sineOriented)
				{
					if (quake->m_Flags & QF_RELATIVE)
					{
						//relIntensityX = (x > 0) ? MAX(x, relIntensityX) : MIN(x, relIntensityX);
						//relIntensityY = (y > 0) ? MAX(y, relIntensityY) : MIN(y, relIntensityY);
						//relIntensityZ = (z > 0) ? MAX(z, relIntensityZ) : MIN(z, relIntensityZ);
						relIntensityX = MAX(x, relIntensityX);
						relIntensityY = MAX(y, relIntensityY);
						relIntensityZ = MAX(z, relIntensityZ);
					}
					else
					{
						//intensityX = (x > 0) ? MAX(x, intensityX) : MIN(x, intensityX);
						//intensityY = (y > 0) ? MAX(y, intensityY) : MIN(y, intensityY);
						//intensityZ = (z > 0) ? MAX(z, intensityZ) : MIN(z, intensityZ);
						intensityX = MAX(x, intensityX);
						intensityY = MAX(y, intensityY);
						intensityZ = MAX(z, intensityZ);
					}
				}
				if (sineOriented)
				{
					fixed_t time = quake->GetModCountdown(quake->m_Countdown, false);
					countdown = (time > countdown) ? time : countdown;
					fixed_t mx = quake->GetModWave(quake->m_mulWaveX, countdown);
					fixed_t my = quake->GetModWave(quake->m_mulWaveY, countdown);
					fixed_t mz = quake->GetModWave(quake->m_mulWaveZ, countdown);
					
					
					int mul = 16;
					int halfmul = mul / 2;
					int ls = 19, rs = 19;
					if (quake->m_Flags & QF_RELATIVE)
					{
						relmulWaveX = (relIntensityX) ? mx * (x >> ls) : mul * mx;
						relmulWaveY = (relIntensityY) ? my * (y >> ls) : mul * my;
						relmulWaveZ = (relIntensityZ) ? mz * (z >> ls) : mul * mz;
					}
					else
					{
						mulWaveX = (intensityX) ? mx * (x >> ls) : mul * mx;
						mulWaveY = (intensityY) ? my * (y >> ls) : mul * my;
						mulWaveZ = (intensityZ) ? mz * (z >> ls) : mul * mz;
					}
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
