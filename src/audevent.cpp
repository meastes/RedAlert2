// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright 2026 ra2-decomp contributors
// Copyright 2025 Electronic Arts Inc.
// Derived from Electronic Arts source code; EA's GPL Section 7 terms apply (COPYING.md).
#include "always.h" // RA2: the includes this file needs
#include "coord.h" // RA2: the includes this file needs
#include <stddef.h> // RA2: the includes this file needs
#include <string.h> // RA2: the includes this file needs
#include <stdio.h> // RA2: the includes this file needs
#include "random.h" // RA2: the includes this file needs
#include "_rand.h" // RA2: the includes this file needs
#include "voc.h" // RA2: the includes this file needs
#include "audevent.h" // RA2: the includes this file needs
#include "audioctl.h" // RA2: the includes this file needs
#include "audcache.h" // RA2: the includes this file needs
#include "auddev.h" // RA2: the includes this file needs
#include "audmem.h" // RA2: the includes this file needs

extern AudioAttribs * AudioSoundAttribs; // RA2: audio.cpp, the sound effects' group level
extern AudioAttribs * AudioGroupAttribs1; // RA2: audio.cpp
extern int StaticSoundCount; // RA2: 0x00AD0848; voc.cpp, the static sounds playing now
extern int StaticSoundMaximum; // RA2: 0x00AD084C; voc.cpp, the most static sounds at once

void __cdecl DebugString(char const * fmt, ...); // RA2: 0x004068F0; the release executable keeps the call and the format strings

static void eventWait(AudioEvent * event, TimeStamp delay, int nextState);

struct EClassBucket // RA2: one priority bucket: 10 lists for the volumes (0x00830B58 is 7 of them, 0x348 bytes)
{
	ListHead bucketVol[NUM_BUCKETS_VOL]; // RA2: as above
};

struct EventKeyword // RA2: a keyword and its value, in the tables at 0x007CCAE8, 0x007CCB18 and 0x007CCB90 (8 bytes each, ended by a NULL name)
{
	char const * Name; // RA2: as above
	int Value; // RA2: as above
};

static EventKeyword PriorityKeywords[] = { // RA2: 0x007CCAE8
	{"LOWEST", 0},
	{"LOW", 1},
	{"NORMAL", 2},
	{"HIGH", 3},
	{"CRITICAL", 4},
	{NULL, 2}
};

static EventKeyword TypeKeywords[] = { // RA2: 0x007CCB18
	{"AMBIENT", 0x1000},
	{"VIOLENT", 0x1},
	{"MOVEMENT", 0x2},
	{"QUIET", 0x4},
	{"LOUD", 0x8},
	{"GLOBAL", 0x10},
	{"SCREEN", 0x20},
	{"LOCAL", 0x40},
	{"PLAYER", 0x80},
	{"NORMAL", 0x0},
	{"GUN_SHY", 0x200},
	{"NOISE_SHY", 0x100},
	{"UNSHROUD", 0x400},
	{"SHROUD", 0x800},
	{NULL, 0}
};

static EventKeyword ControlKeywords[] = { // RA2: 0x007CCB90
	{"ALL", 0x4},
	{"LOOP", 0x1},
	{"RANDOM", 0x2},
	{"PREDELAY", 0x8},
	{"INTERRUPT", 0x10},
	{"ATTACK", 0x20},
	{"DECAY", 0x40},
	{"AMBIENT", 0x80},
	{NULL, 0}
};

static EClassBucket eClassBucket[NUM_BUCKETS_PRI]; // RA2: 0x00830B58
static ListHead audioEvents; // RA2: 0x00830EA0; every event, newest first
static AudioAttribs audioCompressionAttribs; // RA2: 0x00830ED8
static ListHead audioEventClasses; // RA2: 0x00830F70; every event type
int AudioEventsCount; // RA2: 0x00830FA4
int AudioEventsPeak; // RA2: 0x00830FA8
static AudioCache * audioCache; // RA2: 0x00830FB0
static AudioDevice * audioDevice; // RA2: 0x00830FB4
static int audioEventsReady; // RA2: 0x00830FB8; set by SetUp and cleared by CloseDown; (the YR decompilation calls it "initialized")
static int eventsOK; // RA2: 0x00830FBC
static AudioMemoryPool * audioEventPool; // RA2: 0x00830FC0
static int frameStamp; // RA2: 0x00830FC4
static unsigned bucketStamp; // RA2: 0x00830FC8
static int service_in; // RA2: 0x00830FCC
static int eventsOn = 1; // RA2: 0x007CCBD8
static int audioStamp = AUDIO_INVALID_STAMP + 1; // RA2: 0x007CCBDC

static inline int AudioRandomPick(int lo, int hi) // RA2: a static helper, expanded into its callers (no copy of its own in the executable)
{
	return NonCriticalRandomNumber(lo, hi); // RA2: as above
}

static void eventLoadItem(AudioEvent * event, int sampleIndex) // RA2: a static helper, expanded into its callers (no copy of its own in the executable)
{
	if ((event->item[event->numItems] = AudioCacheLoadItem(audioCache, event->eclass->SampleIndex[sampleIndex]))) { // RA2: as above
		AudioCacheItemLock(event->item[event->numItems]); // RA2: as above
		event->numItems++; // RA2: as above
	}
}

/// <summary>
/// Starts the event layer on a device and the sample bag: empty lists, the sample cache and the event pool.
/// </summary>
int __fastcall AudioEventSetUp(AudioDevice * device, AudioIDXData * idx) // RA2: 0x00403EB0
{
	ListInit(&audioEvents); // RA2: as above
	ListInit(&audioEventClasses); // RA2: as above
	AudioIDXData::Instance = idx; // RA2: as above
	audioDevice = device; // RA2: as above
	AudioAttribsInit(&audioCompressionAttribs); // RA2: as above
	AudioEventsPeak = 0; // RA2: as above
	AudioEventsCount = 0; // RA2: as above
	audioEventsReady = 1; // RA2: as above
	eventsOn = 1; // RA2: as above
	if (AudioIDXData::Instance) { // RA2: as above
		audioCache = AudioCacheCreate(AudioIDXData::Instance, 0x100000, 200, 0x2000); // RA2: a 1 MB cache of 200 items with 8 KB frames (edx 0x100000, push 200, push 8192 at 0x00403F09)
		audioEventPool = MemoryPoolCreate(AUDIO_MAX_EVENTS, sizeof(AudioEvent)); // RA2: 100 events of 640 bytes (ecx 100, edx 640 at 0x00403F1D)
		DebugString("event mempool size = %d\n", AUDIO_MAX_EVENTS * sizeof(AudioEvent)); // RA2: pushes 64000, the pool size, and a format string of the executable (0x00403F36)
	}
	if (audioCache && AudioIDXData::Instance && audioDevice && audioEventPool) { // RA2: as above
		eventsOK = 1; // RA2: as above
		return 1; // RA2: as above
	}
	return 0; // RA2: as above
}

static void eventDestroyAllClasses(void) // RA2: a static helper, expanded into its callers (no copy of its own in the executable)
{
	AudioEventType * eclass; // RA2: as above
	while ((eclass = (AudioEventType *)ListFirstItem(&audioEventClasses))) { // RA2: as above
		ListNodeRemove(&eclass->Header); // RA2: as above
		delete eclass; // RA2: as above
	}
}

/// <summary>
/// Stops every event, drops the event types and frees the sample cache and the event pool.
/// </summary>
void AudioEventCloseDown(void) // RA2: 0x00403F80
{
	AudioKillAllEvents(); // RA2: as above
	AudioFlushAllDeadEvents(); // RA2: as above

	eventDestroyAllClasses(); // RA2: as above
	if (audioCache) { // RA2: as above
		AudioCacheDestroy(audioCache); // RA2: as above
		audioCache = NULL; // RA2: as above
	}
	if (audioEventPool) { // RA2: as above
		MemoryPoolDestroy(audioEventPool); // RA2: as above
		audioEventPool = NULL; // RA2: as above
	}
	eventsOK = 0; // RA2: as above
	audioEventsReady = 0; // RA2: as above
}

/// <summary>
/// Lets go of the channel an event plays on: stops it if it plays and clears the channel's event callbacks.
/// </summary>
void __fastcall eventReleaseChannel(AudioEvent * event) // RA2: 0x00404120
{
	if (event->channel && event == (AudioEvent *)event->channel->Data) { // RA2: as above
		if (event->flags & mAUDIO_EVENT_PLAYING) { // RA2: as above
			AudioChannelStop(event->channel); // RA2: as above
		}
		AudioChannelNoUse(event->channel); // RA2: as above
		event->channel->Data = NULL; // RA2: as above
		event->channel->CB_Stop = NULL; // RA2: as above
		event->channel->CB_NextSample = NULL; // RA2: as above
		event->channel->CB_SampleDone = NULL; // RA2: as above
		event->channel->SfxAttribs = NULL; // RA2: as above
		event->channel->CompAttribs = NULL; // RA2: as above
		event->channel->FadeAttribs = NULL; // RA2: as above
		event->channel = NULL; // RA2: as above
	}
}

