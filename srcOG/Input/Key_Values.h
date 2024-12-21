#pragma once

#define KEY_SPACE              32
#define KEY_APOSTROPHE         39  /* ' */
#define KEY_COMMA              44  /* , */
#define KEY_MINUS              45  /* - */
#define KEY_PERIOD             46  /* . */
#define KEY_SLASH              47  /* / */
#define KEY_0                  48
#define KEY_1                  49
#define KEY_2                  50
#define KEY_3                  51
#define KEY_4                  52
#define KEY_5                  53
#define KEY_6                  54
#define KEY_7                  55
#define KEY_8                  56
#define KEY_9                  57
#define KEY_SEMICOLON          59  /* ; */
#define KEY_EQUAL              61  /* = */
#define KEY_A                  65
#define KEY_B                  66
#define KEY_C                  67
#define KEY_D                  68
#define KEY_E                  69
#define KEY_F                  70
#define KEY_G                  71
#define KEY_H                  72
#define KEY_I                  73
#define KEY_J                  74
#define KEY_K                  75
#define KEY_L                  76
#define KEY_M                  77
#define KEY_N                  78
#define KEY_O                  79
#define KEY_P                  80
#define KEY_Q                  81
#define KEY_R                  82
#define KEY_S                  83
#define KEY_T                  84
#define KEY_U                  85
#define KEY_V                  86
#define KEY_W                  87
#define KEY_X                  88
#define KEY_Y                  89
#define KEY_Z                  90
#define KEY_LEFT_BRACKET       91  /* [ */
#define KEY_BACKSLASH          92  /* \ */
#define KEY_RIGHT_BRACKET      93  /* ] */
#define KEY_GRAVE_ACCENT       96  /* ` */
#define KEY_WORLD_1            161 /* non-US #1 */
#define KEY_WORLD_2            162 /* non-US #2 */

/* Function keys */
#define KEY_ESCAPE             256
#define KEY_ENTER              257
#define KEY_TAB                258
#define KEY_BACKSPACE          259
#define KEY_INSERT             260
#define KEY_DELETE             261
#define KEY_RIGHT              262
#define KEY_LEFT               263
#define KEY_DOWN               264
#define KEY_UP                 265
#define KEY_PAGE_UP            266
#define KEY_PAGE_DOWN          267
#define KEY_HOME               268
#define KEY_END                269
#define KEY_CAPS_LOCK          280
#define KEY_SCROLL_LOCK        281
#define KEY_NUM_LOCK           282
#define KEY_PRINT_SCREEN       283
#define KEY_PAUSE              284
#define KEY_F1                 290
#define KEY_F2                 291
#define KEY_F3                 292
#define KEY_F4                 293
#define KEY_F5                 294
#define KEY_F6                 295
#define KEY_F7                 296
#define KEY_F8                 297
#define KEY_F9                 298
#define KEY_F10                299
#define KEY_F11                300
#define KEY_F12                301
#define KEY_F13                302
#define KEY_F14                303
#define KEY_F15                304
#define KEY_F16                305
#define KEY_F17                306
#define KEY_F18                307
#define KEY_F19                308
#define KEY_F20                309
#define KEY_F21                310
#define KEY_F22                311
#define KEY_F23                312
#define KEY_F24                313
#define KEY_F25                314
#define KEY_KP_0               320
#define KEY_KP_1               321
#define KEY_KP_2               322
#define KEY_KP_3               323
#define KEY_KP_4               324
#define KEY_KP_5               325
#define KEY_KP_6               326
#define KEY_KP_7               327
#define KEY_KP_8               328
#define KEY_KP_9               329
#define KEY_KP_DECIMAL         330
#define KEY_KP_DIVIDE          331
#define KEY_KP_MULTIPLY        332
#define KEY_KP_SUBTRACT        333
#define KEY_KP_ADD             334
#define KEY_KP_ENTER           335
#define KEY_KP_EQUAL           336
#define KEY_LEFT_SHIFT         340 //16
#define KEY_LEFT_CONTROL       341 //17
#define KEY_LEFT_ALT           342
#define KEY_LEFT_SUPER         343
#define KEY_RIGHT_SHIFT        344
#define KEY_RIGHT_CONTROL      345
#define KEY_RIGHT_ALT          346
#define KEY_RIGHT_SUPER        347
#define KEY_MENU               348
#define KEY_LAST               KEY_MENU

