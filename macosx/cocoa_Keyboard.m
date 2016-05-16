/* Copyright 2012 Kjetil S. Matheussen

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


#ifdef FOR_MACOSX

#import <Cocoa/Cocoa.h>
#import <Carbon/Carbon.h>


#undef EVENT_H

#include "../common/nsmtracker.h"
#include "../common/eventreciever_proc.h"

#include "../common/OS_system_proc.h"


extern struct TEvent tevent;
extern struct Root *root;


static bool modifiers[64] = {false}; // These are used to keep track of whether it's a key up or key down event when pressing a modifier key. Unfortunately we get NSFlagsChanged events for modifier keys.

static void clear_modifiers(void){
  int i;
  for(i=0;i<64;i++)
    modifiers[i]=false;

  OS_SYSTEM_ResetKeysUpDowns(); // Sync
}


// https://developer.apple.com/library/mac/documentation/Cocoa/Reference/ApplicationKit/Classes/NSEvent_Class

int OS_SYSTEM_get_event_type(void *void_event, bool ignore_autorepeat){
  NSEvent *event = (NSEvent *)void_event;
  NSEventType type = [event type];
  int ret = -1;
  
  if(type==NSFlagsChanged || type==NSKeyDown || type==NSKeyUp){

#if 0
    if (type==NSFlagsChanged)
      printf("type: %d. keycode: %d. modifiers: %d\n",(int)type, [event keyCode],(int)[event modifierFlags]);
    else
      printf("type: %d. keycode: %d. modifiers: %d. autorepeat: %d\n",(int)type, [event keyCode], (int)[event modifierFlags], [event isARepeat]);
#endif
    
    if(type==NSFlagsChanged){
      int keycode = [event keyCode];
      
      if (modifiers[keycode])
        ret = TR_KEYBOARDUP;
      else
        ret = TR_KEYBOARD;
      
      modifiers[keycode] = !modifiers[keycode]; // This seems a bit fragile, so all modifiers are also reset when modifierFlags is 0 for all the osx modifiers. (see below)
      
      //printf("   modifier is %s\n",(ret==TR_KEYBOARDUP)?"released":"pressed");
      
    }else if(type==NSKeyDown)
      ret = TR_KEYBOARD;
    
    else if (type==NSKeyUp)
      ret = TR_KEYBOARDUP;

    
    // Probably not necessary, but just in case, in case things get out sync (see above)
    // WARNING: [event modifierFlags] is resetted when clicking mouse.
    if ( ([event modifierFlags] & NSCommandKeyMask) == 0 &&
         ([event modifierFlags] & NSAlphaShiftKeyMask) == 0 &&
         ([event modifierFlags] & NSShiftKeyMask) == 0 &&
         ([event modifierFlags] & NSAlternateKeyMask) == 0 &&
         ([event modifierFlags] & NSControlKeyMask) == 0 &&
         ([event modifierFlags] & NSFunctionKeyMask) == 0
         )
      {
        //printf("****** No modifiers. Resetting\n");
        // Since modifierFlags are resetted when clicking mouse, perhaps it's better not to do this. (no, tried it, didn't work)
        clear_modifiers();
      }
  }

  return ret;
}

int OS_SYSTEM_get_keycode(void *void_event){
  NSEvent *event = (NSEvent *)void_event;
  return [event keyCode];
}
                             
int OS_SYSTEM_get_modifier(void *void_event){

  NSEvent *event = (NSEvent *)void_event;
  
  switch ([event keyCode]) {
    //case 54: // Right Command (we don't use this one anymore, it's usually not available, and the "menu" key is a bit work to turn into a modifier key)
    //return EVENT_EXTRA_R;
  case 55: // Left Command
    return EVENT_EXTRA_L;
  case 57: // Capslock
    return EVENT_CAPS;
  case 56: // Left Shift
    return EVENT_SHIFT_L;
  case 60: // Right Shift
    return EVENT_SHIFT_R;
  case 58: // Left Alt
    return EVENT_ALT_L;
  case 61: // Right Alt
    return EVENT_ALT_R;
  case 59: // Left Ctrl
    return EVENT_CTRL_L;
  case 62: // Right Ctrl
    return EVENT_CTRL_R;
    //case 63: // Function
  default:
    return EVENT_NO;
  }
}


static int keymap[0x100] = {EVENT_NO};
static int keymap_qwerty[0x100] = {EVENT_NO};

// This only works for qwerty-keyboards (i.e. what scancode is used for in windows and linux). To get EVENT_A (for instance): http://stackoverflow.com/questions/8263618/convert-virtual-key-code-to-unicode-string
// Mac vk overview: http://stackoverflow.com/questions/3202629/where-can-i-find-a-list-of-mac-virtual-key-codes
static void init_keymaps(void){
  // alpha
  keymap_qwerty[kVK_ANSI_A] = EVENT_QWERTY_A;
  keymap_qwerty[kVK_ANSI_B] = EVENT_QWERTY_B;
  keymap_qwerty[kVK_ANSI_C] = EVENT_QWERTY_C;
  keymap_qwerty[kVK_ANSI_D] = EVENT_QWERTY_D;
  keymap_qwerty[kVK_ANSI_E] = EVENT_QWERTY_E;
  keymap_qwerty[kVK_ANSI_F] = EVENT_QWERTY_F;
  keymap_qwerty[kVK_ANSI_G] = EVENT_QWERTY_G;
  keymap_qwerty[kVK_ANSI_H] = EVENT_QWERTY_H;
  keymap_qwerty[kVK_ANSI_I] = EVENT_QWERTY_I;
  keymap_qwerty[kVK_ANSI_J] = EVENT_QWERTY_J;
  keymap_qwerty[kVK_ANSI_K] = EVENT_QWERTY_K;
  keymap_qwerty[kVK_ANSI_L] = EVENT_QWERTY_L;
  keymap_qwerty[kVK_ANSI_M] = EVENT_QWERTY_M;
  keymap_qwerty[kVK_ANSI_N] = EVENT_QWERTY_N;
  keymap_qwerty[kVK_ANSI_O] = EVENT_QWERTY_O;
  keymap_qwerty[kVK_ANSI_P] = EVENT_QWERTY_P;
  keymap_qwerty[kVK_ANSI_Q] = EVENT_QWERTY_Q;
  keymap_qwerty[kVK_ANSI_R] = EVENT_QWERTY_R;
  keymap_qwerty[kVK_ANSI_S] = EVENT_QWERTY_S;
  keymap_qwerty[kVK_ANSI_T] = EVENT_QWERTY_T;
  keymap_qwerty[kVK_ANSI_U] = EVENT_QWERTY_U;
  keymap_qwerty[kVK_ANSI_V] = EVENT_QWERTY_V;
  keymap_qwerty[kVK_ANSI_W] = EVENT_QWERTY_W;
  keymap_qwerty[kVK_ANSI_X] = EVENT_QWERTY_X;
  keymap_qwerty[kVK_ANSI_Y] = EVENT_QWERTY_Y;
  keymap_qwerty[kVK_ANSI_Z] = EVENT_QWERTY_Z;

  // num
  keymap_qwerty[kVK_ANSI_0] = EVENT_QWERTY_0;
  keymap_qwerty[kVK_ANSI_1] = EVENT_QWERTY_1;
  keymap_qwerty[kVK_ANSI_2] = EVENT_QWERTY_2;
  keymap_qwerty[kVK_ANSI_3] = EVENT_QWERTY_3;
  keymap_qwerty[kVK_ANSI_4] = EVENT_QWERTY_4;
  keymap_qwerty[kVK_ANSI_5] = EVENT_QWERTY_5;
  keymap_qwerty[kVK_ANSI_6] = EVENT_QWERTY_6;
  keymap_qwerty[kVK_ANSI_7] = EVENT_QWERTY_7;
  keymap_qwerty[kVK_ANSI_8] = EVENT_QWERTY_8;
  keymap_qwerty[kVK_ANSI_9] = EVENT_QWERTY_9;

  // row 1
  keymap[kVK_Escape] = EVENT_ESC;
  keymap[kVK_F1] = EVENT_F1;
  keymap[kVK_F2] = EVENT_F2;
  keymap[kVK_F3] = EVENT_F3;
  keymap[kVK_F4] = EVENT_F4;
  keymap[kVK_F5] = EVENT_F5;
  keymap[kVK_F6] = EVENT_F6;
  keymap[kVK_F7] = EVENT_F7;
  keymap[kVK_F8] = EVENT_F8;
  keymap[kVK_F9] = EVENT_F9;
  keymap[kVK_F10] = EVENT_F10;
  keymap[kVK_F11] = EVENT_F11;
  keymap[kVK_F12] = EVENT_F12;

  // row 2
  keymap_qwerty[kVK_ISO_Section] = EVENT_1L1;
  keymap_qwerty[kVK_ANSI_Grave]  = EVENT_1L1;
  keymap_qwerty[kVK_ANSI_Minus] = EVENT_0R1;
  keymap_qwerty[kVK_ANSI_Equal] = EVENT_0R2;
  keymap_qwerty[kVK_Delete] = EVENT_BACKSPACE;

  // row 3
  keymap[kVK_Tab] = EVENT_TAB;
  keymap_qwerty[kVK_ANSI_LeftBracket] = EVENT_PR1;
  keymap_qwerty[kVK_ANSI_RightBracket] = EVENT_PR2;
  keymap[kVK_Return] = EVENT_RETURN;

  // row 4
  keymap_qwerty[kVK_ANSI_Semicolon] = EVENT_LR1;
  keymap_qwerty[kVK_ANSI_Quote] = EVENT_LR2;
  keymap_qwerty[kVK_ANSI_Backslash] = EVENT_LR3;

  // row 5
  keymap_qwerty[kVK_ANSI_Grave]    = EVENT_ZL1;
  keymap_qwerty[kVK_ANSI_Comma]  = EVENT_MR1;
  keymap_qwerty[kVK_ANSI_Period] = EVENT_MR2;
  keymap_qwerty[kVK_ANSI_Slash]      = EVENT_MR3;

  // row 6
  keymap[kVK_Space] = EVENT_SPACE;
  keymap[54] = EVENT_MENU; // I.e. EXTRA_R on mac keyboards
  keymap[110] = EVENT_MENU; // I.e. Menu key on pc keyboards
  
  // insert/del/etc.
  keymap[kVK_Help] = EVENT_INSERT;
  keymap[kVK_Home] = EVENT_HOME;
  keymap[kVK_PageUp] = EVENT_PAGE_UP;
  keymap[kVK_PageDown] = EVENT_PAGE_DOWN;
  keymap[kVK_ForwardDelete] = EVENT_DEL;
  keymap[kVK_End] = EVENT_END;

  // arrows
  keymap[kVK_LeftArrow]  = EVENT_LEFTARROW;
  keymap[kVK_UpArrow]    = EVENT_UPARROW;
  keymap[kVK_RightArrow] = EVENT_RIGHTARROW;
  keymap[kVK_DownArrow]  = EVENT_DOWNARROW;

  // keypad
  keymap[kVK_ANSI_Keypad0] = EVENT_KP_0;
  keymap[kVK_ANSI_Keypad1] = EVENT_KP_1;
  keymap[kVK_ANSI_Keypad2] = EVENT_KP_2;
  keymap[kVK_ANSI_Keypad3] = EVENT_KP_3;
  keymap[kVK_ANSI_Keypad4] = EVENT_KP_4;
  keymap[kVK_ANSI_Keypad5] = EVENT_KP_5;
  keymap[kVK_ANSI_Keypad6] = EVENT_KP_6;
  keymap[kVK_ANSI_Keypad7] = EVENT_KP_7;
  keymap[kVK_ANSI_Keypad8] = EVENT_KP_8;
  keymap[kVK_ANSI_Keypad9] = EVENT_KP_9;
  keymap[kVK_ANSI_KeypadDecimal] = EVENT_KP_DOT;
  keymap[kVK_ANSI_KeypadEnter] = EVENT_KP_ENTER;
  keymap[kVK_ANSI_KeypadPlus] = EVENT_KP_ADD;
  keymap[kVK_ANSI_KeypadMinus] = EVENT_KP_SUB;
  keymap[kVK_ANSI_KeypadMultiply] = EVENT_KP_MUL;
  keymap[kVK_ANSI_KeypadDivide] = EVENT_KP_DIV;
}

// copied some code from here: http://stackoverflow.com/questions/8263618/convert-virtual-key-code-to-unicode-string
static char get_char_from_keycode(int keyCode){
  static TISInputSourceRef currentKeyboard;
  static CFDataRef uchr;
  static const UCKeyboardLayout *keyboardLayout = NULL;

  const bool reload_keyboard_every_time = true;

  if (reload_keyboard_every_time || keyboardLayout==NULL){
    currentKeyboard = TISCopyCurrentKeyboardInputSource();
    uchr = (CFDataRef)TISGetInputSourceProperty(currentKeyboard, kTISPropertyUnicodeKeyLayoutData);
    keyboardLayout = (const UCKeyboardLayout*)CFDataGetBytePtr(uchr);
  }
  
  if(keyboardLayout) {
    UInt32 deadKeyState = 0;
    UniCharCount maxStringLength = 255;
    UniCharCount actualStringLength = 0;
    UniChar unicodeString[maxStringLength];
    
    OSStatus status = UCKeyTranslate(keyboardLayout,
                                     keyCode, kUCKeyActionDown, 0,
                                     LMGetKbdType(), 0,
                                     &deadKeyState,
                                     maxStringLength,
                                     &actualStringLength,
                                     unicodeString);
    
    if (actualStringLength == 0 && deadKeyState) {
      status = UCKeyTranslate(keyboardLayout,
                              kVK_Space, kUCKeyActionDown, 0,
                              LMGetKbdType(), 0,
                              &deadKeyState,
                              maxStringLength,
                              &actualStringLength,
                              unicodeString);   
    }
    
    if(actualStringLength > 0 && status == noErr) {
      //NSString *hepp = [[NSString stringWithCharacters:unicodeString length:(NSUInteger)actualStringLength] uppercaseString];
      //printf("      ______________ some code: %s. %c\n",[hepp cStringUsingEncoding:NSUTF8StringEncoding],unicodeString[0]);
      return unicodeString[0];
    }
  }

  return -1;
}

int OS_SYSTEM_get_keynum(void *void_event){
  NSEvent *event = (NSEvent *)void_event;

  int keycode = [event keyCode];

  if(keycode >= 0x100)
    return EVENT_NO;

  int ret = keymap[keycode];
  if (ret != EVENT_NO)
    return ret;

  char c = toupper(get_char_from_keycode(keycode));

  switch(c){
    case 'A':
      return EVENT_A;
    case 'B':
      return EVENT_B;
    case 'C':
      return EVENT_C;
    case 'D':
      return EVENT_D;
    case 'E':
      return EVENT_E;
    case 'F':
      return EVENT_F;
    case 'G':
      return EVENT_G;
    case 'H':
      return EVENT_H;
    case 'I':
      return EVENT_I;
    case 'J':
      return EVENT_J;
    case 'K':
      return EVENT_K;
    case 'L':
      return EVENT_L;
    case 'M':
      return EVENT_M;
    case 'N':
      return EVENT_N;
    case 'O':
      return EVENT_O;
    case 'P':
      return EVENT_P;
    case 'Q':
      return EVENT_Q;
    case 'R':
      return EVENT_R;
    case 'S':
      return EVENT_S;
    case 'T':
      return EVENT_T;
    case 'U':
      return EVENT_U;
    case 'V':
      return EVENT_V;
    case 'W':
      return EVENT_W;
    case 'X':
      return EVENT_X;
    case 'Y':
      return EVENT_Y;
    case 'Z':
      return EVENT_Z;


    case '0':
      return EVENT_0;
    case '1':
      return EVENT_1;
    case '2':
      return EVENT_2;
    case '3':
      return EVENT_3;
    case '4':
      return EVENT_4;
    case '5':
      return EVENT_5;
    case '6':
      return EVENT_6;
    case '7':
      return EVENT_7;
    case '8':
      return EVENT_8;
    case '9':
      return EVENT_9;
      
    default:
      return EVENT_NO;
  }
}

int OS_SYSTEM_get_qwerty_keynum(void *void_event){
  NSEvent *event = (NSEvent *)void_event;

  int keycode = [event keyCode];

  return keymap_qwerty[keycode];
}


void OS_SYSTEM_init_keyboard(void){
  static bool has_inited = false;
  if (has_inited==false){
    init_keymaps();
    clear_modifiers();
    has_inited=true;
  }
}


void OS_SYSTEM_EventPreHandler(void *void_event){
  NSEvent *event = (NSEvent *)void_event;
  NSEventType type = [event type];

  OS_SYSTEM_init_keyboard();

  //printf("Got event. type: %u\n",(unsigned int)type);

  static void *oldHotKeyMode = NULL;
  if(type==NSAppKitDefined || type==NSSystemDefined || type==NSApplicationDefined){ // These three events are received when losing focus. Haven't found a better time to clear modifiers.
    //printf("      DAS EVENT: %x\n",(unsigned int)type);
    clear_modifiers();
    if(oldHotKeyMode!=NULL){
      PushSymbolicHotKeyMode(kHIHotKeyModeAllEnabled);
      oldHotKeyMode = NULL;
    }

    call_me_if_another_window_may_have_taken_focus_but_still_need_our_key_events();
    
    return;
  }else{
    if(oldHotKeyMode==NULL)
      oldHotKeyMode = PushSymbolicHotKeyMode(kHIHotKeyModeAllDisabled); 
  }
}


#endif