static void eventAddLocalSort(AudioEvent * newEvent) // RA2: a static helper, expanded into its callers (no copy of its own in the executable)
{
	AudioEventType * eclass; // RA2: as above
	ListNode * node, * head; // RA2: as above
	AudioEvent * event; // RA2: as above
	unsigned volume, newVolume; // RA2: as above

	eclass = newEvent->eclass; // RA2: as above
	newVolume = AudioAttribsGetVolume(&newEvent->attribs); // RA2: as above
	head = &eclass->local; // RA2: as above
	node = head->next; // RA2: as above

	while (node != head) { // RA2: as above
		int dif, slack; // RA2: as above
		unsigned baseLev; // RA2: as above
		event = (AudioEvent *)((char *)node - offsetof(AudioEvent, local)); // RA2: as above
		volume = AudioAttribsGetVolume(&event->attribs); // RA2: as above
		baseLev = (unsigned)eclass->Volume.level >> AUDIO_LEVEL_SCALE; // RA2: as above
		slack = baseLev / NUM_BUCKETS_VOL; // RA2: as above
		dif = volume - newVolume; // RA2: as above
		if ((dif >= 0 ? dif : -dif) < slack) { // RA2: as above
			if (eclass->Control & AUDIO_EVENT_CTRL_INTERRUPT) { // RA2: as above
				if (event->state != AUDIO_EVENT_NEW) { // RA2: as above
					break; // RA2: as above
				}
			} else { // RA2: as above
				if (event->state == AUDIO_EVENT_NEW) { // RA2: as above
					break; // RA2: as above
				}
			}
		} else if (volume < newVolume) { // RA2: as above
			break; // RA2: as above
		}
		node = node->next; // RA2: as above
	}

	ListNodeInit(&newEvent->local); // RA2: as above
	ListNodeInsert(node, &newEvent->local); // RA2: as above
	eclass->Count++; // RA2: as above
	if (eclass->Limit && (eclass->Count > eclass->Limit)) { // RA2: as above
		event = (AudioEvent *)((char *)head->prev - offsetof(AudioEvent, local)); // RA2: as above
		ListNodeRemove(&event->local); // RA2: as above
		AudioEventStop(event); // RA2: as above
		eclass->Count--; // RA2: as above
	}
}

static void bucketReset(void) // RA2: a static helper, expanded into its callers (no copy of its own in the executable)
{
	int i, k; // RA2: as above
	EClassBucket * ecb; // RA2: as above
	for (i = 0, ecb = eClassBucket; i < NUM_BUCKETS_PRI; i++, ecb++) { // RA2: as above
		for (k = 0; k < NUM_BUCKETS_VOL; k++) { // RA2: as above
			ListInit(&ecb->bucketVol[k]); // RA2: as above
		}
	}
	bucketStamp++; // RA2: as above
}

static void bucketAdd(AudioEvent * event) // RA2: a static helper, expanded into its callers (no copy of its own in the executable)
{
	EClassBucket * ecb; // RA2: as above
	AudioEventType * eclass; // RA2: as above
	int pri; // RA2: as above
	unsigned vol; // RA2: as above

	eclass = event->eclass; // RA2: as above
	pri = eclass->Priority + event->adjustPriority; // RA2: as above
	vol = AudioAttribsGetVolume(&event->attribs) / VOLUME_QUANTIZE; // RA2: as above
	ecb = &eClassBucket[pri]; // RA2: as above

	if (eclass->lastBucketStamp == bucketStamp) { // RA2: as above
		if (pri > eclass->maxPri) { // RA2: as above
			eclass->maxPri = pri; // RA2: as above
			eclass->maxVol = vol; // RA2: as above
			ListNodeRemove(&eclass->nodeVol); // RA2: as above
			ListNodeInsert(&ecb->bucketVol[vol], &eclass->nodeVol); // RA2: as above
		} else if (pri == eclass->maxPri) { // RA2: as above
			if (vol > eclass->maxVol) { // RA2: as above
				eclass->maxVol = vol; // RA2: as above
				ListNodeRemove(&eclass->nodeVol); // RA2: as above
				ListNodeInsert(&ecb->bucketVol[vol], &eclass->nodeVol); // RA2: as above
			}
		}
	} else { // RA2: as above
		eclass->lastBucketStamp = bucketStamp; // RA2: as above
		eclass->maxPri = pri; // RA2: as above
		eclass->maxVol = vol; // RA2: as above
		ListNodeInit(&eclass->nodeVol); // RA2: as above
		ListNodeInsert(&ecb->bucketVol[vol], &eclass->nodeVol); // RA2: as above
	}
}

static int eventClassSacrifice(AudioEvent * newEvent) // RA2: a static helper, expanded into its callers (no copy of its own in the executable)
{
	AudioEvent * event, * next, * head; // RA2: as above
	AudioEventType * eclassSacrifice = NULL; // RA2: as above
	int killed = 0; // RA2: as above
	eclassSacrifice = bucketGetWeaker(newEvent->eclass->Priority + newEvent->adjustPriority); // RA2: as above
	if (eclassSacrifice == NULL) { // RA2: as above
		return 0; // RA2: as above
	}
	ListNodeRemove(&eclassSacrifice->nodeVol); // RA2: as above
	head = (AudioEvent *)&audioEvents; // RA2: as above
	event = (AudioEvent *)head->nd.next; // RA2: as above
	while (event != head) { // RA2: as above
		next = (AudioEvent *)event->nd.next; // RA2: as above
		if (event->eclass == eclassSacrifice) { // RA2: as above
			AudioEventStop(event); // RA2: as above
			audioEventDestroy(event); // RA2: as above
			killed = 1; // RA2: as above
		}
		event = next; // RA2: as above
	}
	return killed; // RA2: as above
}

/// <summary>
/// The once-a-frame service of the event layer: sorts each type's events by volume and enforces its limit, updates
/// every event, drops the dead ones, sets the volume compression for the loudness and starts the events waiting for a channel.
/// </summary>
void AudioServiceAllEvents(void) // RA2: 0x004041B0
{
	AudioEvent * event, * next, * head; // RA2: as above
	enum { AVERAGE_LOUDNESS = 20, FULL_LOUDNESS = 100 }; // RA2: as above
	int loudness; // RA2: as above
	int processedEvents = 0; // RA2: as above

	if (!audioEventsReady || service_in) { // RA2: as above
		return; // RA2: as above
	}
	service_in = 1; // RA2: as above
	frameStamp++; // RA2: as above
	head = (AudioEvent *)&audioEvents; // RA2: as above

	event = (AudioEvent *)head->nd.next; // RA2: as above
	while (event != head) { // RA2: as above
		if (event->eclass->lastFrame != frameStamp) { // RA2: as above
			ListInit(&event->eclass->local); // RA2: as above
			event->eclass->Count = 0; // RA2: as above
			event->eclass->lastFrame = frameStamp; // RA2: as above
		}
		AudioAttribsUpdate(&event->attribs); // RA2: as above
		if (event->state != AUDIO_EVENT_DONE) { // RA2: as above
			eventAddLocalSort(event); // RA2: as above
		}
		event = (AudioEvent *)event->nd.next; // RA2: as above
	}

	event = (AudioEvent *)head->nd.next; // RA2: as above
	while (event != head) { // RA2: as above
		if (event->state != AUDIO_EVENT_DONE) { // RA2: as above
			processedEvents++; // RA2: as above
		}
		AudioEventService(event); // RA2: as above
		event = (AudioEvent *)event->nd.next; // RA2: as above
	}

	event = (AudioEvent *)head->nd.next; // RA2: as above
	loudness = 0; // RA2: as above
	bucketReset(); // RA2: as above
	while (event != head) { // RA2: as above
		next = (AudioEvent *)event->nd.next; // RA2: as above
		if (!event->channel || event != (AudioEvent *)event->channel->Data) { // RA2: as above
			AudioEventStop(event); // RA2: as above
		}
		if (event->flags & mAUDIO_EVENT_DEAD) { // RA2: as above
			audioEventDestroy(event); // RA2: as above
		} else { // RA2: as above
			if (!AudioEventIsPaused(event)) { // RA2: as above
				loudness += AUDIO_LEVEL_APPLY(&event->eclass->Volume, AVERAGE_LOUDNESS); // RA2: as above
			}
			bucketAdd(event); // RA2: as above
		}
		event = next; // RA2: as above
	}

	if (loudness > FULL_LOUDNESS) { // RA2: as above
		AudioLevelAdjust(&audioCompressionAttribs.VolumeLevel, (unsigned)(AUDIO_LEVEL_MAX * FULL_LOUDNESS) / loudness); // RA2: as above
	} else { // RA2: as above
		AudioLevelAdjust(&audioCompressionAttribs.VolumeLevel, AUDIO_LEVEL_MAX); // RA2: as above
	}
	AudioAttribsUpdate(&audioCompressionAttribs); // RA2: as above

	event = (AudioEvent *)head->nd.next; // RA2: as above
	while (event != head) { // RA2: as above
		next = (AudioEvent *)event->nd.next; // RA2: as above
		if (event->state == AUDIO_EVENT_START_PLAYING && !AudioEventIsPaused(event)) { // RA2: as above
			eventLoadAndLockSamples(event); // RA2: as above
			while (!(event->startSample = eventFirstSample(event))) { // RA2: as above
				if (eventClassSacrifice(event)) { // RA2: as above
					next = (AudioEvent *)event->nd.next; // RA2: as above
					eventLoadAndLockSamples(event); // RA2: as above
				} else { // RA2: as above
					break; // RA2: as above
				}
			}
			if (!(event->startSample = eventFirstSample(event)) || !audioEventStart(event)) { // RA2: as above
				AudioEventStop(event); // RA2: as above
				audioEventDestroy(event); // RA2: as above
			}
		}
		event = next; // RA2: as above
	}

	service_in = 0; // RA2: as above
}

