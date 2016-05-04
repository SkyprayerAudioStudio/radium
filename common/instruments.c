/* Copyright 2000 Kjetil S. Matheussen

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA. */


#include "nsmtracker.h"
#include "list_proc.h"
#include "vector_proc.h"
#include "patch_proc.h"

#include "instruments_proc.h"
#include "../midi/midi_i_plugin_proc.h"
#include "../audio/audio_instrument_proc.h"


static struct Instruments *g_instruments;

// Called during program shutdown
static void CloseInstrument(NInt instrumentnum){
	struct Instruments *temp=(struct Instruments *)ListFindElement1(
                &g_instruments->l,
		instrumentnum
	);
	if(temp==NULL) return;

	(*temp->CloseInstrument)(temp);

	ListRemoveElement1(&g_instruments,&temp->l);
}

// Called during program shutdown
void CloseAllInstruments(void){
	while(g_instruments!=NULL)
		CloseInstrument(g_instruments->l.num);
}



/* Called during program init. */
bool OpenInstruments(void){
  {
    struct Instruments *instrument=talloc(sizeof(struct Instruments));
    instrument->l.num=0;
    if(MIDI_initInstrumentPlugIn(instrument)==INSTRUMENT_FAILED) return false;

    ListAddElement1(&g_instruments,&instrument->l);
  }

  {
    struct Instruments *instrument=talloc(sizeof(struct Instruments));
    instrument->l.num=1;
    if(AUDIO_initInstrumentPlugIn(instrument)==INSTRUMENT_FAILED) return false;

    ListAddElement1(&g_instruments,&instrument->l);
  }

  return true;
}

struct Instruments *get_all_instruments(void){
  return g_instruments;
}

// Just use the MIDI instrument for this.
struct Instruments *get_default_instrument(void){
  return g_instruments;
}

struct Instruments *get_MIDI_instrument(void){
  return g_instruments;
}

struct Instruments *get_audio_instrument(void){
  return NextInstrument(g_instruments);
}

struct Instruments *get_instrument_from_type(int type){
  if(type==MIDI_INSTRUMENT_TYPE)
    return get_MIDI_instrument();
  if(type==AUDIO_INSTRUMENT_TYPE)
    return get_audio_instrument();

  return NULL;
}

int get_type_from_instrument(struct Instruments *instrument){
  if(instrument==get_MIDI_instrument())
    return MIDI_INSTRUMENT_TYPE;
  if(instrument==get_audio_instrument())
    return AUDIO_INSTRUMENT_TYPE;
  return NO_INSTRUMENT_TYPE;
}

void StopAllInstruments(void){
        struct Instruments *instrument=g_instruments;

	while(instrument!=NULL){
          VECTOR_FOR_EACH(struct Patch *patch, &instrument->patches)
            PATCH_stop_all_notes(patch);
          END_VECTOR_FOR_EACH;

          (*instrument->StopPlaying)(instrument);

          instrument=NextInstrument(instrument);
	}
}


void InitAllInstrumentsForPlaySongFromStart(void){
	struct Instruments *instrument=g_instruments;

	while(instrument!=NULL){
		(*instrument->PlayFromStartHook)(instrument);
		instrument=NextInstrument(instrument);
	}
}

