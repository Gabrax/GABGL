#pragma once

#define GAB_KEY_SPACE              32
#define GAB_KEY_APOSTROPHE         39  /* ' */
#define GAB_KEY_COMMA              44  /* , */
#define GAB_KEY_MINUS              45  /* - */
#define GAB_KEY_PERIOD             46  /* . */
#define GAB_KEY_SLASH              47  /* / */
#define GAB_KEY_0                  48
#define GAB_KEY_1                  49
#define GAB_KEY_2                  50
#define GAB_KEY_3                  51
#define GAB_KEY_4                  52
#define GAB_KEY_5                  53
#define GAB_KEY_6                  54
#define GAB_KEY_7                  55
#define GAB_KEY_8                  56
#define GAB_KEY_9                  57
#define GAB_KEY_SEMICOLON          59  /* ; */
#define GAB_KEY_EQUAL              61  /* = */
#define GAB_KEY_A                  65
#define GAB_KEY_B                  66
#define GAB_KEY_C                  67
#define GAB_KEY_D                  68
#define GAB_KEY_E                  69
#define GAB_KEY_F                  70
#define GAB_KEY_G                  71
#define GAB_KEY_H                  72
#define GAB_KEY_I                  73
#define GAB_KEY_J                  74
#define GAB_KEY_K                  75
#define GAB_KEY_L                  76
#define GAB_KEY_M                  77
#define GAB_KEY_N                  78
#define GAB_KEY_O                  79
#define GAB_KEY_P                  80
#define GAB_KEY_Q                  81
#define GAB_KEY_R                  82
#define GAB_KEY_S                  83
#define GAB_KEY_T                  84
#define GAB_KEY_U                  85
#define GAB_KEY_V                  86
#define GAB_KEY_W                  87
#define GAB_KEY_X                  88
#define GAB_KEY_Y                  89
#define GAB_KEY_Z                  90
#define GAB_KEY_LEFT_BRACKET       91  /* [ */
#define GAB_KEY_BACKSLASH          92  /* \ */
#define GAB_KEY_RIGHT_BRACKET      93  /* ] */
#define GAB_KEY_GRAVE_ACCENT       96  /* ` */
#define GAB_KEY_WORLD_1            161 /* non-US #1 */
#define GAB_KEY_WORLD_2            162 /* non-US #2 */

/* Function keys */
#define GAB_KEY_ESCAPE             256
#define GAB_KEY_ENTER              257
#define GAB_KEY_TAB                258
#define GAB_KEY_BACKSPACE          259
#define GAB_KEY_INSERT             260
#define GAB_KEY_DELETE             261
#define GAB_KEY_RIGHT              262
#define GAB_KEY_LEFT               263
#define GAB_KEY_DOWN               264
#define GAB_KEY_UP                 265
#define GAB_KEY_PAGE_UP            266
#define GAB_KEY_PAGE_DOWN          267
#define GAB_KEY_HOME               268
#define GAB_KEY_END                269
#define GAB_KEY_CAPS_LOCK          280
#define GAB_KEY_SCROLL_LOCK        281
#define GAB_KEY_NUM_LOCK           282
#define GAB_KEY_PRINT_SCREEN       283
#define GAB_KEY_PAUSE              284
#define GAB_KEY_F1                 290
#define GAB_KEY_F2                 291
#define GAB_KEY_F3                 292
#define GAB_KEY_F4                 293
#define GAB_KEY_F5                 294
#define GAB_KEY_F6                 295
#define GAB_KEY_F7                 296
#define GAB_KEY_F8                 297
#define GAB_KEY_F9                 298
#define GAB_KEY_F10                299
#define GAB_KEY_F11                300
#define GAB_KEY_F12                301
#define GAB_KEY_F13                302
#define GAB_KEY_F14                303
#define GAB_KEY_F15                304
#define GAB_KEY_F16                305
#define GAB_KEY_F17                306
#define GAB_KEY_F18                307
#define GAB_KEY_F19                308
#define GAB_KEY_F20                309
#define GAB_KEY_F21                310
#define GAB_KEY_F22                311
#define GAB_KEY_F23                312
#define GAB_KEY_F24                313
#define GAB_KEY_F25                314
#define GAB_KEY_KP_0               320
#define GAB_KEY_KP_1               321
#define GAB_KEY_KP_2               322
#define GAB_KEY_KP_3               323
#define GAB_KEY_KP_4               324
#define GAB_KEY_KP_5               325
#define GAB_KEY_KP_6               326
#define GAB_KEY_KP_7               327
#define GAB_KEY_KP_8               328
#define GAB_KEY_KP_9               329
#define GAB_KEY_KP_DECIMAL         330
#define GAB_KEY_KP_DIVIDE          331
#define GAB_KEY_KP_MULTIPLY        332
#define GAB_KEY_KP_SUBTRACT        333
#define GAB_KEY_KP_ADD             334
#define GAB_KEY_KP_ENTER           335
#define GAB_KEY_KP_EQUAL           336
#define GAB_KEY_LEFT_SHIFT         340 //16
#define GAB_KEY_LEFT_CONTROL       17//341
#define GAB_KEY_LEFT_ALT           342
#define GAB_KEY_LEFT_SUPER         343
#define GAB_KEY_RIGHT_SHIFT        344
#define GAB_KEY_RIGHT_CONTROL      345
#define GAB_KEY_RIGHT_ALT          346
#define GAB_KEY_RIGHT_SUPER        347
#define GAB_KEY_MENU               348
#define GAB_KEY_LAST               GAB_KEY_MENU