/// <summary>
/// The first sample an event plays: sets up the order of the samples between attack and decay and returns the attack
/// sample, or the next one when the event has no attack.
/// </summary>
AudioSample * __fastcall eventFirstSample(AudioEvent * event) // RA2: 0x004046E0
{
	int j = 0; // RA2: as above
	int i; // RA2: as above

	if ((event->numSequence = event->numItems) == 0 || event->flags & mAUDIO_EVENT_END) { // RA2: as above
		return NULL; // RA2: as above
	}
	if (event->eclass->MinDelay < EVENT_MIN_DELAY) { // RA2: as above
		if (event->eclass->Control & AUDIO_EVENT_CTRL_ATTACK) { // RA2: as above
			j++; // RA2: as above
			event->numSequence--; // RA2: as above
			event->currentItem = 0; // RA2: as above
		} else { // RA2: as above
			event->currentItem = -1; // RA2: as above
		}
		if (event->eclass->Control & AUDIO_EVENT_CTRL_DECAY) { // RA2: as above
			event->numSequence--; // RA2: as above
		}
		if (event->numSequence <= 0) { // RA2: as above
			event->eclass->SamplesOK = 0; // RA2: as above
			return NULL; // RA2: as above
		}
		for (i = 0; i < event->numSequence; i++, j++) { // RA2: as above
			event->sequence[i] = j; // RA2: as above
		}
		if (event->flags & mAUDIO_EVENT_NO_ATTACK || !(event->eclass->Control & AUDIO_EVENT_CTRL_ATTACK)) { // RA2: as above
			return eventNextSample(event); // RA2: as above
		}
	}
	return AudioCacheItemSample(event->item[0]); // RA2: as above
}

/// <summary>
/// The next sample an event plays: the next one in order or a random one of the rest, the loop's first sample again
/// when the sequence is used up, and last the decay sample.
/// </summary>
AudioSample * __fastcall eventNextSample(AudioEvent * event) // RA2: 0x00404790
{
	int i; // RA2: as above
	int control; // RA2: as above

	control = event->eclass->Control; // RA2: as above
	if (event->eclass->MinDelay >= EVENT_MIN_DELAY && !(event->flags & mAUDIO_EVENT_END)) { // RA2: as above
		return NULL; // RA2: as above
	}
	if (event->numSequence == 0 || event->flags & mAUDIO_EVENT_END) { // RA2: as above
		if (control & AUDIO_EVENT_CTRL_LOOP && !(event->flags & mAUDIO_EVENT_END)) { // RA2: as above
			if (!event->eclass->Loop || (event->loopCount < event->eclass->Loop - 1)) { // RA2: as above
				event->loopCount++; // RA2: as above
				return eventFirstSample(event); // RA2: as above
			}
		}
		if ((!(event->eclass->Control & AUDIO_EVENT_CTRL_DECAY) || event->flags & mAUDIO_EVENT_NO_DECAY) || event->numItems < 1) { // RA2: as above
			return NULL; // RA2: as above
		}
		event->flags |= mAUDIO_EVENT_NO_DECAY; // RA2: as above
		return AudioCacheItemSample(event->item[event->numItems - 1]); // RA2: as above
	}
	if (control & AUDIO_EVENT_CTRL_RANDOM) { // RA2: as above
		int ndx; // RA2: as above

		ndx = AudioRandomPick(0, event->numSequence - 1); // RA2: as above
		event->currentItem = event->sequence[ndx]; // RA2: as above
		for (i = ndx; i < event->numSequence - 1; i++) { // RA2: as above
			event->sequence[i] = event->sequence[i + 1]; // RA2: as above
		}
	} else { // RA2: as above
		event->currentItem++; // RA2: as above
	}
	event->numSequence--; // RA2: as above
	return AudioCacheItemSample(event->item[event->currentItem]); // RA2: as above
}

/// <summary>
/// Loads and locks the samples an event needs from the cache: all of them at once, or one at a time
/// when the type has a delay between its samples.
/// </summary>
void __fastcall eventLoadAndLockSamples(AudioEvent * event) // RA2: 0x00404890
{
	int i, ndx; // RA2: as above
	int control; // RA2: as above
	AudioEventType * eclass; // RA2: as above

	control = event->eclass->Control; // RA2: as above
	eclass = event->eclass; // RA2: as above
	if (event->numItems) { // RA2: as above
		if (eclass->MinDelay >= EVENT_MIN_DELAY) { // RA2: as above
			eventUnlockSamples(event); // RA2: as above
		} else { // RA2: as above
			return; // RA2: as above
		}
	}
	if (eclass->MinDelay < EVENT_MIN_DELAY) { // RA2: as above
		if (eclass->Attack) { // RA2: as above
			eventLoadItem(event, AudioRandomPick(0, eclass->Attack - 1)); // RA2: as above
		}
		if (eclass->Control & AUDIO_EVENT_CTRL_ALL) { // RA2: as above
			for (i = eclass->Attack; i < eclass->NumSamples - eclass->Decay; i++) { // RA2: as above
				eventLoadItem(event, i); // RA2: as above
			}
		} else if (eclass->Control & AUDIO_EVENT_CTRL_RANDOM) { // RA2: as above
			i = AudioRandomPick(eclass->Attack, eclass->NumSamples - eclass->Decay - 1); // RA2: as above
			eventLoadItem(event, i); // RA2: as above
		} else { // RA2: as above
			eventLoadItem(event, eclass->Attack); // RA2: as above
		}
		if (eclass->Decay) { // RA2: as above
			eventLoadItem(event, AudioRandomPick(eclass->NumSamples - eclass->Decay, eclass->NumSamples - 1)); // RA2: as above
		}
	} else { // RA2: as above
		int decay_ndx = -1; // RA2: as above

		if (eclass->Decay && eclass->Control & AUDIO_EVENT_CTRL_DECAY) { // RA2: as above
			decay_ndx = AudioRandomPick(eclass->NumSamples - eclass->Decay, eclass->NumSamples - 1); // RA2: as above
		}
		if ((ndx = eventLoadSchedule(event)) >= 0) { // RA2: as above
			eventLoadItem(event, ndx); // RA2: as above
			if (decay_ndx != -1) { // RA2: as above
				eventLoadItem(event, decay_ndx); // RA2: as above
			}
		} else { // RA2: as above
			if (decay_ndx != -1 && !(event->flags & mAUDIO_EVENT_NO_DECAY)) { // RA2: as above
				eventLoadItem(event, decay_ndx); // RA2: as above
				event->flags |= mAUDIO_EVENT_NO_DECAY; // RA2: as above
			}
		}
	}
}

/// <summary>
/// The index of the next sample to load for an event with delays between its samples, or -1 when the
/// loops are used up; makes the order of the samples for the next round when the last one is used.
/// </summary>
int __fastcall eventLoadSchedule(AudioEvent * event) // RA2: 0x00404B90
{
	AudioEventType * eclass; // RA2: as above
	int control; // RA2: as above
	int i; // RA2: as above
	int item_ndx; // RA2: as above

	eclass = event->eclass; // RA2: as above
	control = eclass->Control; // RA2: as above
	if (event->numLoadSequence == event->loadNdx) { // RA2: as above
		int ndx = 0; // RA2: as above

		if ((!(eclass->Control & AUDIO_EVENT_CTRL_LOOP) && event->loadLoopCount == 1) // RA2: as above
			|| ((eclass->Control & AUDIO_EVENT_CTRL_LOOP) && eclass->Loop && (event->loadLoopCount) >= eclass->Loop)) { // RA2: as above
			return -1; // RA2: as above
		}
		if (eclass->Attack && !(event->flags & mAUDIO_EVENT_NO_ATTACK)) { // RA2: as above
			event->loadSequence[ndx++] = AudioRandomPick(0, eclass->Attack - 1); // RA2: as above
		}
		if (eclass->Control & AUDIO_EVENT_CTRL_ALL) { // RA2: as above
			for (i = eclass->Attack; i < eclass->NumSamples - eclass->Decay; i++) { // RA2: as above
				event->loadSequence[ndx++] = i; // RA2: as above
			}
		} else { // RA2: as above
			if (eclass->Control & AUDIO_EVENT_CTRL_RANDOM) { // RA2: as above
				if (event->loadLoopCount == 0) { // RA2: as above
					i = AudioRandomPick(eclass->Attack, eclass->NumSamples - (eclass->Decay + 1)); // RA2: as above
					event->loadSaveNdx = i; // RA2: as above
				} else { // RA2: as above
					i = event->loadSaveNdx; // RA2: as above
				}
			} else { // RA2: as above
				i = eclass->Attack; // RA2: as above
			}
			event->loadSequence[ndx++] = i; // RA2: as above
		}
		event->numLoadSequence = ndx; // RA2: as above
		event->loadNdx = 0; // RA2: as above
		event->loadLoopCount++; // RA2: as above
	}
	if (control & AUDIO_EVENT_CTRL_RANDOM // RA2: as above
		&& ((!eclass->Attack || (event->flags & mAUDIO_EVENT_NO_ATTACK)) || event->loadNdx > 0)) { // RA2: as above
		int ndx, currentSampleIndex; // RA2: as above

		ndx = AudioRandomPick((!eclass->Attack || (event->flags & mAUDIO_EVENT_NO_ATTACK)) ? 0 : 1, event->numLoadSequence - 1); // RA2: as above
		item_ndx = currentSampleIndex = event->loadSequence[ndx]; // RA2: as above
		for (i = ndx; i < event->numLoadSequence - 1; i++) { // RA2: as above
			event->loadSequence[i] = event->loadSequence[i + 1]; // RA2: as above
		}
		event->numLoadSequence--; // RA2: as above
	} else { // RA2: as above
		item_ndx = event->loadSequence[event->loadNdx]; // RA2: as above
		event->loadNdx++; // RA2: as above
	}
	return item_ndx; // RA2: as above
}

