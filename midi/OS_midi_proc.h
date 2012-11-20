/* Copyright 2003 Kjetil S. Matheussen

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


#ifndef OS_MIDI_PROC_H
#define OS_MIDI_PROC_H




// OS_midi_spesific.h contains the definition of MidiPortOs
#include <OS_midi_spesific.h>


extern LANGSPEC char **MIDI_getOutputPortOsNames(int *retsize);
extern LANGSPEC char **MIDI_getInputPortOsNames(int *retsize);


// DeleteMidi(midinode);
//	CloseLibrary(CamdBase);
//	UninitMiniCamd();
//	if(inputsig!=-1) FreeSignal(inputsig);
extern LANGSPEC void MIDI_Delete(void);

extern LANGSPEC void MIDI_closeMidiPortOs(MidiPortOs port);

extern LANGSPEC MidiPortOs MIDI_getMidiPortOs(struct Tracker_Windows *window, ReqType reqtype,char *name);
//GoodPutMidi(mymidilink->midilink,(ULONG)((cc<<24)|(data1<<16)|(data2<<8)),(ULONG)maxbuff);

extern LANGSPEC void MIDI_OS_SetInputPort(const char *portname);

extern LANGSPEC void OS_GoodPutMidi(MidiPortOs port,
                                    int cc,
                                    int data1,
                                    int data2,
                                    STime time,
                                    uint32_t maxbuff
                                 );

extern LANGSPEC void OS_PutMidi(MidiPortOs port,
                                int cc,
                                int data1,
                                int data2,
                                STime time
                             );

extern LANGSPEC void OS_PlayFromStart(MidiPortOs port);

extern LANGSPEC bool MIDI_New(struct Instruments *instrument);



#endif