#define GAB_KEY_LEFT_SHIFT_GLFW	340					// special case
#define GAB_KEY_LEFT_CONTROL_GLFW	341					// special case

#define GAB_MOUSE_LEFT				350
#define GAB_MOUSE_RIGHT			351

#define GAB_KEY_WIN_ENTER        13
#define GAB_KEY_WIN_SHIFT         16
#define GAB_KEY_WIN_CONTROL       17
#define GAB_KEY_WIN_ALT		   18
#define GAB_KEY_WIN_TAB           9
#define GAB_KEY_WIN_CAPSLOCK      20
#define GAB_KEY_WIN_ESCAPE			27

/*#define GAB_CONTROLLER_BUTTON_0	352
#define GAB_CONTROLLER_BUTTON_1    353
#define GAB_CONTROLLER_BUTTON_2    354
#define GAB_CONTROLLER_BUTTON_3    355
#define GAB_CONTROLLER_BUTTON_4    356
#define GAB_CONTROLLER_BUTTON_5    357
#define GAB_CONTROLLER_BUTTON_6    358
#define GAB_CONTROLLER_BUTTON_7    359
#define GAB_CONTROLLER_BUTTON_8    360
#define GAB_CONTROLLER_BUTTON_9    361
#define GAB_CONTROLLER_BUTTON_10	362
#define GAB_CONTROLLER_BUTTON_11    363
#define GAB_CONTROLLER_BUTTON_12    364
#define GAB_CONTROLLER_BUTTON_13    365
#define GAB_CONTROLLER_BUTTON_14    366
#define GAB_CONTROLLER_BUTTON_15    367
#define GAB_CONTROLLER_BUTTON_16    368
#define GAB_CONTROLLER_BUTTON_17    369
#define GAB_CONTROLLER_BUTTON_18    370
#define GAB_CONTROLLER_BUTTON_19    371*/

#define GAB_PS_4_CONTROLLER_CROSS			1	//0
#define GAB_PS_4_CONTROLLER_CIRCLE			2	//1
#define GAB_PS_4_CONTROLLER_SQUARE			0	//2
#define GAB_PS_4_CONTROLLER_TRIANGLE		3
#define GAB_PS_4_CONTROLLER_L1			    4
#define GAB_PS_4_CONTROLLER_L2			    6
#define GAB_PS_4_CONTROLLER_L3			    10	//9
#define GAB_PS_4_CONTROLLER_R1			    5
#define GAB_PS_4_CONTROLLER_R2			    7
#define GAB_PS_4_CONTROLLER_R3			    11	//10
#define GAB_PS_4_CONTROLLER_SHARE			6
#define GAB_PS_4_CONTROLLER_OPTIONS		9	//7
#define GAB_PS_4_CONTROLLER_PS_BUTTON	    12	//8
#define GAB_PS_4_CONTROLLER_DPAD_UP		14	//11
#define GAB_PS_4_CONTROLLER_DPAD_RIGHT		15	//12
#define GAB_PS_4_CONTROLLER_DPAD_DOWN		16	//13
#define GAB_PS_4_CONTROLLER_DPAD_LEFT		17	//14
#define GAB_PS_4_CONTROLLER_TRIGGER_L		18			// you manually chose this button code so u can store it in the same buttons vector
#define GAB_PS_4_CONTROLLER_TRIGGER_R		19			// you manually chose this button code so u can store it in the same buttons vector



#define GAB_XBOX_CONTROLLER_A				0
#define GAB_XBOX_CONTROLLER_B				1
#define GAB_XBOX_CONTROLLER_X				2
#define GAB_XBOX_CONTROLLER_Y				3
#define GAB_XBOX_CONTROLLER_L1			    4
#define GAB_XBOX_CONTROLLER_R1			    5
#define GAB_XBOX_CONTROLLER_WIERD_1		6
#define GAB_XBOX_CONTROLLER_WIERD_2		7
#define GAB_XBOX_CONTROLLER_L3			    8
#define GAB_XBOX_CONTROLLER_R3			    9
#define GAB_XBOX_CONTROLLER_DPAD_UP		10
#define GAB_XBOX_CONTROLLER_DPAD_RIGHT		11
#define GAB_XBOX_CONTROLLER_DPAD_DOWN		12
#define GAB_XBOX_CONTROLLER_DPAD_LEFT		13
#define GAB_XBOX_CONTROLLER_TRIGGER_L		14			// you manually chose this button code so u can store it in the same buttons vector
#define GAB_XBOX_CONTROLLER_TRIGGER_R		15			// you manually chose this button code so u can store it in the same buttons vector