/// <summary>
/// Unlocks the cached samples an event holds.
/// </summary>
void __fastcall eventUnlockSamples(AudioEvent * event) // RA2: 0x00404D60
{
	int i; // RA2: as above

	for (i = 0; i < event->numItems; i++) { // RA2: as above
		AudioCacheItemUnlock(event->item[i]); // RA2: as above
		event->item[i] = NULL; // RA2: as above
	}
	event->numItems = 0; // RA2: as above
}

/// <summary>
/// Takes a dead event out of the list and gives its memory back: to the pool, or to the heap when it was allocated there.
/// </summary>
void __fastcall audioEventDestroy(AudioEvent * event) // RA2: 0x00404DB0
{
	ListNodeRemove(&event->nd); // RA2: as above
	event->stamp = AUDIO_INVALID_STAMP; // RA2: as above
	if (event->flags & mAUDIO_EVENT_ALLOCATED) { // RA2: as above
		delete event; // RA2: as above
	} else { // RA2: as above
		MemoryPoolReturnItem(audioEventPool, event); // RA2: as above
	}
	AudioEventsCount--; // RA2: as above
}

/// <summary>
/// The first event type in the buckets of a lower priority than this one: the type to sacrifice for a new event.
/// </summary>
AudioEventType * __fastcall bucketGetWeaker(int pri) // RA2: 0x00404E00
{
	EClassBucket * ecb; // RA2: as above
	ListNode * node; // RA2: as above
	int i, v; // RA2: as above

	for (i = 0, ecb = eClassBucket; i < pri; i++, ecb++) { // RA2: as above
		for (v = 0; v < NUM_BUCKETS_VOL; v++) { // RA2: as above
			node = ListFirstItem(&ecb->bucketVol[v]); // RA2: as above
			if (node) { // RA2: as above
				AudioEventType * event = (AudioEventType *)node; // RA2: as above
				event = (AudioEventType *)((unsigned)event - ((unsigned)&event->nodeVol - (unsigned)event)); // RA2: as above
				return event; // RA2: as above
			}
		}
	}
	return NULL; // RA2: as above
}

/// <summary>
/// Stops and frees every event.
/// </summary>
void AudioKillAllEvents(void) // RA2: 0x00404E50
{
	AudioEvent * event, * next, * head; // RA2: as above

	if (!audioEventsReady) { // RA2: as above
		return; // RA2: as above
	}
	head = (AudioEvent *)&audioEvents; // RA2: as above
	event = (AudioEvent *)head->nd.next; // RA2: as above
	while (event != head) { // RA2: as above
		next = (AudioEvent *)event->nd.next; // RA2: as above
		AudioEventStop(event); // RA2: as above
		audioEventDestroy(event); // RA2: as above
		event = next; // RA2: as above
	}
}

/// <summary>
/// Pauses every event that is not paused yet; a waiting event keeps the time it has left.
/// </summary>
void AudioPauseAllEvents(void) // RA2: 0x00404FB0
{
	AudioEvent * event, * head; // RA2: as above

	if (!audioEventsReady) { // RA2: as above
		return; // RA2: as above
	}
	head = (AudioEvent *)&audioEvents; // RA2: as above
	event = (AudioEvent *)head->nd.next; // RA2: as above
	while (event != head) { // RA2: as above
		if (!AudioEventIsPaused(event)) { // RA2: as above
			if (event->state == AUDIO_EVENT_WAITING) { // RA2: as above
				event->timeOut -= AudioGetTime(); // RA2: as above
			}
		}
		LockAcquire(&event->paused); // RA2: as above
		event = (AudioEvent *)event->nd.next; // RA2: as above
	}
}

/// <summary>
/// Resumes every paused event; a waiting event gets its time back.
/// </summary>
void AudioResumeAllEvents(void) // RA2: 0x00405020
{
	AudioEvent * event, * head; // RA2: as above

	if (!audioEventsReady) { // RA2: as above
		return; // RA2: as above
	}
	head = (AudioEvent *)&audioEvents; // RA2: as above
	event = (AudioEvent *)head->nd.next; // RA2: as above
	while (event != head) { // RA2: as above
		if (AudioEventIsPaused(event)) { // RA2: as above
			LockRelease(&event->paused); // RA2: as above
			if (!AudioEventIsPaused(event)) { // RA2: as above
				if (event->state == AUDIO_EVENT_WAITING) { // RA2: as above
					event->timeOut += AudioGetTime(); // RA2: as above
				}
			}
		}
		event = (AudioEvent *)event->nd.next; // RA2: as above
	}
}

/// <summary>
/// Frees the events that are dead.
/// </summary>
void AudioFlushAllDeadEvents(void) // RA2: 0x004050A0
{
	AudioEvent * event, * next, * head; // RA2: as above

	if (!audioEventsReady) { // RA2: as above
		return; // RA2: as above
	}
	head = (AudioEvent *)&audioEvents; // RA2: as above
	event = (AudioEvent *)head->nd.next; // RA2: as above
	while (event != head) { // RA2: as above
		next = (AudioEvent *)event->nd.next; // RA2: as above
		if (event->flags & mAUDIO_EVENT_DEAD) { // RA2: as above
			audioEventDestroy(event); // RA2: as above
		}
		event = next; // RA2: as above
	}
}

/// <summary>
/// The range of an event type, in cells.
/// </summary>
int __fastcall AudioEventTypeRange(AudioEventType * type) // RA2: 0x00405110
{
	if (!audioEventsReady) { // RA2: as above
		return 0; // RA2: as above
	}
	return type->Range; // RA2: as above
}

/// <summary>
/// The lowest volume an event type is played at.
/// </summary>
float __fastcall AudioEventTypeMinVolume(AudioEventType * type) // RA2: 0x00405120
{
	if (!audioEventsReady) { // RA2: as above
		return 0.0f; // RA2: as above
	}
	return type->MinVolume; // RA2: as above
}

/// <summary>
/// The "Type" flags of an event type.
/// </summary>
unsigned int __fastcall AudioEventTypeGetType(AudioEventType * type) // RA2: 0x00405140
{
	if (!audioEventsReady) { // RA2: as above
		return 0; // RA2: as above
	}
	return type->Type; // RA2: as above
}

/// <summary>
/// The name of an event type.
/// </summary>
char const * __fastcall AudioEventTypeName(AudioEventType * type) // RA2: 0x00405150
{
	if (!audioEventsReady) { // RA2: as above
		return "no events"; // RA2: as above
	}
	return type->Name; // RA2: as above
}

/// <summary>
/// Makes an event of a type, from the pool or from the heap when the pool is empty; NULL when the layer is off or the type has a missing sample.
/// </summary>
AudioEvent * __fastcall AudioEventCreate(AudioEventType * eclass) // RA2: 0x00405170
{
	AudioEvent * event; // RA2: as above
	int allocated = 0; // RA2: as above

	if (!eventsOn || !eventsOK) { // RA2: as above
		return NULL; // RA2: as above
	}
	if (!eclass->SamplesOK) { // RA2: as above
		return NULL; // RA2: as above
	}
	if (!(event = (AudioEvent *)MemoryPoolGetItem(audioEventPool))) { // RA2: as above
		AudioFlushAllDeadEvents(); // RA2: as above
		if (!(event = (AudioEvent *)MemoryPoolGetItem(audioEventPool))) { // RA2: as above
			event = (AudioEvent *)operator new(sizeof(AudioEvent)); // RA2: a full pool: the event comes from the heap (the YR decompilation gives up here)
			allocated = 1; // RA2: as above
			if (!event) { // RA2: as above
				return NULL; // RA2: as above
			}
		}
	}
	AudioEventsCount++; // RA2: as above
	if (AudioEventsCount > AudioEventsPeak) { // RA2: as above
		AudioEventsPeak = AudioEventsCount; // RA2: as above
	}
	memset(event, 0, sizeof(AudioEvent)); // RA2: as above
	event->eclass = eclass; // RA2: as above
	ListNodeInit(&event->nd); // RA2: as above
	event->state = AUDIO_EVENT_NEW; // RA2: as above
	event->adjustPriority = 0; // RA2: as above
	event->numItems = event->numSequence = event->numLoadSequence = 0; // RA2: as above
	AudioAttribsInit(&event->attribs); // RA2: as above
	LockInit(&event->paused); // RA2: as above
	ListNodeInsert((ListNode *)&audioEvents, &event->nd); // RA2: as above
	event->stamp = audioStamp++; // RA2: as above
	if (allocated) { // RA2: a heap event is flagged so that audioEventDestroy deletes it
		event->flags |= mAUDIO_EVENT_ALLOCATED; // RA2: as above
	}
	return event; // RA2: as above
}

