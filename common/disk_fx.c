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
#include "../config/config.h"
#include "disk.h"
#include "patch_proc.h"

#include "disk_fx_proc.h"


void SaveFX(struct FX *fx,struct Tracks *track){
if(fx==NULL) return;
DC_start("FX");
	DC_SaveN(fx->num);
	DC_SSS("name",fx->name);
	DC_SSI("color",fx->color);
	DC_SSI("min",fx->min);
	DC_SSI("max",fx->max);
	DC_SSI("effect_num",fx->effect_num);
        DC_SSN("patchnum",fx->patch->id);

	(*fx->SaveFX)(fx,track);

DC_end();
//SaveFX((struct FX *)fx->l.next,track);
}


struct FX *LoadFX(struct Tracks *track){

	static char *objs[1]={
		"FXDATA"
	};
	static char *vars[6]={
		"name",
		"color",
		"min",
		"max",
                "effect_num",
                "patchnum"
	};
	struct FX *fx=DC_alloc(sizeof(struct FX));
	fx->num = DC_LoadN();
        
        fx->patch = DC_alloc(sizeof(struct Patch)); // temporary object used during loading.
        fx->patch->id = -1; // for loading older songs.
        
	GENERAL_LOAD(0,6)


var0:
	fx->name=DC_LoadSNoMatterWhat();
	goto start;
var1:
	fx->color=DC_LoadI();
	if(dc.colorize==true){
		fx->color=dc.startcolor;
		dc.startcolor++;
		if(dc.startcolor==256){
			dc.startcolor=4;
		}
	}
	goto start;
var2:
	fx->min=DC_LoadI();
	goto start;
var3:
	fx->max=DC_LoadI();
	goto start;
var4:        
	fx->effect_num=DC_LoadI();
        if (DISKVERSION > 0.835)
          R_ASSERT(fx->num == fx->effect_num);

        fx->num = fx->effect_num; // Fix bugs in previous versions. Sometimes fx->effect_num is the only one containing the right value.
	goto start;

var5:
        fx->patch->id = DC_LoadN();
        goto start;
        
obj0:
	fx->fxdata=(*track->patch->instrument->LoadFX)(fx,track);
	goto start;


var6:
var7:
var8:
var9:
var10:
var11:
var12:
var13:
var14:
var15:
var16:
var17:
var18:
var19:
var20:
        
obj1:
obj2:
obj3:
obj4:
obj5:
obj6:

error:
end:

	return fx;
}


void DLoadFX(struct Root *newroot,struct Tracks *track, struct FXs *fxs, struct FX *fx){
  NInt id = fx->patch->id;
  
  if (id==-1)
    fx->patch = track->patch;
  else
    fx->patch = PATCH_get_from_id(fx->patch->id);
}

