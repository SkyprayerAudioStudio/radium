/************************************************************************
 ************************************************************************
    FAUST Architecture File
	Copyright (C) 2003-2011 GRAME, Centre National de Creation Musicale
    ---------------------------------------------------------------------

	This is sample code. This file is provided as an example of minimal
	FAUST architecture file. Redistribution and use in source and binary
	forms, with or without modification, in part or in full are permitted.
	In particular you can create a derived work of this FAUST architecture
	and distribute that work under terms of your choice.

	This sample code is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 ************************************************************************
 ************************************************************************/

#include <cmath>
#include <string.h>

#include "faust/gui/GUI.h"
#include "faust/dsp/dsp.h"
#include "faust/gui/meta.h"
#include "faust/gui/jsonfaustui.h"
#include "faust/gui/JSONUI.h"

using namespace std;

/******************************************************************************
*******************************************************************************

							       VECTOR INTRINSICS

*******************************************************************************
*******************************************************************************/

<<includeIntrinsic>>

/******************************************************************************
*******************************************************************************

			ABSTRACT USER INTERFACE

*******************************************************************************
*******************************************************************************/

//----------------------------------------------------------------------------
//  FAUST generated signal processor
//----------------------------------------------------------------------------

<<includeclass>>

mydsp DSP;

int main(int argc, char *argv[])
{
    /*
    httpdfaust::jsonfaustui json("", "", 0);
    DSP.buildUserInterface(&json);
    mydsp::metadata(&json);
    json.numInput(DSP.getNumInputs());
    json.numOutput(DSP.getNumOutputs());
    cout << json.json();
    */
    
    JSONUI json(DSP.getNumInputs(), DSP.getNumOutputs());
    // Add metadata before UI is mandatory for proper JSONUI functionning
    mydsp::metadata(&json);
    DSP.buildUserInterface(&json);
    cout << json.JSON();
}