/// <summary>
/// Marks an event as past its attack.
/// </summary>
void __fastcall AudioEventSetFlag8(AudioEvent * event) // RA2: 0x004052F0
{
	event->flags |= mAUDIO_EVENT_NO_ATTACK; // RA2: as above
}

/// <summary>
/// Stops an event for good: lets go of its channel and samples and marks it dead.
/// </summary>
void __fastcall AudioEventStop(AudioEvent * event) // RA2: 0x00405300
{
	if (event->flags & mAUDIO_EVENT_DEAD) { // RA2: as above
		return; // RA2: as above
	}
	eventReleaseChannel(event); // RA2: as above
	eventUnlockSamples(event); // RA2: as above
	event->state = AUDIO_EVENT_DONE; // RA2: as above
	event->stamp = AUDIO_INVALID_STAMP; // RA2: as above
	event->handle = NULL; // RA2: as above
	event->flags |= mAUDIO_EVENT_DEAD; // RA2: as above
}

/// <summary>
/// Pauses one event; a waiting event keeps the time it has left.
/// </summary>
void __fastcall AudioEventPause(AudioEvent * event) // RA2: 0x004053F0
{
	if (!AudioEventIsPaused(event)) { // RA2: as above
		if (event->state == AUDIO_EVENT_WAITING) { // RA2: as above
			event->timeOut -= AudioGetTime(); // RA2: as above
		}
	}
	LockAcquire(&event->paused); // RA2: as above
}

/// <summary>
/// Resumes one paused event; a waiting event gets its time back.
/// </summary>
void __fastcall AudioEventResume(AudioEvent * event) // RA2: 0x00405440
{
	if (AudioEventIsPaused(event)) { // RA2: as above
		LockRelease(&event->paused); // RA2: as above
		if (!AudioEventIsPaused(event)) { // RA2: as above
			if (event->state == AUDIO_EVENT_WAITING) { // RA2: as above
				event->timeOut += AudioGetTime(); // RA2: as above
			}
		}
	}
}

/// <summary>
/// The pause count of an event.
/// </summary>
int __fastcall AudioEventIsPaused(AudioEvent * event) // RA2: 0x004054A0
{
	return event->paused.count; // RA2: as above
}

/// <summary>
/// Starts the sample an event has ready on its channel at the event's pitch and volume shift; undoes it and returns 0 when the channel does not start.
/// </summary>
int __fastcall audioEventStart(AudioEvent * event) // RA2: 0x004054B0
{
	AudioSample * sample; // RA2: as above
	unsigned volume; // RA2: as above

	if (event != (AudioEvent *)event->channel->Data) { // RA2: as above
		return 0; // RA2: as above
	}
	AudioChannelLock(event->channel); // RA2: as above
	sample = (AudioSample *)event->startSample; // RA2: as above
	event->flags |= mAUDIO_EVENT_NO_ATTACK; // RA2: as above
	AudioChannelSetSample(event->channel, sample); // RA2: as above
	AudioLevelSet(&event->channel->ChannelAttribs.PitchLevel, event->fshift); // RA2: as above
	volume = AUDIO_LEVEL_MAX; // RA2: as above
	if (event->vshift > 0) { // RA2: as above
		volume -= (unsigned)(AUDIO_LEVEL_MAX * event->vshift) / 100; // RA2: as above
	}
	AudioLevelSet(&event->channel->ChannelAttribs.VolumeLevel, volume); // RA2: as above
	event->state = AUDIO_EVENT_PLAYING; // RA2: as above
	event->flags |= mAUDIO_EVENT_PLAYING; // RA2: as above
	event->startSample = NULL; // RA2: as above
	if (AudioChannelStart(event->channel) != vNO_ERROR) { // RA2: as above
		event->state = AUDIO_EVENT_START_PLAYING; // RA2: as above
		event->startSample = sample; // RA2: as above
		event->flags &= ~mAUDIO_EVENT_PLAYING; // RA2: as above
		AudioChannelUnlock(event->channel); // RA2: as above
		return 0; // RA2: as above
	}
	AudioChannelUnlock(event->channel); // RA2: as above
	return 1; // RA2: as above
}

/// <summary>
/// Asks an event to end: it plays its decay and stops instead of looping on.
/// </summary>
void __fastcall AudioEventEnd(AudioEvent * event) // RA2: 0x00405590
{
	if (event->flags & mAUDIO_EVENT_END) { // RA2: as above
		return; // RA2: as above
	}
	if (event->channel && event == (AudioEvent *)event->channel->Data) { // RA2: as above
		AudioChannelLock(event->channel); // RA2: as above
		event->flags |= mAUDIO_EVENT_END | mAUDIO_EVENT_DO_END; // RA2: as above
		AudioChannelUnlock(event->channel); // RA2: as above
	} else { // RA2: as above
		event->flags |= mAUDIO_EVENT_END | mAUDIO_EVENT_DO_END; // RA2: as above
	}
}

static int audioEventPrep(AudioEvent * event) // RA2: a static helper, expanded into its callers (no copy of its own in the executable)
{
	AudioEventType * eclass; // RA2: as above
	int delay; // RA2: as above

	eclass = event->eclass; // RA2: as above
	if (eclass->NumSamples == 0 || !eclass->SamplesOK) { // RA2: as above
		return 0; // RA2: as above
	}
	event->loopCount = 0; // RA2: as above
	if (!(event->channel = audioPrepareChannel(eclass->Priority + event->adjustPriority, 0))) { // RA2: as above
		return 0; // RA2: as above
	}
	event->fshift = 100 + AudioRandomPick(eclass->MinFDelta, eclass->MaxFDelta); // RA2: as above
	event->vshift = AudioRandomPick(0, eclass->VShift); // RA2: as above
	AudioChannelUse(event->channel); // RA2: as above
	event->channel->Data = event; // RA2: as above
	event->channel->CB_NextSample = audioEventNextSample; // RA2: as above
	event->channel->CB_SampleDone = audioEventSampleDone; // RA2: as above
	event->channel->CB_Stop = audioEventStop; // RA2: as above
	event->state = AUDIO_EVENT_START_PLAYING; // RA2: as above
	event->channel->SfxAttribs = &event->attribs; // RA2: as above
	event->channel->CompAttribs = &audioCompressionAttribs; // RA2: as above
	event->channel->FadeAttribs = AudioGroupAttribs1; // RA2: as above
	if (eclass->Control & (AUDIO_EVENT_CTRL_PREDELAY | AUDIO_EVENT_CTRL_AMBIENT)) { // RA2: as above
		delay = AudioRandomPick((eclass->Control & AUDIO_EVENT_CTRL_AMBIENT) ? EVENT_MIN_DELAY : eclass->MinDelay, eclass->MaxDelay); // RA2: as above
		if (delay >= EVENT_MIN_DELAY) { // RA2: as above
			eventWait(event, delay, AUDIO_EVENT_START_PLAYING); // RA2: as above
		}
	}
	return 1; // RA2: as above
}

static void eventCheckStart(AudioEvent * event, int succeeded) // RA2: a static helper, expanded into its callers (no copy of its own in the executable)
{
	if (!succeeded) { // RA2: as above
		AudioEventStop(event); // RA2: as above
	}
}