#define KEY_LEFT_SHIFT_GLFW	340					// special case
#define KEY_LEFT_CONTROL_GLFW	341					// special case

#define MOUSE_LEFT				350
#define MOUSE_RIGHT			351

#define KEY_WIN_ENTER        13
#define KEY_WIN_SHIFT         16
#define KEY_WIN_CONTROL       17
#define KEY_WIN_ALT		   18
#define KEY_WIN_TAB           9
#define KEY_WIN_CAPSLOCK      20
#define KEY_WIN_ESCAPE			27

/*#define CONTROLLER_BUTTON_0	352
#define CONTROLLER_BUTTON_1    353
#define CONTROLLER_BUTTON_2    354
#define CONTROLLER_BUTTON_3    355
#define CONTROLLER_BUTTON_4    356
#define CONTROLLER_BUTTON_5    357
#define CONTROLLER_BUTTON_6    358
#define CONTROLLER_BUTTON_7    359
#define CONTROLLER_BUTTON_8    360
#define CONTROLLER_BUTTON_9    361
#define CONTROLLER_BUTTON_10	362
#define CONTROLLER_BUTTON_11    363
#define CONTROLLER_BUTTON_12    364
#define CONTROLLER_BUTTON_13    365
#define CONTROLLER_BUTTON_14    366
#define CONTROLLER_BUTTON_15    367
#define CONTROLLER_BUTTON_16    368
#define CONTROLLER_BUTTON_17    369
#define CONTROLLER_BUTTON_18    370
#define CONTROLLER_BUTTON_19    371*/

#define PS_4_CONTROLLER_CROSS			1	//0
#define PS_4_CONTROLLER_CIRCLE			2	//1
#define PS_4_CONTROLLER_SQUARE			0	//2
#define PS_4_CONTROLLER_TRIANGLE		3
#define PS_4_CONTROLLER_L1			    4
#define PS_4_CONTROLLER_L2			    6
#define PS_4_CONTROLLER_L3			    10	//9
#define PS_4_CONTROLLER_R1			    5
#define PS_4_CONTROLLER_R2			    7
#define PS_4_CONTROLLER_R3			    11	//10
#define PS_4_CONTROLLER_SHARE			6
#define PS_4_CONTROLLER_OPTIONS		9	//7
#define PS_4_CONTROLLER_PS_BUTTON	    12	//8
#define PS_4_CONTROLLER_DPAD_UP		14	//11
#define PS_4_CONTROLLER_DPAD_RIGHT		15	//12
#define PS_4_CONTROLLER_DPAD_DOWN		16	//13
#define PS_4_CONTROLLER_DPAD_LEFT		17	//14
#define PS_4_CONTROLLER_TRIGGER_L		18			// you manually chose this button code so u can store it in the same buttons vector
#define PS_4_CONTROLLER_TRIGGER_R		19			// you manually chose this button code so u can store it in the same buttons vector



#define XBOX_CONTROLLER_A				0
#define XBOX_CONTROLLER_B				1
#define XBOX_CONTROLLER_X				2
#define XBOX_CONTROLLER_Y				3
#define XBOX_CONTROLLER_L1			    4
#define XBOX_CONTROLLER_R1			    5
#define XBOX_CONTROLLER_WIERD_1		6
#define XBOX_CONTROLLER_WIERD_2		7
#define XBOX_CONTROLLER_L3			    8
#define XBOX_CONTROLLER_R3			    9
#define XBOX_CONTROLLER_DPAD_UP		10
#define XBOX_CONTROLLER_DPAD_RIGHT		11
#define XBOX_CONTROLLER_DPAD_DOWN		12
#define XBOX_CONTROLLER_DPAD_LEFT		13
#define XBOX_CONTROLLER_TRIGGER_L		14			// you manually chose this button code so u can store it in the same buttons vector
#define XBOX_CONTROLLER_TRIGGER_R		15			// you manually chose this button code so u can store it in the same buttons vector