/// <summary>
/// Moves one event along its states: gets it a channel, starts it when its wait is over, ends it when asked to, frees it when done.
/// </summary>
void __fastcall AudioEventService(AudioEvent * event) // RA2: 0x004055D0
{
	if (!eventsOn) { // RA2: as above
		event->state = AUDIO_EVENT_DONE; // RA2: as above
	}

	for (;;) { // RA2: the states loop back here; the executable switches over the state through the jump table at 0x004059E0
		switch (event->state) { // RA2: as above
		case AUDIO_EVENT_WAITING: // RA2: as above
			if (!(event->flags & mAUDIO_EVENT_DO_END)) { // RA2: as above
				if (!AudioEventIsPaused(event) && AudioGetTime() > event->timeOut) { // RA2: as above
					event->state = event->nextState; // RA2: as above
					continue; // RA2: as above
				}
				return; // RA2: as above
			}
			break; // RA2: as above

		case AUDIO_EVENT_NEW: // RA2: as above
			eventCheckStart(event, audioEventPrep(event)); // RA2: as above
			return; // RA2: as above

		case AUDIO_EVENT_PLAYING: // RA2: as above
			if ((event->eclass->Control & AUDIO_EVENT_CTRL_LOOP) && !event->eclass->Loop // RA2: as above
				&& !(event->flags & mAUDIO_EVENT_END)) { // RA2: as above
				AudioController * controller = event->handle; // RA2: as above
				if (!controller || AudioControllerGetEvent(controller) != event) { // RA2: as above
					AudioEventStop(event); // RA2: as above
				}
			}
			if (!(event->flags & mAUDIO_EVENT_DO_END)) { // RA2: as above
				return; // RA2: as above
			}
			break; // RA2: as above

		case AUDIO_EVENT_DONE: // RA2: as above
			AudioEventStop(event); // RA2: as above
			return; // RA2: as above

		default: // RA2: as above
			return; // RA2: as above
		}

		{
			AudioChannel * channel = event->channel; // RA2: as above
			if (channel) { // RA2: as above
				AudioChannelLock(channel); // RA2: as above
				event->flags &= ~mAUDIO_EVENT_DO_END; // RA2: as above
				AudioChannelStop(event->channel); // RA2: as above
				if ((event->eclass->Control & AUDIO_EVENT_CTRL_DECAY) && !(event->flags & mAUDIO_EVENT_NO_DECAY)) { // RA2: as above
					event->state = AUDIO_EVENT_START_PLAYING; // RA2: as above
					event->startSample = eventNextSample(event); // RA2: as above
					if (!AudioEventIsPaused(event)) { // RA2: as above
						eventCheckStart(event, audioEventStart(event)); // RA2: as above
					}
				}
				AudioChannelUnlock(channel); // RA2: as above
			} else { // RA2: as above
				event->flags &= ~mAUDIO_EVENT_DO_END; // RA2: as above
			}
		}
		return; // RA2: as above
	}
}

/// <summary>
/// The channel callback for a stopped channel: the event is done.
/// </summary>
int __fastcall audioEventStop(AudioChannel * chan) // RA2: 0x00405A00
{
	AudioEvent * event = (AudioEvent *)chan->Data; // RA2: as above

	event->state = AUDIO_EVENT_DONE; // RA2: as above
	return vNO_ERROR; // RA2: as above
}

static void eventWait(AudioEvent * event, TimeStamp delay, int nextState) // RA2: a static helper, expanded into its callers (no copy of its own in the executable)
{
	event->nextState = nextState; // RA2: as above
	if (AudioEventIsPaused(event)) { // RA2: as above
		event->timeOut = delay; // RA2: as above
	} else { // RA2: as above
		event->timeOut = AudioGetTime() + delay; // RA2: as above
	}
	event->state = AUDIO_EVENT_WAITING; // RA2: as above
}

/// <summary>
/// The channel callback for a sample that is played: the event waits out its delay, plays on, or is done.
/// </summary>
int __fastcall audioEventSampleDone(AudioChannel * chan) // RA2: 0x00405A10
{
	AudioEvent * event = (AudioEvent *)chan->Data; // RA2: as above
	int new_state; // RA2: as above

	event->flags &= ~mAUDIO_EVENT_PLAYING; // RA2: as above
	if (event->state == AUDIO_EVENT_DONE) { // RA2: as above
		return vNO_ERROR; // RA2: as above
	}
	if (event->startSample || event->numLoadSequence || event->delay) { // RA2: as above
		new_state = AUDIO_EVENT_START_PLAYING; // RA2: as above
	} else { // RA2: as above
		new_state = AUDIO_EVENT_DONE; // RA2: as above
		event->delay = 0; // RA2: as above
	}
	if (event->delay) { // RA2: as above
		eventWait(event, event->delay, new_state); // RA2: as above
		event->delay = 0; // RA2: as above
	} else { // RA2: as above
		event->state = new_state; // RA2: as above
	}
	return vNO_ERROR; // RA2: as above
}

/// <summary>
/// The channel callback that asks for the next sample: hands it on, or keeps it for after the delay between samples.
/// </summary>
int __fastcall audioEventNextSample(AudioChannel * chan) // RA2: 0x00405AD0
{
	AudioEvent * event = (AudioEvent *)chan->Data; // RA2: as above
	AudioSample * sample; // RA2: as above
	unsigned delay; // RA2: as above

	sample = eventNextSample(event); // RA2: as above
	if (event->eclass->MaxDelay < EVENT_MIN_DELAY) { // RA2: as above
		AudioChannelSetSample(chan, sample); // RA2: as above
		return vNO_ERROR; // RA2: as above
	}
	AudioChannelSetSample(chan, NULL); // RA2: as above
	if ((event->eclass->Control & (AUDIO_EVENT_CTRL_PREDELAY | AUDIO_EVENT_CTRL_AMBIENT)) && !sample && !event->numLoadSequence) { // RA2: as above
		return vNO_ERROR; // RA2: as above
	}
	delay = AudioRandomPick(event->eclass->MinDelay, event->eclass->MaxDelay); // RA2: as above
	if (delay < EVENT_MIN_DELAY) { // RA2: as above
		AudioChannelSetSample(chan, sample); // RA2: as above
		return vNO_ERROR; // RA2: as above
	}
	event->delay = delay; // RA2: as above
	event->startSample = sample; // RA2: as above
	return vNO_ERROR; // RA2: as above
}

/// <summary>
/// A channel for an event of a priority: a free one, or one that plays something of the same or a lower priority.
/// </summary>
AudioChannel * __fastcall audioPrepareChannel(int pri, int flags) // RA2: 0x00405B60
{
	AudioChannel * chan = NULL; // RA2: as above

	if (audioDevice && (chan = AudioDeviceGetChannel(audioDevice, AUDIO_CHANNEL_TYPE_STD))) { // RA2: as above
		if (!(chan->Control.Status & (mAUDIO_CTRL_PLAYING | mAUDIO_CTRL_PAUSED | mAUDIO_CTRL_INUSE)) || chan->Control.Priority <= pri) { // RA2: as above
			AudioChannelStop(chan); // RA2: as above
			chan->GroupAttribs = AudioSoundAttribs; // RA2: as above
			chan->FadeAttribs = AudioGroupAttribs1; // RA2: as above
			chan->Control.Priority = pri; // RA2: as above
			if (flags & 1) { // RA2: as above
				chan->Control.LoopCount = AUDIO_CTRL_LOOP_FOREVER; // RA2: as above
			} else { // RA2: as above
				chan->Control.LoopCount = 0; // RA2: as above
			}
		} else { // RA2: as above
			return NULL; // RA2: as above
		}
	}
	return chan; // RA2: as above
}

/// <summary>
/// Forgets the event of a handle and its type; the sound it started keeps playing.
/// </summary>
void AudioController::Init(void) // RA2: 0x00405BF0
{
	AudioIndex = (void **)&AudioIDXData::Instance; // RA2: as above
	Event = NULL; // RA2: as above
	Stamp = 0; // RA2: as above
	EventType = NULL; // RA2: as above
}

/// <summary>
/// Stops the sound of a handle and detaches it from the audio library.
/// </summary>
void AudioController::Release(void) // RA2: 0x00405C10
{
	Stop(); // RA2: as above
	AudioIndex = NULL; // RA2: as above
}

/// <summary>
/// Stops the event of a handle if it is still the same one, and forgets the event and its type.
/// </summary>
void AudioController::Stop(void) // RA2: 0x00405D50
{
	AudioEvent * event = AudioControllerGetEvent(this); // RA2: as above

	if (event) { // RA2: as above
		AudioEventStop(event); // RA2: as above
		Event = NULL; // RA2: as above
	}
	EventType = NULL; // RA2: as above
}

/// <summary>
/// As Stop, but only for an event that loops without end.
/// </summary>
void __fastcall AudioControllerStopLooping(AudioController * controller) // RA2: 0x00405E90; a free function here: a member declared in audioctl.h unmatches vein.cpp (VC6 header effect)
{
	AudioEvent * event = AudioControllerGetEvent(controller); // RA2: as above

	if (event && AudioEventTypeLoopsForever(event->eclass)) { // RA2: as above
		AudioEventStop(event); // RA2: as above
		controller->Event = NULL; // RA2: as above
	}
	controller->EventType = NULL; // RA2: as above
}

/// <summary>
/// Ends the event of a handle if it is still the same one (it plays its decay); forgets the event and its type.
/// </summary>
void AudioController::End(void) // RA2: 0x00405FE0
{
	AudioEvent * event = AudioControllerGetEvent(this); // RA2: as above

	if (event) { // RA2: as above
		AudioEventEnd(event); // RA2: as above
		Event = NULL; // RA2: as above
	}
	EventType = NULL; // RA2: as above
}

/// <summary>
/// As End, but only for an event that loops without end.
/// </summary>
void AudioController::EndLooping(void) // RA2: 0x00406070
{
	AudioEvent * event = AudioControllerGetEvent(this); // RA2: as above

	if (event && AudioEventTypeLoopsForever(event->eclass)) { // RA2: as above
		AudioEventEnd(event); // RA2: as above
		Event = NULL; // RA2: as above
	}
	EventType = NULL; // RA2: as above
}

/// <summary>
/// Points a handle at an event and its type, and the event back at the handle.
/// </summary>
void __fastcall AudioController::SetEvent(AudioEvent * event, VocClass * type) // RA2: 0x00406100
{
	if (AudioIndex != (void **)&AudioIDXData::Instance) { // RA2: as above
		return; // RA2: as above
	}
	EventType = type; // RA2: as above
	if ((Event = event)) { // RA2: as above
		Stamp = event->stamp; // RA2: as above
		if (!type) { // RA2: as above
			EventType = (VocClass *)event->eclass; // RA2: as above
		}
		event->handle = this; // RA2: as above
	}
}

/// <summary>
/// The event of a handle while it is still good: the event is gone, or made again for another type, when its stamp or type differ.
/// </summary>
AudioEvent * __fastcall AudioControllerGetEvent(AudioController * controller) // RA2: 0x00406140
{
	AudioEvent * event; // RA2: as above

	if (!audioEventsReady || controller->AudioIndex != (void **)&AudioIDXData::Instance) { // RA2: as above
		return NULL; // RA2: as above
	}
	if ((event = controller->Event)) { // RA2: as above
		if ((AudioEventType *)controller->EventType != event->eclass || controller->Stamp != (unsigned)event->stamp) { // RA2: as above
			controller->Event = event = NULL; // RA2: as above
		}
	}
	return event; // RA2: as above
}

/// <summary>
/// The event type of a handle.
/// </summary>
AudioEventType * __fastcall AudioControllerGetType(AudioController * controller) // RA2: 0x00406180
{
	if (!audioEventsReady || controller->AudioIndex != (void **)&AudioIDXData::Instance) { // RA2: as above
		return NULL; // RA2: as above
	}
	return (AudioEventType *)controller->EventType; // RA2: as above
}

static unsigned eventCalcVolume(AudioEvent * event, int volume) // RA2: a static helper, expanded into its callers (no copy of its own in the executable)
{
	if (volume > AUDIO_PAN_RIGHT) { // RA2: as above
		volume = AUDIO_LEVEL_MAX; // RA2: as above
	} else if (volume < 0) { // RA2: as above
		volume = 0; // RA2: as above
	}
	return AUDIO_LEVEL_APPLY(&event->eclass->Volume, volume); // RA2: as above
}

/// <summary>
/// Sets the volume of an event at once, scaled by its type's volume.
/// </summary>
void __fastcall AudioEventJumpVolume(AudioEvent * event, int volume) // RA2: 0x004061A0
{
	AudioLevelSet(&event->attribs.VolumeLevel, eventCalcVolume(event, volume)); // RA2: as above
	AudioLevelUpdate(&event->attribs.VolumeLevel); // RA2: as above
}

/// <summary>
/// Sets the volume of an event: fading to it while it plays, at once before.
/// </summary>
void __fastcall AudioEventSetVolume(AudioEvent * event, int volume) // RA2: 0x004061E0
{
	if (event->state == AUDIO_EVENT_PLAYING) { // RA2: as above
		AudioLevelAdjust(&event->attribs.VolumeLevel, eventCalcVolume(event, volume)); // RA2: as above
	} else { // RA2: as above
		AudioEventJumpVolume(event, volume); // RA2: as above
	}
}

/// <summary>
/// Sets the pan of an event at once.
/// </summary>
void __fastcall AudioEventJumpPan(AudioEvent * event, int pan) // RA2: 0x00406260
{
	if (pan > AUDIO_PAN_RIGHT) { // RA2: as above
		pan = AUDIO_PAN_RIGHT; // RA2: as above
	} else if (pan < AUDIO_PAN_LEFT) { // RA2: as above
		pan = AUDIO_PAN_LEFT; // RA2: as above
	}
	AudioLevelSet(&event->attribs.PanPosition, pan); // RA2: as above
}

/// <summary>
/// Sets the pan of an event: moving to it while it plays, at once before.
/// </summary>
void __fastcall AudioEventSetPan(AudioEvent * event, int pan) // RA2: 0x00406280
{
	if (event->state == AUDIO_EVENT_PLAYING) { // RA2: as above
		if (pan > AUDIO_PAN_RIGHT) { // RA2: as above
			pan = AUDIO_PAN_RIGHT; // RA2: as above
		} else if (pan < AUDIO_PAN_LEFT) { // RA2: as above
			pan = AUDIO_PAN_LEFT; // RA2: as above
		}
		AudioLevelAdjust(&event->attribs.PanPosition, pan); // RA2: as above
	} else { // RA2: as above
		AudioEventJumpPan(event, pan); // RA2: as above
	}
}

/// <summary>
/// Sets how long the volume, pitch and pan of an event take to reach a new level.
/// </summary>
void __fastcall AudioEventSetAdjustDuration(AudioEvent * event, TimeStamp time) // RA2: 0x004062D0
{
	AudioLevelSetDuration(&event->attribs.VolumeLevel, time, AUDIO_LEVEL_MAX); // RA2: as above
	AudioLevelSetDuration(&event->attribs.PitchLevel, time, AUDIO_LEVEL_MAX); // RA2: as above
	AudioLevelSetDuration(&event->attribs.PanPosition, time, AUDIO_LEVEL_MAX); // RA2: as above
}

/// <summary>
/// The event type of an event.
/// </summary>
AudioEventType * __fastcall AudioEventGetType(AudioEvent * event) // RA2: 0x00406320
{
	return event->eclass; // RA2: as above
}

/// <summary>
/// Whether an event loops without end.
/// </summary>
int __fastcall AudioEventNeverEnds(AudioEvent * event) // RA2: 0x00406330
{
	return AudioEventTypeLoopsForever(event->eclass); // RA2: as above
}

/// <summary>
/// The name of an event's type.
/// </summary>
char const * __fastcall AudioEventName(AudioEvent * event) // RA2: 0x00406350
{
	if (!audioEventsReady) { // RA2: as above
		return "no events"; // RA2: as above
	}
	if (!event->eclass) { // RA2: as above
		return "unknown"; // RA2: as above
	}
	return event->eclass->Name; // RA2: as above
}

/// <summary>
/// The event type of this name, or NULL.
/// </summary>
AudioEventType * __fastcall AudioEventClassFind(char const * name) // RA2: 0x00406370
{
	AudioEventType * eclass; // RA2: as above

	if (audioEventsReady) { // RA2: as above
		for (eclass = (AudioEventType *)ListFirstItem(&audioEventClasses); eclass; eclass = (AudioEventType *)ListNextItem(&eclass->Header)) { // RA2: as above
			if (!stricmp(name, eclass->Name)) { // RA2: as above
				return eclass; // RA2: as above
			}
		}
	}
	return NULL; // RA2: as above
}

/// <summary>
/// The event type of this name; a new one with the defaults is made when there is none.
/// </summary>
AudioEventType * __fastcall AudioEventTypeFind(char const * name) // RA2: 0x004063C0
{
	AudioEventType * eclass; // RA2: as above

	if (!audioEventsReady) { // RA2: as above
		return NULL; // RA2: as above
	}
	eclass = (AudioEventType *)ListFirstItem(&audioEventClasses); // RA2: as above
	while (eclass) { // RA2: as above
		if (!stricmp(name, eclass->Name)) { // RA2: as above
			break; // RA2: as above
		}
		eclass = (AudioEventType *)ListNextItem(&eclass->Header); // RA2: as above
	}
	if (eclass) { // RA2: as above
		return eclass; // RA2: as above
	}
	eclass = new AudioEventType; // RA2: as above
	memset(eclass, 0, sizeof(AudioEventType)); // RA2: as above
	memset(eclass, 0, sizeof(AudioEventType)); // RA2: as above
	ListNodeInit(&eclass->Header); // RA2: as above
	eclass->SamplesOK = 1; // RA2: as above
	eclass->Control = 0; // RA2: as above
	eclass->Type = 0x20; // RA2: as above
	eclass->Limit = 3; // RA2: as above
	eclass->Priority = 2; // RA2: as above
	eclass->Range = 10; // RA2: as above
	eclass->MinVolume = 0.2f; // RA2: as above
	AudioLevelInit(&eclass->Volume, AUDIO_LEVEL_MAX); // RA2: as above
	strncpy(eclass->Name, name, 31); // RA2: as above
	eclass->Name[31] = 0; // RA2: as above
	ListNodeInsert((ListNode *)&audioEventClasses, &eclass->Header); // RA2: as above
	return eclass; // RA2: as above
}

/// <summary>
/// Adds a sample of the bag to an event type; a missing sample is logged and clears SamplesOK.
/// </summary>
int __fastcall AudioEventTypeAddSample(AudioEventType * type, char const * sample) // RA2: 0x004064B0
{
	int index; // RA2: as above

	if (type->NumSamples == MAX_AUDIO_EVENT_SAMPLES) { // RA2: as above
		return 0; // RA2: as above
	}
	if (audioEventsReady && AudioIDXData::Instance) { // RA2: as above
		while (*sample == '$' || *sample == '#') { // RA2: as above
			sample++; // RA2: as above
		}
		index = AudioIDXData::Instance->FindSampleIndex(sample); // RA2: as above
		if (index != -1) { // RA2: as above
			type->SampleIndex[type->NumSamples] = index; // RA2: as above
			type->NumSamples++; // RA2: as above
			return 1; // RA2: as above
		}
		DebugString("Missing sample %s\n", sample); // RA2: as above
		type->SamplesOK = 0; // RA2: as above
	}
	return 1; // RA2: as above
}

/// <summary>
/// Sets how many samples of an event type are attack samples.
/// </summary>
void __fastcall AudioEventTypeSetAttack(AudioEventType * type, int attack) // RA2: 0x00406530
{
	type->Attack = attack; // RA2: as above
}

/// <summary>
/// Sets how many samples of an event type are decay samples.
/// </summary>
void __fastcall AudioEventTypeSetDecay(AudioEventType * type, int decay) // RA2: 0x00406540
{
	type->Decay = decay; // RA2: as above
}

/// <summary>
/// Sets the priority of an event type.
/// </summary>
void __fastcall AudioEventTypeSetPriority(AudioEventType * type, int priority) // RA2: 0x00406550
{
	type->Priority = priority; // RA2: as above
}

/// <summary>
/// Sets the volume of an event type.
/// </summary>
void __fastcall AudioEventTypeSetVolume(AudioEventType * type, int volume) // RA2: 0x00406560
{
	AudioLevelSet(&type->Volume, volume); // RA2: as above
	AudioLevelUpdate(&type->Volume); // RA2: as above
}

/// <summary>
/// Sets the control flags of an event type; an attack or decay sample is needed when its flag is set and none when it is not.
/// </summary>
void __fastcall AudioEventTypeSetControl(AudioEventType * type, unsigned int control) // RA2: 0x00406580
{
	type->Control = control; // RA2: as above
	if (control & AUDIO_EVENT_CTRL_ATTACK) { // RA2: as above
		if (type->Attack == 0) { // RA2: as above
			type->Attack = 1; // RA2: as above
		}
	} else { // RA2: as above
		type->Attack = 0; // RA2: as above
	}
	if (control & AUDIO_EVENT_CTRL_DECAY) { // RA2: as above
		if (type->Decay == 0) { // RA2: as above
			type->Decay = 1; // RA2: as above
		}
	} else { // RA2: as above
		type->Decay = 0; // RA2: as above
	}
}

/// <summary>
/// Sets the "Type" flags of an event type.
/// </summary>
void __fastcall AudioEventTypeSetType(AudioEventType * type, unsigned int flags) // RA2: 0x004065D0
{
	type->Type = flags; // RA2: as above
}

/// <summary>
/// Sets how many events of an event type may play at once.
/// </summary>
void __fastcall AudioEventTypeSetLimit(AudioEventType * type, int limit) // RA2: 0x004065E0
{
	type->Limit = limit; // RA2: as above
}

/// <summary>
/// Sets the range of an event type, in cells.
/// </summary>
void __fastcall AudioEventTypeSetRange(AudioEventType * type, int range) // RA2: 0x004065F0
{
	type->Range = range; // RA2: as above
}

/// <summary>
/// Sets the lowest volume an event type is played at.
/// </summary>
void __fastcall AudioEventTypeSetMinVolume(AudioEventType * type, float minVolume) // RA2: 0x00406600
{
	type->MinVolume = minVolume; // RA2: as above
}

/// <summary>
/// Sets the shortest and longest delay between the samples of an event type.
/// </summary>
void __fastcall AudioEventTypeSetDelay(AudioEventType * type, int minDelay, int maxDelay) // RA2: 0x00406610
{
	type->MinDelay = minDelay; // RA2: as above
	type->MaxDelay = maxDelay; // RA2: as above
}

/// <summary>
/// Sets the lowest and highest pitch shift of an event type.
/// </summary>
void __fastcall AudioEventTypeSetFShift(AudioEventType * type, int minShift, int maxShift) // RA2: 0x00406620
{
	type->MinFDelta = minShift; // RA2: as above
	type->MaxFDelta = maxShift; // RA2: as above
}

/// <summary>
/// Sets the volume shift of an event type, in percent.
/// </summary>
void __fastcall AudioEventTypeSetVShift(AudioEventType * type, int shift) // RA2: 0x00406630
{
	if (shift > 100) { // RA2: as above
		shift = 100; // RA2: as above
	} else if (shift < 0) { // RA2: as above
		shift = 0; // RA2: as above
	}
	type->VShift = shift; // RA2: as above
}

/// <summary>
/// Sets how often an event type loops; 0 is for ever.
/// </summary>
void __fastcall AudioEventTypeSetLoop(AudioEventType * type, int loop) // RA2: 0x00406650
{
	type->Loop = loop; // RA2: as above
}

/// <summary>
/// Whether an event type loops without end.
/// </summary>
int __fastcall AudioEventTypeLoopsForever(AudioEventType * type) // RA2: 0x00406660
{
	if ((type->Control & AUDIO_EVENT_CTRL_LOOP) && !type->Loop) { // RA2: as above
		return 1; // RA2: as above
	}
	return 0; // RA2: as above
}

/// <summary>
/// Plays the event type of this name once at a volume (0 to 256).
/// </summary>
void __fastcall Play_Named_Sound(char const * name, int volume) // RA2: 0x00406680
{
	AudioEventType * eclass; // RA2: as above
	AudioEvent * event; // RA2: as above

	if (audioEventsReady) { // RA2: as above
		for (eclass = (AudioEventType *)ListFirstItem(&audioEventClasses); eclass; eclass = (AudioEventType *)ListNextItem(&eclass->Header)) { // RA2: as above
			if (!stricmp(name, eclass->Name)) { // RA2: as above
				break; // RA2: as above
			}
		}
		if (eclass && (event = AudioEventCreate(eclass))) { // RA2: as above
			AudioEventJumpVolume(event, (int)((__int64)volume * AUDIO_LEVEL_MAX / 256)); // RA2: as above
		}
	}
}

/// <summary>
/// Turns the event layer off: every event is done at its next service.
/// </summary>
void AudioEventTurnOff(void) // RA2: 0x00406740
{
	eventsOn = 0; // RA2: as above
}

/// <summary>
/// Turns the event layer on.
/// </summary>
void AudioEventTurnOn(void) // RA2: 0x00406750
{
	eventsOn = 1; // RA2: as above
}

/// <summary>
/// Whether the event layer is on.
/// </summary>
int AudioEventOn(void) // RA2: 0x00406760
{
	return eventsOn; // RA2: as above
}

/// <summary>
/// Prints the number of events and of static sounds, now and at most, through a line printer.
/// </summary>
void __fastcall AudioEventsDump(void (__fastcall * print)(char const * line)) // RA2: 0x00406770
{
	char buffer[200]; // RA2: as above

	if (audioCache) { // RA2: as above
		sprintf(buffer, "AudioEvents : %05d - Max: %05d", AudioEventsCount, AudioEventsPeak); // RA2: as above
		print(buffer); // RA2: as above
		sprintf(buffer, "StaticSounds: %05d - Max: %05d", StaticSoundCount, StaticSoundMaximum); // RA2: as above
		print(buffer); // RA2: as above
	}
}

/// <summary>
/// The priority of a name (LOWEST to CRITICAL); returns whether the name is one.
/// </summary>
bool __fastcall AudioPriorityFromName(char const * name, int * priority) // RA2: 0x004067E0
{
	EventKeyword * entry; // RA2: as above

	for (entry = PriorityKeywords; entry->Name; entry++) { // RA2: as above
		if (!stricmp(name, entry->Name)) { // RA2: as above
			break; // RA2: as above
		}
	}
	if (priority) { // RA2: as above
		*priority = entry->Value;
	}
	return entry->Name != NULL; // RA2: as above
}

/// <summary>
/// Adds the control flag of a name to the flags; returns whether the name is one.
/// </summary>
bool __fastcall AudioControlApplyKeyword(unsigned int * control, char const * name) // RA2: 0x00406830
{
	EventKeyword * entry; // RA2: as above

	for (entry = ControlKeywords; entry->Name; entry++) { // RA2: as above
		if (!stricmp(name, entry->Name)) { // RA2: as above
			break; // RA2: as above
		}
	}
	if (control) { // RA2: as above
		*control |= entry->Value;
	}
	return entry->Name != NULL; // RA2: as above
}

/// <summary>
/// Adds the type flag of a name to the flags (a SCREEN, LOCAL or GLOBAL replaces the others of its kind, as UNSHROUD and SHROUD do);
/// returns whether the name is one.
/// </summary>
int __fastcall AudioTypeApplyKeyword(unsigned int * type, char const * name) // RA2: 0x00406880
{
	EventKeyword * entry; // RA2: as above

	for (entry = TypeKeywords; entry->Name; entry++) { // RA2: as above
		if (!stricmp(name, entry->Name)) { // RA2: as above
			break; // RA2: as above
		}
	}
	if (type) { // RA2: as above
		if (entry->Value & 0x60) { // RA2: as above
			*type &= ~0x60;
		} else if (entry->Value & 0xC00) { // RA2: as above
			*type &= ~0xC00;
		}
		*type |= entry->Value;
	}
	return entry->Name != NULL; // RA2: as above
}

/// <summary>
/// The release executable's debug print: nothing is printed, but the calls and their format strings stay.
/// </summary>
void __cdecl DebugString(char const *, ...) // RA2: 0x004068F0
{
}
