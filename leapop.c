/* SPDX-License-Identifier: GPL-3.0-or-later */
/* leapop v1.0 (November 2022)
 * Copyright (C) 2017-2022 Norbert de Jonge <nlmdejonge@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program. If not, see [ www.gnu.org/licenses/ ].
 *
 * To properly read this code, set your program's tab stop to: 2.
 */

/*========== Includes ==========*/
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <dirent.h>
#include <ctype.h>
#include <time.h>
#include <math.h>
#include <limits.h>
#if defined WIN32 || _WIN32 || WIN64 || _WIN64
#include <windows.h>
#undef PlaySound
#endif

#include <SDL.h>
#include <SDL_audio.h>
#include <SDL_thread.h>
#include <SDL_ttf.h>
#include <SDL_image.h>
/*========== Includes ==========*/

/*========== Defines ==========*/
#if defined WIN32 || _WIN32 || WIN64 || _WIN64
#define SLASH "\\"
#define DEVNULL "NUL"
#else
#define SLASH "/"
#define DEVNULL "/dev/null"
#endif

#define EXIT_NORMAL 0
#define EXIT_ERROR 1
#define EDITOR_NAME "leapop"
#define EDITOR_VERSION "v1.0 (November 2022)"
#define COPYRIGHT "Copyright (C) 2022 Norbert de Jonge"
#define LEVEL_SIZE 2304
#define LEVELS 15
#define ROOMS 24
#define TILES 30
#define EVENTS 256
#define DISK_DIR_A "disk_appleii"
#define BACKUP_A DISK_DIR_A SLASH "disk.bak"
#define DISK_DIR_B "disk_bbcmaster"
#define BACKUP_B DISK_DIR_B SLASH "disk.bak"
#define DISK_DIR_C "disk_c64"
#define BACKUP_C DISK_DIR_C SLASH "disk.bak"
#define MAX_PATHFILE 200
#define MAX_TOWRITE 720
#define WINDOW_WIDTH 560 + 2 + 50 /*** 612 ***/
#define WINDOW_HEIGHT 384 + 2 + 75 /*** 461 ***/
#define MAX_IMG 200
#define MAX_CON 30
#define REFRESH_PROG 25 /*** That is 40 fps (1000/25). ***/
#define REFRESH_GAME 50 /*** That is 20 fps (1000/50). ***/
#define FONT_SIZE_15 15
#define FONT_SIZE_11 11
#define FONT_SIZE_20 20
#define NUM_SOUNDS 20 /*** Sounds that may play at the same time. ***/
#define MAX_TEXT 100
#define ADJ_BASE_X 339
#define ADJ_BASE_Y 63
#define MAX_OPTION 100
#define MAX_WARNING 200
#define MAX_ERROR 200
#define MAX_INFO 200
#define TABS_GUARD 8
#define TABS_LEVEL 15
#define BAR_FULL 437

/*** Apple II: adamgreen (A0) ***/
#define A0_PRODOS_OFFSET_1 0x103
#define A0_PRODOS_OFFSET_2 0x42C
#define A0_PRODOS_TEXT "PRODOS"
#define A0_POP_OFFSET_1 0x12E6
#define A0_POP_OFFSET_2 0x29600
#define A0_POP_TEXT "Prince of Persia"
/*** Apple II: peterferrie (A1) ***/
#define A1_PRODOS_OFFSET_1 0x103
#define A1_PRODOS_OFFSET_2 0x42C
#define A1_PRODOS_TEXT "PRODOS"
#define A1_POP_OFFSET_1 0x405
#define A1_POP_OFFSET_2 0x47A
#define A1_POP_TEXT "PRINCEUNP"
/*** BBC Master: kieranhj 1.0 (B0) ***/
#define B0_POPBBCM_OFFSET 0
#define B0_POPBBCM_TEXT "POP BEEB"
#define B0_VANDB_OFFSET 0x26F0
/*** v1.0 2018-03-29 22:00 kc ***/
#define B0_VANDB_TEXT "\x10\x18\x03\x29\x22\x00\x6B\x63"
/*** BBC Master: kieranhj 1.1 (B1) ***/
#define B1_POPBBCM_OFFSET 0
#define B1_POPBBCM_TEXT "POP BEEB"
#define B1_VANDB_OFFSET 0x26F0
/*** v1.1 2018-04-01 20:35 kc ***/
#define B1_VANDB_TEXT "\x11\x18\x04\x01\x20\x35\x6B\x63"
/*** C64: mrsid (C0) ***/
#define C0_C64CART_OFFSET 0x00
#define C0_C64CART_TEXT "C64 CARTRIDGE"
#define C0_DATE_OFFSET 0x688C1
#define C0_DATE_TEXT "05/11/2011"

#define BROKEN_ROOM_X 355
#define BROKEN_ROOM_Y 79
#define BROKEN_LEFT_X 340
#define BROKEN_LEFT_Y 79
#define BROKEN_RIGHT_X 370
#define BROKEN_RIGHT_Y 79
#define BROKEN_UP_X 355
#define BROKEN_UP_Y 64
#define BROKEN_DOWN_X 355
#define BROKEN_DOWN_Y 94

#define PNG_VARIOUS "various"
#define PNG_LIVING "living"
#define PNG_SLIVING "sliving"
#define PNG_BUTTONS "buttons"
#define PNG_EXTRAS "extras"
#define PNG_ROOMS "rooms"
#define PNG_GAMEPAD "gamepad"

#define OFFSETD_X 25 - 1 /*** Left column, pixels from the left. ***/
#define OFFSETD_Y 50 + 6 + 2 /*** Top row, pixels from the top. ***/
#define TTPD_1 20 /*** Top row, pixels behind interface/floor. ***/
#define TTPD_O 20 /*** Other rows, pixels behind superjacent rows. ***/
#define DD_X 56 /*** Horizontal distance between (overlapping) tiles. ***/
#define DD_Y 126 /*** Vertical distance between (overlapping) tiles. ***/

#define TILEWIDTH 42 /*** On tiles screen. ***/
#define TILEHEIGHT 52 /*** On tiles screen. ***/
#define TILESX1 (TILEWIDTH + 2) * 0
#define TILESX2 (TILEWIDTH + 2) * 1
#define TILESX3 (TILEWIDTH + 2) * 2
#define TILESX4 (TILEWIDTH + 2) * 3
#define TILESX5 (TILEWIDTH + 2) * 4
#define TILESX6 (TILEWIDTH + 2) * 5
#define TILESX7 (TILEWIDTH + 2) * 6
#define TILESX8 (TILEWIDTH + 2) * 7
#define TILESX9 (TILEWIDTH + 2) * 8
#define TILESX10 (TILEWIDTH + 2) * 9
#define TILESX11 (TILEWIDTH + 2) * 10
#define TILESX12 (TILEWIDTH + 2) * 11
#define TILESX13 (TILEWIDTH + 2) * 12
#define TILESY1 2 + (TILEHEIGHT + 2) * 0
#define TILESY2 2 + (TILEHEIGHT + 2) * 1
#define TILESY3 2 + (TILEHEIGHT + 2) * 2
#define TILESY4 2 + (TILEHEIGHT + 2) * 3
#define TILESY5 2 + (TILEHEIGHT + 2) * 4
#define TILESY6 2 + (TILEHEIGHT + 2) * 5
#define TILESY7 2 + (TILEHEIGHT + 2) * 6

#ifndef O_BINARY
#define O_BINARY 0
#endif
/*========== Defines ==========*/

int iDebug;
unsigned char arLevel[LEVEL_SIZE + 2];
int iLevelRead;
char sPathFileA[MAX_PATHFILE + 2];
char sPathFileB[MAX_PATHFILE + 2];
char sPathFileC[MAX_PATHFILE + 2];
char sPathFile[MAX_PATHFILE + 2];
int iChanged;
int iScreen;
TTF_Font *font1;
TTF_Font *font2;
TTF_Font *font3;
SDL_Window *window;
SDL_Renderer *ascreen;
int iScale;
int iFullscreen;
SDL_Cursor *curArrow;
SDL_Cursor *curWait;
SDL_Cursor *curHand;
int iNoAudio;
int iNoController;
int iPreLoaded;
int iNrToPreLoad;
int iCurrentBarHeight;
int iDownAt;
int iSelected;
int iChangeEvent;
int iGuardType;
int iCurLevel;
int iExtras;
int arBrokenRoomLinks[LEVELS + 2];
int iCurRoom;
int iMovingRoom;
int iMovingNewBusy;
int iChangingBrokenRoom;
int iChangingBrokenSide;
int iLastX, iLastTile, iLastMod;
int iXPos, iYPos;
int iInfo;
int arMovingRooms[ROOMS + 1 + 2][ROOMS + 2];
unsigned int gamespeed;
Uint32 looptime;
char cCurType;
int iCurGuard;
int arDone[ROOMS + 2];
int iStartRoomsX, iStartRoomsY;
int iMovingNewX, iMovingNewY;
int iMinX, iMaxX, iMinY, iMaxY;
int iMovingOldX, iMovingOldY;
int arRoomConnectionsBroken[LEVELS + 2][ROOMS + 2][4 + 2];
int iOnTile;
int iOnTileOld;
Uint32 ontile;
int iCloseOn;
int iHelpOK;
int iEXESave;
int iOKOn;
int iYesOn;
int iNoOn;
int iCopied;
int iStartLevel;
int iCustomX, iCustomTile, iCustomMod;
int iCustomHover, iCustomHoverOld;
int iEmulator;
char sInfo[MAX_INFO + 2];
int iNoAnim;
int iFlameFrame;
int cChecksum;
Uint32 oldticks, newticks;
int iMouse;
int iGuardTooltip;
int iEventHover;
int iHomeComputer;
int iDiskImageA;
int iDiskImageB;
int iDiskImageC;
int iAppleII;
int iOnAppleII;
int iBBCMaster;
int iOnBBCMaster;
int iC64;
int iOnC64;
int iHomeComputerActive;
int iModified;

/*** EXE ***/
int iEXEPrinceHP;
int iEXEShadowHP;
int iEXEChomperDelay;
int iEXEMouseDelay;
static const int arDefaultGuard[][12] = {
	{ 0x4B, 0x64, 0x4B, 0x4B, 0x4B, 0x32, 0x64, 0xDC, 0x00, 0x3C, 0x28, 0x3C },
	{ 0x00, 0x00, 0x00, 0x05, 0x05, 0xAF, 0x14, 0x0A, 0x00, 0xFF, 0xFF, 0x96 },
	{ 0x00, 0x96, 0x96, 0xC8, 0xC8, 0xFF, 0xC8, 0xFA, 0x00, 0xFF, 0xFF, 0xFF },
	{ 0x00, 0x4B, 0x4B, 0x64, 0x64, 0x91, 0x64, 0xFA, 0x00, 0x91, 0xFF, 0xAF },
	{ 0xFF, 0xC8, 0xC8, 0xC8, 0xFF, 0xFF, 0xC8, 0x00, 0x00, 0xFF, 0x64, 0x64 },
	{ 0x14, 0x14, 0x14, 0x14, 0x0A, 0x0A, 0x0A, 0x0A, 0x00, 0x0A, 0x00, 0x00 },
	{ 0x00, 0x00, 0x00, 0x01, 0x00, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x01 },
	{ 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }
};
static const int arDefaultGuardHP[TABS_LEVEL] = { 4, 3, 3, 3, 3, 4, 5, 4, 4, 5, 5, 5, 4, 6, 1 }; /*** Last value is not used. ***/
static const int arDefaultGuardU[TABS_LEVEL] = { 1, 0, 0, 0, 1, 1, 1, 0, 0, 0, 1, 1, 0, 0, 96 }; /*** Last value is not used. ***/
static const int arDefaultGuardS[TABS_LEVEL] = { 0, 0, 0, 1, 2, 2, 3, 2, 2, 2, 2, 2, 4, 5, 5 }; /*** Last value is not used. ***/
static const int arDefaultEnv1[TABS_LEVEL] = { 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x02, 0x02, 0x02, 0x01, 0x01, 0x02, 0x02, 0x01 };
static const int arDefaultEnv2[TABS_LEVEL] = { 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x02, 0x02, 0x02, 0x01, 0x01, 0x02, 0x02, 0x01 };

/* The following offsets are for:
 * ...A[] = { adamgreen (A0), peterferrie (A1) }
 * ...B[] = { kieranhj 1.0 (B0), kieranhj 1.1 (B1) }
 * ...C[] = { mrsid (C0) }
 * 0x00 = unused
 */
static const unsigned long arLevelOffsetsA[][15] = /*** 0 - 14 ***/
{
	{
		0x26900 + (1 * 0x900),
		0x26900 + (2 * 0x900),
		0x26900 + (0 * 0x900),
		0x49000 + (10 * 0x900),
		0x49000 + (11 * 0x900),
		0x49000 + (8 * 0x900),
		0x49000 + (9 * 0x900),
		0x49000 + (6 * 0x900),
		0x49000 + (7 * 0x900),
		0x49000 + (4 * 0x900),
		0x49000 + (5 * 0x900),
		0x49000 + (2 * 0x900),
		0x49000 + (3 * 0x900),
		0x49000 + (0 * 0x900),
		0x49000 + (1 * 0x900)
	},
	{
		0x28D00 + (1 * 0x900),
		0x28D00 + (2 * 0x900),
		0x28D00 + (0 * 0x900),
		0x48A00 + (10 * 0x900),
		0x48A00 + (11 * 0x900),
		0x48A00 + (8 * 0x900),
		0x48A00 + (9 * 0x900),
		0x48A00 + (6 * 0x900),
		0x48A00 + (7 * 0x900),
		0x48A00 + (4 * 0x900),
		0x48A00 + (5 * 0x900),
		0x48A00 + (2 * 0x900),
		0x48A00 + (3 * 0x900),
		0x48A00 + (0 * 0x900),
		0x48A00 + (1 * 0x900)
	}
};
static const unsigned long arLevelOffsetsB[][15] = /*** 0 - 14 ***/
{
	{
		0x24200 + (0 * 0xA00),
		0x24200 + (1 * 0xA00),
		0x24200 + (2 * 0xA00),
		0x24200 + (3 * 0xA00),
		0x24200 + (4 * 0xA00),
		0x24200 + (5 * 0xA00),
		0x24200 + (6 * 0xA00),
		0x24200 + (7 * 0xA00),
		0x24200 + (8 * 0xA00),
		0x24200 + (9 * 0xA00),
		0x24200 + (10 * 0xA00),
		0x24200 + (11 * 0xA00),
		0x24200 + (12 * 0xA00),
		0x24200 + (13 * 0xA00),
		0x24200 + (14 * 0xA00)
	},
	{
		0x24200 + (0 * 0xA00),
		0x24200 + (1 * 0xA00),
		0x24200 + (2 * 0xA00),
		0x24200 + (3 * 0xA00),
		0x24200 + (4 * 0xA00),
		0x24200 + (5 * 0xA00),
		0x24200 + (6 * 0xA00),
		0x24200 + (7 * 0xA00),
		0x24200 + (8 * 0xA00),
		0x24200 + (9 * 0xA00),
		0x24200 + (10 * 0xA00),
		0x24200 + (11 * 0xA00),
		0x24200 + (12 * 0xA00),
		0x24200 + (13 * 0xA00),
		0x24200 + (14 * 0xA00)
	}
};
static const unsigned long arLevelOffsetsC[][15] = /*** 0 - 14 ***/
{
	{
		0x00,
		0x100D0,
		0x109D0,
		0x112D0,
		0x140F0,
		0x149F0,
		0x152F0,
		0x18110,
		0x18A10,
		0x19310,
		0x1C130,
		0x1CA30,
		0x1D330,
		0x20150,
		0x20A50
	}
};
/* In theory, ulStartLevelA for A1 could be set to 0x7343, but this only
 * seems to work for level 2. Perhaps because of packing. Additionally,
 * 0x731A might skip the intro. And the ProDOS info, by changing 9D to BD at
 * 0x1638, 0x1643, 0x164E, 0x1659.
 */
static const unsigned long ulSkipIntroA[] = { 0x00, 0x00 };
static const unsigned long ulSkipIntroB[] = { 0x428, 0x428 };
static const unsigned long ulSkipIntroC[] = { 0x00 };
static const unsigned long ulStartLevelA[] = { 0x00, 0x00 };
static const unsigned long ulStartLevelB[] = { 0x12F9, 0x12F9 };
static const unsigned long ulStartLevelC[] = { 0x00 };
static const unsigned long ulSavedLevelA[] = { 0x43600, 0x00 };
static const unsigned long ulSavedLevelB[] = { 0x00, 0x00 };
static const unsigned long ulSavedLevelC[] = { 0x00 };
/*** A5 7C 30 04 A9 40 85 7D ***/
static const unsigned long ulCopyProtA[] = { 0x19497, 0x1BA97 };
static const unsigned long ulCopyProtB[] = { 0x00, 0x00 };
static const unsigned long ulCopyProtC[] = { 0x00 };
static const unsigned long ulPrinceHPA[] = { 0x7258, 0xA058 };
static const unsigned long ulPrinceHPB[] = { 0x14CB, 0x14CB };
static const unsigned long ulPrinceHPC[] = { 0x00 }; /*** Inaccessible. ***/
static const unsigned long ulShadowHPA[] = { 0x15F44, 0x18B44 };
static const unsigned long ulShadowHPB[] = { 0x00, 0x00 };
static const unsigned long ulShadowHPC[] = { 0x00 };
static const unsigned long ulChomperDelayA[] = { 0x19E9A, 0x1C29A };
static const unsigned long ulChomperDelayB[] = { 0x00, 0x00 };
static const unsigned long ulChomperDelayC[] = { 0x00 };
static const unsigned long ulMouseDelayA[] = { 0x79B8, 0xA7BB };
static const unsigned long ulMouseDelayB[] = { 0x00, 0x00 };
static const unsigned long ulMouseDelayC[] = { 0x00 };
/***
Defaults, as used in arDefaultGuard[][]:
4B 64 4B 4B 4B 32 64 DC 00 3C 28 3C strike prob.
00 00 00 05 05 AF 14 0A 00 FF FF 96 re-strike prob.
00 96 96 C8 C8 FF C8 FA 00 FF FF FF block prob.
00 4B 4B 64 64 91 64 FA 00 91 FF AF imp. block prob.
FF C8 C8 C8 FF FF C8 00 00 FF 64 64 advance prob.
14 14 14 14 0A 0A 0A 0A 00 0A 00 00 refractory timer
00 00 00 01 00 01 01 00 00 00 00 01 special color
00 00 00 00 01 00 00 00 00 00 00 00 extra strength
***/
static const unsigned long ulGuardA[][TABS_GUARD] =
{
	{
		0x15A1C + (0 * 0x0C),
		0x15A1C + (1 * 0x0C),
		0x15A1C + (2 * 0x0C),
		0x15A1C + (3 * 0x0C),
		0x15A1C + (4 * 0x0C),
		0x15A1C + (5 * 0x0C),
		0x15A1C + (6 * 0x0C),
		0x15A1C + (7 * 0x0C)
	},
	{
		0x1861C + (0 * 0x0C),
		0x1861C + (1 * 0x0C),
		0x1861C + (2 * 0x0C),
		0x1861C + (3 * 0x0C),
		0x1861C + (4 * 0x0C),
		0x1861C + (5 * 0x0C),
		0x1861C + (6 * 0x0C),
		0x1861C + (7 * 0x0C)
	}
};
static const unsigned long ulGuardB[][TABS_GUARD] =
{
	{
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00
	},
	{
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00
	}
};
static const unsigned long ulGuardC[][TABS_GUARD] =
{
	{
		0xEC42,
		0xEC4E,
		0xEC5A,
		0xEC66,
		0xEC72,
		0xEC7E,
		0xEC8A,
		0xEC96
	}
};
/*** 04 03 03 03 03 04 05 04 04 05 05 05 04 06 (01) ***/
static const unsigned long ulGuardHPA[] = { 0x15A7C, 0x1867C };
static const unsigned long ulGuardHPB[] = { 0x00, 0x00 };
static const unsigned long ulGuardHPC[] = { 0xECA2 };
/*** 01 00 00 00 01 01 01 00 00 00 01 01 00 00 (60) ***/
static const unsigned long ulGuardUniformA[] = { 0x15A8A, 0x1868A };
static const unsigned long ulGuardUniformB[] = { 0x00, 0x00 };
static const unsigned long ulGuardUniformC[] = { 0x00 };
/*** 00 00 00 01 02 02 03 02 02 02 02 02 04 05 (05) ***/
static const unsigned long ulGuardSpriteA[] = { 0x1A8D5, 0x1CCD5 };
static const unsigned long ulGuardSpriteB[] = { 0x114A, 0x114A };
static const unsigned long ulGuardSpriteC[] = { 0x00 };
/*** 00 00 00 00 01 01 01 02 02 02 01 01 02 02 01 ***/
static const unsigned long ulEnv1A[] = { 0x1A8B7, 0x1CCB7 };
static const unsigned long ulEnv1B[] = { 0x00, 0x00 };
static const unsigned long ulEnv1C[] = { 0x00 };
/*** 00 00 00 00 01 01 01 02 02 02 01 01 02 02 01 ***/
static const unsigned long ulEnv2A[] = { 0x1A8C6, 0x1CCC6 };
static const unsigned long ulEnv2B[] = { 0x00, 0x00 };
static const unsigned long ulEnv2C[] = { 0x00 };

int iEXEGuard[TABS_GUARD + 2][12 + 2];
int iEXEGuardHP[TABS_LEVEL + 2];
int iEXEGuardU[TABS_LEVEL + 2];
int iEXEGuardS[TABS_LEVEL + 2];
int iEXEEnv1[TABS_LEVEL + 2];
int iEXEEnv2[TABS_LEVEL + 2];
int iEXETab, iEXETabS;

/*** for text ***/
SDL_Color color_bl = {0x00, 0x00, 0x00, 255};
SDL_Color color_wh = {0xff, 0xff, 0xff, 255};
SDL_Color color_blue = {0x00, 0x00, 0xff, 255};
SDL_Color color_gray = {0xbf, 0xbf, 0xbf, 255};
SDL_Surface *message;
SDL_Texture *messaget;
SDL_Rect offset;

/*** for copying ***/
int arCopyPasteX[TILES + 2];
unsigned char arCopyPasteTile[TILES + 2];
unsigned char arCopyPasteMod[TILES + 2];
unsigned char cCopyPasteGuardTile;
unsigned char cCopyPasteGuardDir;
unsigned char cCopyPasteGuardSkill;
unsigned char cCopyPasteGuardC;

/*** controller ***/
int iController;
SDL_GameController *controller;
char sControllerName[MAX_CON + 2];
SDL_Joystick *joystick;
SDL_Haptic *haptic;
Uint32 joyleft, joyright, joyup, joydown;
Uint32 trigleft, trigright;
int iXJoy1, iYJoy1, iXJoy2, iYJoy2;

/*** These are the levels. ***/
int arRoomX[LEVELS + 2][ROOMS + 2][TILES + 2];
unsigned char arRoomTiles[LEVELS + 2][ROOMS + 2][TILES + 2];
unsigned char arRoomMod[LEVELS + 2][ROOMS + 2][TILES + 2];
unsigned char arEventsRoom[LEVELS + 2][EVENTS + 2];
unsigned char arEventsTile[LEVELS + 2][EVENTS + 2];
int arEventsNext[LEVELS + 2][EVENTS + 2];
unsigned char arEventsTimer[LEVELS + 2][EVENTS + 2];
unsigned char arRoomLinks[LEVELS + 2][ROOMS + 2][4 + 2];
unsigned char arBytes64[LEVELS + 2][64 + 2];
unsigned char arStartLocation[LEVELS + 2][3 + 2];
unsigned char arBytes4[LEVELS + 2][4 + 2];
unsigned char arGuardTile[LEVELS + 2][ROOMS + 2];
unsigned char arGuardDir[LEVELS + 2][ROOMS + 2];
unsigned char arGuardUnk1[LEVELS + 2][ROOMS + 2]; /*** GdStartX ***/
unsigned char arGuardUnk2[LEVELS + 2][ROOMS + 2]; /*** GdStartSeqL ***/
unsigned char arGuardSkill[LEVELS + 2][ROOMS + 2];
unsigned char arGuardUnk3[LEVELS + 2][ROOMS + 2]; /*** GdStartSeqH ***/
unsigned char arGuardC[LEVELS + 2][ROOMS + 2]; /*** ? ***/
unsigned char arBytes16[LEVELS + 2][16 + 2];

int iDX, iDY, iTTP1, iTTPO;
int iHor[10 + 2];
int iVer0, iVer1, iVer2, iVer3, iVer4;

SDL_Texture *imgloading;
SDL_Texture *imgd[0xFF + 2][0xFF + 2][2 + 2];
SDL_Texture *imgp[0xFF + 2][0xFF + 2][2 + 2];
SDL_Texture *imgblack;
SDL_Texture *imgprincel[2 + 2], *imgprincer[2 + 2];
SDL_Texture *imgguardl[2 + 2], *imgguardr[2 + 2];
SDL_Texture *imgskell[2 + 2], *imgskelr[2 + 2];
SDL_Texture *imgfatl[2 + 2], *imgfatr[2 + 2];
SDL_Texture *imgshadowl[2 + 2], *imgshadowr[2 + 2];
SDL_Texture *imgjaffarl[2 + 2], *imgjaffarr[2 + 2];
SDL_Texture *imgdisabled;
SDL_Texture *imgunk[2 + 2];
SDL_Texture *imgup_0;
SDL_Texture *imgup_1;
SDL_Texture *imgdown_0;
SDL_Texture *imgdown_1;
SDL_Texture *imgleft_0;
SDL_Texture *imgleft_1;
SDL_Texture *imgright_0;
SDL_Texture *imgright_1;
SDL_Texture *imgudno;
SDL_Texture *imglrno;
SDL_Texture *imgudnonfo;
SDL_Texture *imgprevon_0;
SDL_Texture *imgprevon_1;
SDL_Texture *imgnexton_0;
SDL_Texture *imgnexton_1;
SDL_Texture *imgprevoff;
SDL_Texture *imgnextoff;
SDL_Texture *imgbara, *imgbarb, *imgbarc;
SDL_Texture *imgextras[10 + 2];
SDL_Texture *imgroomson_0;
SDL_Texture *imgroomson_1;
SDL_Texture *imgroomsoff;
SDL_Texture *imgbroomson_0;
SDL_Texture *imgbroomson_1;
SDL_Texture *imgbroomsoff;
SDL_Texture *imgeventson_0;
SDL_Texture *imgeventson_1;
SDL_Texture *imgeventsoff;
SDL_Texture *imgsaveon_0;
SDL_Texture *imgsaveon_1;
SDL_Texture *imgsaveoff;
SDL_Texture *imgquit_0;
SDL_Texture *imgquit_1;
SDL_Texture *imgrl;
SDL_Texture *imgbrl;
SDL_Texture *imgsrc;
SDL_Texture *imgsrs;
SDL_Texture *imgsrm;
SDL_Texture *imgsrp;
SDL_Texture *imgsrb;
SDL_Texture *imgevents;
SDL_Texture *imgsele;
SDL_Texture *imgeventu;
SDL_Texture *imgsell;
SDL_Texture *imgdungeon;
SDL_Texture *imgpalace;
SDL_Texture *imgclosebig_0;
SDL_Texture *imgclosebig_1;
SDL_Texture *imgborderb;
SDL_Texture *imgborders;
SDL_Texture *imgbordersl;
SDL_Texture *imgborderbl;
SDL_Texture *imgfadedl;
SDL_Texture *imgpopup;
SDL_Texture *imgok[2 + 2];
SDL_Texture *imgsave[2 + 2];
SDL_Texture *imgpopup_yn;
SDL_Texture *imgyes[2 + 2];
SDL_Texture *imgno[2 + 2];
SDL_Texture *imghelp;
SDL_Texture *imgexe;
SDL_Texture *imgfadeds;
SDL_Texture *imgroom[24 + 2];
SDL_Texture *imgchover;
SDL_Texture *imgemulator;
SDL_Texture *imgspriteflamed;
SDL_Texture *imgspriteflamep;
SDL_Texture *imgexetab;
SDL_Texture *imgexetabs;
SDL_Texture *imgexeenvok;
SDL_Texture *imgexeenvwarn;
SDL_Texture *imgmouse;
SDL_Texture *imgtooltipg;
SDL_Texture *imgpreviewb;
SDL_Texture *imgeventh;
SDL_Texture *imghc;
SDL_Texture *imghcadis, *imghcaoff, *imghcaon, *imghcalb;
SDL_Texture *imghcbdis, *imghcboff, *imghcbon, *imghcblb;
SDL_Texture *imghccdis, *imghccoff, *imghccon, *imghcclb;

struct sample {
	Uint8 *data;
	Uint32 dpos;
	Uint32 dlen;
} sounds[NUM_SOUNDS];

void ShowUsage (void);
int GetPathFileA (void);
int GetPathFileB (void);
int GetPathFileC (void);
void LoadLevels (int iAtLevel);
void SaveLevels (void);
void PrintTileName (int iLevel, int iRoom, int iTile, int iTileValue);
void PrintMod (int iTileValue, int iModValue);
void PrIfDe (char *sString);
char cShowDirection (int iDirection);
void Quit (void);
void InitScreen (void);
void InitPopUpSave (void);
void ShowPopUpSave (void);
void LoadFonts (void);
void MixAudio (void *unused, Uint8 *stream, int iLen);
void PlaySound (char *sFile);
void PreLoadSet (char cTypeP, int iTile, int iMod);
void PreLoad (char *sPath, char *sPNG, SDL_Texture **imgImage);
void ShowScreen (void);
void InitPopUp (void);
void ShowPopUp (void);
void Help (void);
void ShowHelp (void);
void EXE (void);
void ShowEXE (void);
void InitScreenAction (char *sAction);
void RunLevel (int iLevel);
int StartGame (void *unused);
void ClearRoom (void);
void UseTile (int iTile, int iLocation, int iRoom);
void Zoom (int iToggleFull);
void LinkMinus (void);
int BrokenRoomLinks (int iPrint);
void ChangeEvent (int iAmount, int iChangePos);
void ChangeCustom (int iAmount, int iType);
void Prev (void);
void Next (void);
void CallSave (void);
void Sprinkle (void);
void SetLocation (int iRoom, int iLocation, int iTile, int iMod);
void FlipRoom (int iAxis);
void CopyPaste (int iAction);
int InArea (int iUpperLeftX, int iUpperLeftY,
	int iLowerRightX, int iLowerRightY);
int MouseSelectAdj (void);
int OnLevelBar (void);
void ChangePos (void);
void RemoveOldRoom (void);
void AddNewRoom (int iX, int iY, int iRoom);
void LinkPlus (void);
void ShowImage (SDL_Texture *img, int iX, int iY, char *sImageInfo);
void CustomRenderCopy (SDL_Texture* src, SDL_Rect* srcrect,
	SDL_Rect *dstrect, char *sImageInfo);
void CreateBAK (void);
void DisplayText (int iStartX, int iStartY, int iFontSize,
	char arText[9 + 2][MAX_TEXT + 2], int iLines, TTF_Font *font);
void InitRooms (void);
void WhereToStart (void);
void CheckSides (int iRoom, int iX, int iY);
void ShowRooms (int iRoom, int iX, int iY, int iNext);
void BrokenRoomChange (int iRoom, int iSide, int *iX, int *iY);
void ShowChange (void);
int OnTile (void);
void ChangePosAction (char *sAction);
void DisableSome (void);
int IsDisabled (int iTile);
void CenterNumber (int iNumber, int iX, int iY,
	SDL_Color fore, int iHex);
int Unused (int iTile);
void OpenURL (char *sURL);
void EXELoad (void);
void EXESave (void);
int PlusMinus (int *iWhat, int iX, int iY,
	int iMin, int iMax, int iChange, int iAddChanged);
void GetOptionValue (char *sArgv, char *sValue);
int BitsToInt (char *sString);
void IntToBits (int iInt, char *sOutput, int iBits);
void GetAsEightBits (unsigned char cChar, char *sBinary);
int ChecksumOrWrite (int iFd, int iLevel);
int Verify (int iFd, int iOffset, char *sText);
void GetTileMod (int iGetRoom, int iGetTile, int *iTile, int *iMod);
void GetTileModChange (int iGetTile, int *iTile, int *iMod);
void ApplySkillIfNecessary (int iTile);
void LoadingBar (int iBarHeight);
void HomeComputerAction (char *sAction);
void HomeComputer (void);
void ShowHomeComputer (void);
void PlaytestStart (int iLevel);
void PlaytestStop (void);

/*****************************************************************************/
int main (int argc, char *argv[])
/*****************************************************************************/
{
	int iArgLoop;
	SDL_version verc, verl;
	time_t tm;
	char sStartLevel[MAX_OPTION + 2];

	iDebug = 0;
	iExtras = 0;
	iLastX = 0;
	iLastTile = 0x00;
	iLastMod = 0x00;
	iInfo = 0;
	iScale = 1;
	iOnTile = 1;
	iOnTileOld = 1;
	iCopied = 0;
	iNoAudio = 0;
	iFullscreen = 0;
	iNoController = 0;
	iStartLevel = 1;
	iCustomMod = 0;
	iCustomTile = 0x00;
	iCustomMod = 0x00;
	iEmulator = 0;
	iNoAnim = 0;
	iMouse = 0;
	iGuardTooltip = 0;
	iEventHover = 0;
	iHomeComputer = 0;
	iModified = 0;

	if (argc > 1)
	{
		for (iArgLoop = 1; iArgLoop <= argc - 1; iArgLoop++)
		{
			if ((strcmp (argv[iArgLoop], "-h") == 0) ||
				(strcmp (argv[iArgLoop], "-?") == 0) ||
				(strcmp (argv[iArgLoop], "--help") == 0))
			{
				ShowUsage();
			}
			else if ((strcmp (argv[iArgLoop], "-v") == 0) ||
				(strcmp (argv[iArgLoop], "--version") == 0))
			{
				printf ("%s %s\n", EDITOR_NAME, EDITOR_VERSION);
				exit (EXIT_NORMAL);
			}
			else if ((strcmp (argv[iArgLoop], "-d") == 0) ||
				(strcmp (argv[iArgLoop], "--debug") == 0))
			{
				iDebug = 1;
			}
			else if ((strcmp (argv[iArgLoop], "-n") == 0) ||
				(strcmp (argv[iArgLoop], "--noaudio") == 0))
			{
				iNoAudio = 1;
			}
			else if ((strcmp (argv[iArgLoop], "-z") == 0) ||
				(strcmp (argv[iArgLoop], "--zoom") == 0))
			{
				iScale = 2;
			}
			else if ((strcmp (argv[iArgLoop], "-f") == 0) ||
				(strcmp (argv[iArgLoop], "--fullscreen") == 0))
			{
				iFullscreen = SDL_WINDOW_FULLSCREEN_DESKTOP;
			}
			else if ((strncmp (argv[iArgLoop], "-l=", 3) == 0) ||
				(strncmp (argv[iArgLoop], "--level=", 8) == 0))
			{
				GetOptionValue (argv[iArgLoop], sStartLevel);
				iStartLevel = atoi (sStartLevel);
				if ((iStartLevel < 1) || (iStartLevel > LEVELS))
				{
					iStartLevel = 1;
				}
			}
			else if ((strcmp (argv[iArgLoop], "-s") == 0) ||
				(strcmp (argv[iArgLoop], "--static") == 0))
			{
				iNoAnim = 1;
			}
			else if ((strcmp (argv[iArgLoop], "-k") == 0) ||
				(strcmp (argv[iArgLoop], "--keyboard") == 0))
			{
				iNoController = 1;
			}
			else if ((strcmp (argv[iArgLoop], "-a") == 0) ||
				(strcmp (argv[iArgLoop], "--appleii") == 0))
			{
				iHomeComputer = 1;
			}
			else if ((strcmp (argv[iArgLoop], "-b") == 0) ||
				(strcmp (argv[iArgLoop], "--bbcmaster") == 0))
			{
				iHomeComputer = 2;
			}
			else if ((strcmp (argv[iArgLoop], "-c") == 0) ||
				(strcmp (argv[iArgLoop], "--c64") == 0))
			{
				iHomeComputer = 3;
			}
			else
			{
				ShowUsage();
			}
		}
	}

	iAppleII = GetPathFileA();
	iBBCMaster = GetPathFileB();
	iC64 = GetPathFileC();

	srand ((unsigned)time(&tm));

	/*** Show the SDL version used for compiling and linking. ***/
	if (iDebug == 1)
	{
		SDL_VERSION (&verc);
		SDL_GetVersion (&verl);
		printf ("[ INFO ] Compiled with SDL %u.%u.%u, linked with SDL %u.%u.%u.\n",
			verc.major, verc.minor, verc.patch, verl.major, verl.minor, verl.patch);
	}

	InitScreen();
	Quit();

	return 0;
}
/*****************************************************************************/
void ShowUsage (void)
/*****************************************************************************/
{
	printf ("%s %s\n%s\n\n", EDITOR_NAME, EDITOR_VERSION, COPYRIGHT);
	printf ("Usage:\n");
	printf ("  %s [OPTIONS]\n\nOptions:\n", EDITOR_NAME);
	printf ("  -h, -?,    --help           display this help and exit\n");
	printf ("  -v,        --version        output version information and"
		" exit\n");
	printf ("  -d,        --debug          also show levels on the console\n");
	printf ("  -n,        --noaudio        do not play sound effects\n");
	printf ("  -z,        --zoom           double the interface size\n");
	printf ("  -f,        --fullscreen     start in fullscreen mode\n");
	printf ("  -l=NR,     --level=NR       start in level NR\n");
	printf ("  -s,        --static         do not display animations\n");
	printf ("  -k,        --keyboard       do not use a game controller\n");
	printf ("  -a,        --appleii        edit Apple II levels\n");
	printf ("  -b,        --bbcmaster      edit BBC Master levels\n");
	printf ("  -c,        --c64            edit C64 levels\n");
	printf ("\n");
	exit (EXIT_NORMAL);
}
/*****************************************************************************/
int GetPathFileA (void)
/*****************************************************************************/
{
	int iFound;
	DIR *dDir;
	struct dirent *stDirent;
	char sExtension[100 + 2];
	char sWarning[MAX_WARNING + 2];
	int iFd;
	int iVerify[10 + 2];

	iFound = 0;

	dDir = opendir (DISK_DIR_A);
	if (dDir == NULL)
	{
		printf ("[ WARN ] Cannot open directory \"%s\": %s!\n",
			DISK_DIR_A, strerror (errno));
		return (0);
	}

	while ((stDirent = readdir (dDir)) != NULL)
	{
		if (iFound == 0)
		{
			if ((strcmp (stDirent->d_name, ".") != 0) &&
				(strcmp (stDirent->d_name, "..") != 0))
			{
				snprintf (sExtension, 100, "%s", strrchr (stDirent->d_name, '.'));
				if ((toupper (sExtension[1]) == 'P') &&
					(toupper (sExtension[2]) == 'O'))
				{
					iFound = 1;
					snprintf (sPathFileA, MAX_PATHFILE, "%s%s%s", DISK_DIR_A, SLASH,
						stDirent->d_name);
					if (iDebug == 1)
					{
						printf ("[  OK  ] Found Apple II disk image \"%s\".\n",
							sPathFileA);
					}
				}
			}
		}
	}

	closedir (dDir);

	if (iFound == 0)
	{
		snprintf (sWarning, MAX_WARNING, "Cannot find a .po disk image in"
			" directory \"%s\"!", DISK_DIR_A);
		printf ("[ WARN ] %s\n", sWarning);
		SDL_ShowSimpleMessageBox (SDL_MESSAGEBOX_ERROR,
			"Warning", sWarning, NULL);
		return (0);
	}

	/*** Is the file accessible? ***/
	if (access (sPathFileA, R_OK|W_OK) == -1)
	{
		printf ("[ WARN ] Cannot access \"%s\": %s!\n",
			sPathFileA, strerror (errno));
		return (0);
	}

	/*** Which disk image: adamgreen (A0) or peterferrie (A1)? ***/
	iDiskImageA = -1;
	iFd = open (sPathFileA, O_RDONLY|O_BINARY);
	if (iFd == -1)
	{
		printf ("[ WARN ] Could not open \"%s\": %s!\n",
			sPathFileA, strerror (errno));
		return (0);
	}
	/*** A0 version (adamgreen) ***/
	if (iDiskImageA == -1)
	{
		iVerify[1] = Verify (iFd, A0_PRODOS_OFFSET_1, A0_PRODOS_TEXT);
		iVerify[2] = Verify (iFd, A0_PRODOS_OFFSET_2, A0_PRODOS_TEXT);
		iVerify[3] = Verify (iFd, A0_POP_OFFSET_1, A0_POP_TEXT);
		iVerify[4] = Verify (iFd, A0_POP_OFFSET_2, A0_POP_TEXT);
		if ((iVerify[1] == 1) && (iVerify[2] == 1) &&
			(iVerify[3] == 1) && (iVerify[4] == 1))
		{
			iDiskImageA = 0;
			PrIfDe ("[ INFO ] adamgreen (A0)\n");
		}
	}
	/*** A1 version (peterferrie) ***/
	if (iDiskImageA == -1)
	{
		iVerify[1] = Verify (iFd, A1_PRODOS_OFFSET_1, A1_PRODOS_TEXT);
		iVerify[2] = Verify (iFd, A1_PRODOS_OFFSET_2, A1_PRODOS_TEXT);
		iVerify[3] = Verify (iFd, A1_POP_OFFSET_1, A1_POP_TEXT);
		iVerify[4] = Verify (iFd, A1_POP_OFFSET_2, A1_POP_TEXT);
		if ((iVerify[1] == 1) && (iVerify[2] == 1) &&
			(iVerify[3] == 1) && (iVerify[4] == 1))
		{
			iDiskImageA = 1;
			PrIfDe ("[ INFO ] peterferrie (A1)\n");
		}
	}
	close (iFd);

	if (iDiskImageA == -1)
	{
		snprintf (sWarning, MAX_WARNING, "File %s is not a PoP1 for Apple II"
			" disk image!", sPathFileA);
		printf ("[ WARN ] %s\n", sWarning);
		SDL_ShowSimpleMessageBox (SDL_MESSAGEBOX_ERROR,
			"Warning", sWarning, NULL);
		return (0);
	} else {
		return (1);
	}
}
/*****************************************************************************/
int GetPathFileB (void)
/*****************************************************************************/
{
	int iFound;
	DIR *dDir;
	struct dirent *stDirent;
	char sExtension[100 + 2];
	char sWarning[MAX_WARNING + 2];
	int iFd;
	int iVerify[10 + 2];

	iFound = 0;

	dDir = opendir (DISK_DIR_B);
	if (dDir == NULL)
	{
		printf ("[ WARN ] Cannot open directory \"%s\": %s!\n",
			DISK_DIR_B, strerror (errno));
		return (0);
	}

	while ((stDirent = readdir (dDir)) != NULL)
	{
		if (iFound == 0)
		{
			if ((strcmp (stDirent->d_name, ".") != 0) &&
				(strcmp (stDirent->d_name, "..") != 0))
			{
				snprintf (sExtension, 100, "%s", strrchr (stDirent->d_name, '.'));
				if ((toupper (sExtension[1]) == 'S') &&
					(toupper (sExtension[2]) == 'S') &&
					(toupper (sExtension[3]) == 'D'))
				{
					iFound = 1;
					snprintf (sPathFileB, MAX_PATHFILE, "%s%s%s", DISK_DIR_B, SLASH,
						stDirent->d_name);
					if (iDebug == 1)
					{
						printf ("[  OK  ] Found BBC Master disk image \"%s\".\n",
							sPathFileB);
					}
				}
			}
		}
	}

	closedir (dDir);

	if (iFound == 0)
	{
		snprintf (sWarning, MAX_WARNING, "Cannot find a .ssd disk image in"
			" directory \"%s\"!", DISK_DIR_B);
		printf ("[ WARN ] %s\n", sWarning);
		SDL_ShowSimpleMessageBox (SDL_MESSAGEBOX_ERROR,
			"Warning", sWarning, NULL);
		return (0);
	}

	/*** Is the file accessible? ***/
	if (access (sPathFileB, R_OK|W_OK) == -1)
	{
		printf ("[ WARN ] Cannot access \"%s\": %s!\n",
			sPathFileB, strerror (errno));
		return (0);
	}

	/*** Which disk image: kieranhj 1.0 (B0) or kieranhj 1.1 (B1)? ***/
	iDiskImageB = -1;
	iFd = open (sPathFileB, O_RDONLY|O_BINARY);
	if (iFd == -1)
	{
		printf ("[ WARN ] Could not open \"%s\": %s!\n",
			sPathFileB, strerror (errno));
		return (0);
	}
	/*** B0 version (kieranhj 1.0) ***/
	if (iDiskImageB == -1)
	{
		iVerify[1] = Verify (iFd, B0_POPBBCM_OFFSET, B0_POPBBCM_TEXT);
		iVerify[2] = Verify (iFd, B0_VANDB_OFFSET, B0_VANDB_TEXT);
		if ((iVerify[1] == 1) && (iVerify[2] == 1))
		{
			iDiskImageB = 0;
			PrIfDe ("[ INFO ] kieranhj 1.0 (B0)\n");
		}
	}
	/*** B1 version (kieranhj 1.1) ***/
	if (iDiskImageB == -1)
	{
		iVerify[1] = Verify (iFd, B1_POPBBCM_OFFSET, B1_POPBBCM_TEXT);
		iVerify[2] = Verify (iFd, B1_VANDB_OFFSET, B1_VANDB_TEXT);
		if ((iVerify[1] == 1) && (iVerify[2] == 1))
		{
			iDiskImageB = 1;
			PrIfDe ("[ INFO ] kieranhj 1.1 (B1)\n");
		}
	}
	close (iFd);

	if (iDiskImageB == -1)
	{
		snprintf (sWarning, MAX_WARNING, "File %s is not a PoP1 for BBC Master"
			" disk image!", sPathFileB);
		printf ("[ WARN ] %s\n", sWarning);
		SDL_ShowSimpleMessageBox (SDL_MESSAGEBOX_ERROR,
			"Warning", sWarning, NULL);
		return (0);
	} else {
		return (1);
	}
}
/*****************************************************************************/
int GetPathFileC (void)
/*****************************************************************************/
{
	int iFound;
	DIR *dDir;
	struct dirent *stDirent;
	char sExtension[100 + 2];
	char sWarning[MAX_WARNING + 2];
	int iFd;
	int iVerify[10 + 2];

	iFound = 0;

	dDir = opendir (DISK_DIR_C);
	if (dDir == NULL)
	{
		printf ("[ WARN ] Cannot open directory \"%s\": %s!\n",
			DISK_DIR_C, strerror (errno));
		return (0);
	}

	while ((stDirent = readdir (dDir)) != NULL)
	{
		if (iFound == 0)
		{
			if ((strcmp (stDirent->d_name, ".") != 0) &&
				(strcmp (stDirent->d_name, "..") != 0))
			{
				snprintf (sExtension, 100, "%s", strrchr (stDirent->d_name, '.'));
				if ((toupper (sExtension[1]) == 'C') &&
					(toupper (sExtension[2]) == 'R') &&
					(toupper (sExtension[3]) == 'T'))
				{
					iFound = 1;
					snprintf (sPathFileC, MAX_PATHFILE, "%s%s%s", DISK_DIR_C, SLASH,
						stDirent->d_name);
					if (iDebug == 1)
					{
						printf ("[  OK  ] Found C64 disk image \"%s\".\n",
							sPathFileC);
					}
				}
			}
		}
	}

	closedir (dDir);

	if (iFound == 0)
	{
		snprintf (sWarning, MAX_WARNING, "Cannot find a .crt disk image in"
			" directory \"%s\"!", DISK_DIR_C);
		printf ("[ WARN ] %s\n", sWarning);
		SDL_ShowSimpleMessageBox (SDL_MESSAGEBOX_ERROR,
			"Warning", sWarning, NULL);
		return (0);
	}

	/*** Is the file accessible? ***/
	if (access (sPathFileC, R_OK|W_OK) == -1)
	{
		printf ("[ WARN ] Cannot access \"%s\": %s!\n",
			sPathFileC, strerror (errno));
		return (0);
	}

	/*** Which disk image: mrsid (C0)? ***/
	iDiskImageC = -1;
	iFd = open (sPathFileC, O_RDONLY|O_BINARY);
	if (iFd == -1)
	{
		printf ("[ WARN ] Could not open \"%s\": %s!\n",
			sPathFileC, strerror (errno));
		return (0);
	}
	/*** C0 version (mrsid) ***/
	if (iDiskImageC == -1)
	{
		iVerify[1] = Verify (iFd, C0_C64CART_OFFSET, C0_C64CART_TEXT);
		iVerify[2] = Verify (iFd, C0_DATE_OFFSET, C0_DATE_TEXT);
		if ((iVerify[1] == 1) && (iVerify[2] == 1))
		{
			iDiskImageC = 0;
			PrIfDe ("[ INFO ] mrsid (C0)\n");
		}
	}
	close (iFd);

	if (iDiskImageC == -1)
	{
		snprintf (sWarning, MAX_WARNING, "File %s is not a PoP1 for C64"
			" disk image!", sPathFileC);
		printf ("[ WARN ] %s\n", sWarning);
		SDL_ShowSimpleMessageBox (SDL_MESSAGEBOX_ERROR,
			"Warning", sWarning, NULL);
		return (0);
	} else {
		return (1);
	}
}
/*****************************************************************************/
void LoadLevels (int iAtLevel)
/*****************************************************************************/
{
	int iOffsetStart;
	int iOffsetEnd;
	int iLevel;
	int iTileValue;
	int iTileMod;
	int iTiles;
	int iTemp;
	int iRead;
	unsigned char sRead[2 + 2];
	int iEOF;
	char sBinaryFDoors[9 + 2]; /*** 8 chars, plus \0 ***/
	char sBinarySDoors[9 + 2]; /*** 8 chars, plus \0 ***/
	char sEventsRoom[10 + 2];
	char sEventsTile[10 + 2];
	char sEventsTimer[10 + 2];
	int iFd;

	/*** Used for looping. ***/
	int iRoomLoop;
	int iTileLoop;
	int iSideLoop;
	int iGuardLoop;
	int iEventLoop;
	int iLevelLoop;
	int iByteLoop;

	/*** Set cCurType and iCurGuard. ***/
	EXELoad();
	switch (iEXEEnv1[iAtLevel])
	{
		case 0x00: case 0x02: cCurType = 'd'; break;
		case 0x01: cCurType = 'p'; break;
	}
	iCurGuard = iEXEGuardS[iAtLevel];

	iFd = open (sPathFile, O_RDONLY|O_BINARY);
	if (iFd == -1)
	{
		printf ("[FAILED] Could not open \"%s\": %s!\n",
			sPathFile, strerror (errno));
		exit (EXIT_ERROR);
	}

	for (iLevelLoop = 1; iLevelLoop <= LEVELS; iLevelLoop++)
	{
		switch (iHomeComputer)
		{
			case 1:
				iOffsetStart = arLevelOffsetsA[iDiskImageA][iLevelLoop - 1];
				break;
			case 2:
				iOffsetStart = arLevelOffsetsB[iDiskImageB][iLevelLoop - 1];
				break;
			case 3:
				iOffsetStart = arLevelOffsetsC[iDiskImageC][iLevelLoop - 1];
				break;
			default: printf ("[FAILED] iHomeComputer!\n"); exit (EXIT_ERROR); break;
		}
		lseek (iFd, iOffsetStart, SEEK_SET);

		/*** We present level 0 to users as level 15. ***/
		switch (iLevelLoop)
		{
			case 1: iLevel = 15; break;
			default: iLevel = iLevelLoop - 1; break;
		}

		/*** Read the level into arLevel. ***/
		if (iDebug == 1)
		{
			printf ("[ INFO ] Level %i starts at offset 0x%02x (%i).\n",
				iLevel, iOffsetStart, iOffsetStart);
		}
		iLevelRead = 0;
		iEOF = 0;
		do {
			iRead = read (iFd, sRead, 1);
			switch (iRead)
			{
				case -1:
					printf ("[FAILED] Could not read from \"%s\": %s!\n",
						sPathFile, strerror (errno));
					exit (EXIT_ERROR);
					break;
				case 0: iEOF = 1; break;
				default:
					arLevel[iLevelRead] = sRead[0];
					iLevelRead++;
					break;
			}
		} while ((iEOF == 0) && (iLevelRead < LEVEL_SIZE));
		iOffsetEnd = iOffsetStart + LEVEL_SIZE - 1;

		if (iDebug == 1)
		{
			printf ("[ INFO ] Level %i ends at offset 0x%02x (%i).\n",
				iLevel, iOffsetEnd, iOffsetEnd);
			printf ("[ INFO ] Level size: %i\n",
				iOffsetEnd - iOffsetStart + 1);
			printf ("\n");
		}

		iOffsetStart = iOffsetEnd + 1;

		/*** Extract tiles and mods. ***/
		iTiles = -1;
		for (iRoomLoop = 1; iRoomLoop <= ROOMS; iRoomLoop++)
		{
			for (iTileLoop = 1; iTileLoop <= TILES; iTileLoop++)
			{
				iTiles++;
				iTileValue = arLevel[iTiles];
				iTileMod = arLevel[iTiles + (ROOMS * TILES)];
				if (iTileValue >= 32)
				{
					iTileValue-=32;
					arRoomX[iLevel][iRoomLoop][iTileLoop] = 1;
				} else {
					arRoomX[iLevel][iRoomLoop][iTileLoop] = 0;
				}
				arRoomTiles[iLevel][iRoomLoop][iTileLoop] = iTileValue;
				arRoomMod[iLevel][iRoomLoop][iTileLoop] = iTileMod;
			}

			/*** Debug. ***/
			if (iDebug == 1)
			{
				printf ("[Level %i] Room %i:\n\n", iLevel, iRoomLoop);

				for (iTileLoop = 1; iTileLoop <= 10; iTileLoop++)
				{
					PrintTileName (iLevel, iRoomLoop, iTileLoop,
						arRoomTiles[iLevel][iRoomLoop][iTileLoop]);
					if (iTileLoop == 10) { printf ("\n"); } else { printf ("|"); }
				}
				for (iTileLoop = 1; iTileLoop <= 10; iTileLoop++)
				{
					PrintMod (arRoomTiles[iLevel][iRoomLoop][iTileLoop],
						arRoomMod[iLevel][iRoomLoop][iTileLoop]);
					if (iTileLoop == 10) { printf ("\n"); } else { printf ("|"); }
				}
				for (iTemp = 1; iTemp <= 79; iTemp++) { printf ("-"); }
				printf ("\n");
				for (iTileLoop = 11; iTileLoop <= 20; iTileLoop++)
				{
					PrintTileName (iLevel, iRoomLoop, iTileLoop,
						arRoomTiles[iLevel][iRoomLoop][iTileLoop]);
					if (iTileLoop == 20) { printf ("\n"); } else { printf ("|"); }
				}
				for (iTileLoop = 11; iTileLoop <= 20; iTileLoop++)
				{
					PrintMod (arRoomTiles[iLevel][iRoomLoop][iTileLoop],
						arRoomMod[iLevel][iRoomLoop][iTileLoop]);
					if (iTileLoop == 20) { printf ("\n"); } else { printf ("|"); }
				}
				for (iTemp = 1; iTemp <= 79; iTemp++) { printf ("-"); }
				printf ("\n");
				for (iTileLoop = 21; iTileLoop <= 30; iTileLoop++)
				{
					PrintTileName (iLevel, iRoomLoop, iTileLoop,
						arRoomTiles[iLevel][iRoomLoop][iTileLoop]);
					if (iTileLoop == 30) { printf ("\n"); } else { printf ("|"); }
				}
				for (iTileLoop = 21; iTileLoop <= 30; iTileLoop++)
				{
					PrintMod (arRoomTiles[iLevel][iRoomLoop][iTileLoop],
						arRoomMod[iLevel][iRoomLoop][iTileLoop]);
					if (iTileLoop == 30) { printf ("\n"); } else { printf ("|"); }
				}
				printf ("\n");
			}
		}
		iTiles+=(ROOMS * TILES);

		/*** Events. ***/
		for (iEventLoop = 1; iEventLoop <= EVENTS; iEventLoop++)
		{
			iTiles++;
			GetAsEightBits (arLevel[iTiles], sBinaryFDoors);
			GetAsEightBits (arLevel[iTiles + EVENTS], sBinarySDoors);
			snprintf (sEventsRoom, 10, "%c%c%c%c%c",
				sBinarySDoors[0], sBinarySDoors[1], sBinarySDoors[2],
				sBinaryFDoors[1], sBinaryFDoors[2]);
			arEventsRoom[iLevel][iEventLoop] = BitsToInt (sEventsRoom);
			snprintf (sEventsTile, 10, "%c%c%c%c%c",
				sBinaryFDoors[3], sBinaryFDoors[4], sBinaryFDoors[5],
				sBinaryFDoors[6], sBinaryFDoors[7]);
			arEventsTile[iLevel][iEventLoop] = BitsToInt (sEventsTile) + 1;
			switch (sBinaryFDoors[0])
			{
				case '0': arEventsNext[iLevel][iEventLoop] = 1; break;
				case '1': arEventsNext[iLevel][iEventLoop] = 0; break;
			}
			snprintf (sEventsTimer, 10, "%c%c%c%c%c",
				sBinarySDoors[3], sBinarySDoors[4], sBinarySDoors[5],
				sBinarySDoors[6], sBinarySDoors[7]);
			arEventsTimer[iLevel][iEventLoop] = BitsToInt (sEventsTimer);

			if (iDebug == 1)
			{
				printf ("[ INFO ] Event %i triggers room %i, tile %i. (next: ",
					iEventLoop,
					arEventsRoom[iLevel][iEventLoop],
					arEventsTile[iLevel][iEventLoop]);
				switch (arEventsNext[iLevel][iEventLoop])
				{
					case 0: printf ("no"); break;
					case 1: printf ("yes"); break;
				}
				printf (", timer: %i)\n", arEventsTimer[iLevel][iEventLoop]);
			}
		}
		iTiles+=EVENTS;

		/*** Extract room links. ***/
		PrIfDe ("[  OK  ] Loading: Room Links\n");
		for (iRoomLoop = 1; iRoomLoop <= ROOMS; iRoomLoop++)
		{
			for (iSideLoop = 1; iSideLoop <= 4; iSideLoop++)
			{
				iTiles++;
				arRoomLinks[iLevel][iRoomLoop][iSideLoop] = arLevel[iTiles];
			}
			if (iDebug == 1)
			{
				printf ("[ INFO ] Room %i is connected to room (0 = none):"
					" l%i, r%i, u%i, d%i\n", iRoomLoop,
					arRoomLinks[iLevel][iRoomLoop][1],
					arRoomLinks[iLevel][iRoomLoop][2],
					arRoomLinks[iLevel][iRoomLoop][3],
					arRoomLinks[iLevel][iRoomLoop][4]);
			}
		}

		/*** Unknown (64). ***/
		for (iByteLoop = 0; iByteLoop < 64; iByteLoop++)
		{
			iTiles++;
			arBytes64[iLevel][iByteLoop] = arLevel[iTiles];
		}

		/* We want remapped modifiers in all 24 rooms.
		 * And [0] contains the first unused room.
		 */
		arBytes64[iLevel][0] = 0x19;

		/*** Extract start location. ***/
		iTiles++;
		arStartLocation[iLevel][1] = arLevel[iTiles]; /*** Room. ***/
		iTiles++;
		arStartLocation[iLevel][2] = arLevel[iTiles] + 1; /*** Tile. ***/
		iTiles++;
		arStartLocation[iLevel][3] = arLevel[iTiles]; /*** Direction. ***/
		/*** 1 of 2 ***/
		if ((iLevel == 1) || (iLevel == 13))
		{
			if (arStartLocation[iLevel][3] == 0x00)
				{ arStartLocation[iLevel][3] = 0xFF; }
					else { arStartLocation[iLevel][3] = 0x00; }
		}
		if (iDebug == 1)
		{
			printf ("[ INFO ] The prince starts in room: %i, tile %i, turned: %c\n",
				arStartLocation[iLevel][1], arStartLocation[iLevel][2],
				cShowDirection (arStartLocation[iLevel][3]));
		}

		/*** Unknown (4). ***/
		for (iByteLoop = 0; iByteLoop < 4; iByteLoop++)
		{
			iTiles++;
			arBytes4[iLevel][iByteLoop] = arLevel[iTiles];
		}

		/*** Extract guards. ***/
		for (iGuardLoop = 1; iGuardLoop <= ROOMS; iGuardLoop++)
		{
			iTiles++;
			arGuardTile[iLevel][iGuardLoop] = arLevel[iTiles] + 1;
			arGuardDir[iLevel][iGuardLoop] = arLevel[iTiles + (ROOMS * 1)];
			arGuardUnk1[iLevel][iGuardLoop] = arLevel[iTiles + (ROOMS * 2)];
			arGuardUnk2[iLevel][iGuardLoop] = arLevel[iTiles + (ROOMS * 3)];
			arGuardSkill[iLevel][iGuardLoop] = arLevel[iTiles + (ROOMS * 4)];
			arGuardUnk3[iLevel][iGuardLoop] = arLevel[iTiles + (ROOMS * 5)];
			arGuardC[iLevel][iGuardLoop] = arLevel[iTiles + (ROOMS * 6)];

			if (iDebug == 1)
			{
				if (arGuardTile[iLevel][iGuardLoop] <= TILES)
				{
					printf ("[ INFO ] (l%i) Guard; room:%i, tile:%i, dir:%c,"
						" skill:%i, c:%i (%i/%i/%i)\n",
						iLevel, iGuardLoop, arGuardTile[iLevel][iGuardLoop],
						cShowDirection (arGuardDir[iLevel][iGuardLoop]),
						arGuardSkill[iLevel][iGuardLoop],
						arGuardC[iLevel][iGuardLoop],
						arGuardUnk1[iLevel][iGuardLoop],
						arGuardUnk2[iLevel][iGuardLoop],
						arGuardUnk3[iLevel][iGuardLoop]);
				}
			}
		}
		iTiles+=(ROOMS * 6);

		/*** Unknown (16). ***/
		for (iByteLoop = 0; iByteLoop < 16; iByteLoop++)
		{
			iTiles++;
			arBytes16[iLevel][iByteLoop] = arLevel[iTiles];
		}

		/*** Checksum. ***/
		iTiles++;
		if (iDebug == 1)
		{
			printf ("[ INFO ] Loading checksum: 0x%02x (%i)\n",
				arLevel[iTiles], arLevel[iTiles]);
		}

		PrIfDe ("[  OK  ] Checking for broken room links.\n");
		arBrokenRoomLinks[iLevel] = BrokenRoomLinks (1);

		if (iDebug == 1)
		{
			printf ("[  OK  ] Done processing level %i.\n\n", iLevel);
		}
	}

	close (iFd);
}
/*****************************************************************************/
void SaveLevels (void)
/*****************************************************************************/
{
	int iOffsetStart;
	int iLevel;
	int iFd;
	char sToWrite[MAX_TOWRITE + 2];

	/*** Used for looping. ***/
	int iLevelLoop;
	int iByteLoop;

	iFd = open (sPathFile, O_WRONLY|O_BINARY);
	if (iFd == -1)
	{
		printf ("[FAILED] Could not open \"%s\": %s!\n",
			sPathFile, strerror (errno));
		exit (EXIT_ERROR);
	}

	for (iLevelLoop = 1; iLevelLoop <= LEVELS; iLevelLoop++)
	{
		switch (iHomeComputer)
		{
			case 1:
				iOffsetStart = arLevelOffsetsA[iDiskImageA][iLevelLoop - 1];
				break;
			case 2:
				iOffsetStart = arLevelOffsetsB[iDiskImageB][iLevelLoop - 1];
				break;
			case 3:
				iOffsetStart = arLevelOffsetsC[iDiskImageC][iLevelLoop - 1];
				break;
			default: printf ("[FAILED] iHomeComputer!\n"); exit (EXIT_ERROR); break;
		}
		lseek (iFd, iOffsetStart, SEEK_SET);

		/*** We present level 0 to users as level 15. ***/
		switch (iLevelLoop)
		{
			case 1: iLevel = 15; break;
			default: iLevel = iLevelLoop - 1; break;
		}

		cChecksum = ChecksumOrWrite (-1, iLevel);

		if ((iHomeComputer == 3) && (iLevel == 15))
		{
			/*** The C64 port has no demo level. ***/
		} else {
			ChecksumOrWrite (iFd, iLevel);
		}
	}

	/*** Workaround for copy-protection. ***/
	switch (iHomeComputer)
	{
		case 1: iOffsetStart = ulCopyProtA[iDiskImageA]; break;
		case 2: iOffsetStart = ulCopyProtB[iDiskImageB]; break;
		case 3: iOffsetStart = ulCopyProtC[iDiskImageC]; break;
		default: printf ("[FAILED] iHomeComputer!\n"); exit (EXIT_ERROR); break;
	}
	if (iOffsetStart != 0x00)
	{
		lseek (iFd, iOffsetStart, SEEK_SET);
		for (iByteLoop = 0; iByteLoop < 8; iByteLoop++)
		{
			snprintf (sToWrite, MAX_TOWRITE, "%c", 0xEA); /*** nop ***/
			write (iFd, sToWrite, 1);
		}
	}

	/*** There should be no saved game. ***/
	switch (iHomeComputer)
	{
		case 1: iOffsetStart = ulSavedLevelA[iDiskImageA]; break;
		case 2: iOffsetStart = ulSavedLevelB[iDiskImageB]; break;
		case 3: iOffsetStart = ulSavedLevelC[iDiskImageC]; break;
		default: printf ("[FAILED] iHomeComputer!\n"); exit (EXIT_ERROR); break;
	}
	if (iOffsetStart != 0x00)
	{
		lseek (iFd, iOffsetStart, SEEK_SET);
		snprintf (sToWrite, MAX_TOWRITE, "%c", 0xFF); /*** negative ***/
		write (iFd, sToWrite, 1);
	}

	close (iFd);

	PlaySound ("wav/save.wav");

	iChanged = 0;
}
/*****************************************************************************/
void PrintTileName (int iLevel, int iRoom, int iTile, int iTileValue)
/*****************************************************************************/
{
	switch (iTileValue)
	{
		case 0x00: printf ("empty  "); break;
		case 0x01: printf ("floor  "); break;
		case 0x02: printf ("spikes "); break;
		case 0x03: printf ("pil sma"); break;
		case 0x04: printf ("gate   "); break;
		case 0x05: printf ("stuck b"); break;
		case 0x06: printf ("drop   "); break;
		case 0x07: printf ("tap+flr"); break;
		case 0x08: printf ("pil bot"); break;
		case 0x09: printf ("pil top"); break;
		case 0x0A: printf ("potion "); break;
		case 0x0B: printf ("loose  "); break;
		case 0x0C: printf ("tapestr"); break;
		case 0x0D: printf ("mirror "); break;
		case 0x0E: printf ("rubble "); break;
		case 0x0F: printf ("raise  "); break;
		case 0x10: printf ("exit le"); break;
		case 0x11: printf ("exit ri"); break;
		case 0x12: printf ("chomper"); break;
		case 0x13: printf ("torch  "); break;
		case 0x14: printf ("wall   "); break;
		case 0x15: printf ("skeleto"); break;
		case 0x16: printf ("sword  "); break;
		case 0x17: printf ("balc le"); break;
		case 0x18: printf ("balc ri"); break;
		case 0x19: printf ("lat pil"); break;
		case 0x1A: printf ("lat top"); break;
		case 0x1B: printf ("lat arc"); break;
		case 0x1C: printf ("lat lef"); break;
		case 0x1D: printf ("lat rig"); break;
		case 0x1E: printf ("1E     "); break;
		default:
			printf ("\n[ WARN ] Unknown tile in level %i, room %i,"
				" tile %i: 0x%02x!\n", iLevel, iRoom, iTile, iTileValue);
			break;
	}
}
/*****************************************************************************/
void PrintMod (int iTileValue, int iModValue)
/*****************************************************************************/
{
	char sModValue[10 + 2];

	switch (iTileValue)
	{
		case 0x00: /*** empty ***/
			switch (iModValue)
			{
				case 0x00: printf ("      0"); break;
				case 0x01: printf ("shadow1"); break;
				case 0x02: printf ("shadow2"); break;
				case 0x03: printf ("window "); break;
				case 0xFF: printf ("    255"); break;
				default:
					snprintf (sModValue, 10, "WARN%3i", iModValue);
					printf ("%s", sModValue);
					break;
			}
			break;
		case 0x01: /*** floor ***/
			switch (iModValue)
			{
				case 0x00: printf ("      0"); break;
				case 0x01: printf ("      1"); break;
				case 0x02: printf ("      2"); break;
				case 0x03: printf ("      3"); break;
				case 0xFF: printf ("    255"); break;
				default:
					snprintf (sModValue, 10, "WARN%3i", iModValue);
					printf ("%s", sModValue);
					break;
			}
			break;
		case 0x02: /*** spikes ***/
			switch (iModValue)
			{
				case 0x00: printf ("regular"); break;
				case 0x01: printf ("part 1 "); break;
				case 0x02: printf ("part 2 "); break;
				case 0x03: printf ("part 3 "); break;
				case 0x04: printf ("part 4 "); break;
				case 0x05: printf ("part 5 "); break;
				case 0x06: printf ("part 6 "); break;
				case 0x07: printf ("part 7 "); break;
				case 0x08: printf ("part 8 "); break;
				case 0x09: printf ("reg stu"); break;
				default:
					snprintf (sModValue, 10, "WARN%3i", iModValue);
					printf ("%s", sModValue);
					break;
			}
			break;
		case 0x04: /*** gate ***/
			switch (iModValue)
			{
				case 0x01: printf ("open   "); break;
				case 0x02: printf ("closed "); break;
				default:
					snprintf (sModValue, 10, "WARN%3i", iModValue);
					printf ("%s", sModValue);
					break;
			}
			break;
		case 0x06: case 0x0F: /*** drop and raise ***/
			snprintf (sModValue, 10, "E:%5i", iModValue + 1);
			printf ("%s", sModValue);
			break;
		case 0x07: /*** tap flr ***/
			switch (iModValue)
			{
				case 0x00: printf ("      0"); break;
				case 0x01: printf ("      1"); break;
				case 0x02: printf ("      2"); break;
				case 0x03: printf ("      3"); break;
				default:
					snprintf (sModValue, 10, "WARN%3i", iModValue);
					printf ("%s", sModValue);
					break;
			}
			break;
		case 0x0A: /*** potion ***/
			switch (iModValue)
			{
				case 0x00: printf ("empty s"); break;
				case 0x01: printf ("heal   "); break;
				case 0x02: printf ("life   "); break;
				case 0x03: printf ("float  "); break;
				case 0x04: printf ("flip   "); break;
				case 0x05: printf ("hurt   "); break;
				case 0x06: printf ("empty b"); break;
				default:
					snprintf (sModValue, 10, "WARN%3i", iModValue);
					printf ("%s", sModValue);
					break;
			}
			break;
		case 0x0C: /*** tap ***/
			switch (iModValue)
			{
				case 0x00: printf ("blue   "); break;
				case 0x01: printf ("red    "); break;
				case 0x02: printf ("square "); break;
				case 0x03: printf ("      3"); break;
				case 0x04: printf ("      4"); break;
				case 0x05: printf ("      5"); break;
				case 0x06: printf ("      6"); break;
				case 0x07: printf ("      7"); break;
				default:
					snprintf (sModValue, 10, "WARN%3i", iModValue);
					printf ("%s", sModValue);
					break;
			}
			break;
		case 0x12: /*** chomper ***/
			switch (iModValue)
			{
				case 0x00: printf ("regular"); break;
				case 0x01: printf ("      1"); break;
				case 0x02: printf ("closed "); break;
				case 0x03: printf ("      3"); break;
				case 0x04: printf ("      4"); break;
				case 0x05: printf ("      5"); break;
				case 0x80: printf ("reg bld"); break;
				case 0x81: printf ("blood81"); break;
				case 0x82: printf ("cls bld"); break;
				case 0x83: printf ("blood83"); break;
				case 0x84: printf ("blood84"); break;
				case 0x85: printf ("blood85"); break;
				default:
					snprintf (sModValue, 10, "WARN%3i", iModValue);
					printf ("%s", sModValue);
					break;
			}
			break;
		case 0x14: /*** wall ***/
			switch (iModValue)
			{
				case 0x00: printf ("      0"); break;
				case 0x01: printf ("      1"); break;
				default:
					snprintf (sModValue, 10, "WARN%3i", iModValue);
					printf ("%s", sModValue);
					break;
			}
			break;
		default:
			if (iModValue != 0)
			{
				snprintf (sModValue, 10, "WARN%3i", iModValue);
				printf ("%s", sModValue);
			} else {
				printf ("      0");
			}
			break;
	}
}
/*****************************************************************************/
void PrIfDe (char *sString)
/*****************************************************************************/
{
	if (iDebug == 1) { printf ("%s", sString); }
}
/*****************************************************************************/
char cShowDirection (int iDirection)
/*****************************************************************************/
{
	switch (iDirection)
	{
		case 0x00: return ('r'); break;
		case 0xFF: return ('l'); break;
	}
	return ('?');
}
/*****************************************************************************/
void Quit (void)
/*****************************************************************************/
{
	if (iChanged != 0) { InitPopUpSave(); }
	if (iModified == 1) { PlaytestStop(); }
	TTF_CloseFont (font1);
	TTF_CloseFont (font2);
	TTF_CloseFont (font3);
	TTF_Quit();
	SDL_Quit();
	exit (EXIT_NORMAL);
}
/*****************************************************************************/
void InitScreen (void)
/*****************************************************************************/
{
	SDL_AudioSpec fmt;
	char sImage[MAX_IMG + 2];
	SDL_Surface *imgicon;
	int iJoyNr;
	SDL_Event event;
	int iOldXPos, iOldYPos;
	const Uint8 *keystate;
	SDL_Rect barbox;

	/*** Used for looping. ***/
	int iRoomLoop;
	int iRoomLoop2;
	int iTileLoop;
	int iColLoop, iRowLoop;

	if (SDL_Init (SDL_INIT_AUDIO|SDL_INIT_VIDEO|
		SDL_INIT_GAMECONTROLLER|SDL_INIT_HAPTIC) < 0)
	{
		printf ("[FAILED] Unable to init SDL: %s!\n", SDL_GetError());
		exit (EXIT_ERROR);
	}
	atexit (SDL_Quit);

	window = SDL_CreateWindow (EDITOR_NAME " " EDITOR_VERSION,
		SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
		(WINDOW_WIDTH) * iScale, (WINDOW_HEIGHT) * iScale, iFullscreen);
	if (window == NULL)
	{
		printf ("[FAILED] Unable to create a window: %s!\n", SDL_GetError());
		exit (EXIT_ERROR);
	}
	ascreen = SDL_CreateRenderer (window, -1, 0);
	if (ascreen == NULL)
	{
		printf ("[FAILED] Unable to set video mode: %s!\n", SDL_GetError());
		exit (EXIT_ERROR);
	}
	/*** Some people may prefer linear, but we're going old school. ***/
	SDL_SetHint (SDL_HINT_RENDER_SCALE_QUALITY, "nearest");
	if (iFullscreen != 0)
	{
		SDL_RenderSetLogicalSize (ascreen, (WINDOW_WIDTH) * iScale,
			(WINDOW_HEIGHT) * iScale);
	}

	if (TTF_Init() == -1)
	{
		printf ("[FAILED] Could not initialize TTF!\n");
		exit (EXIT_ERROR);
	}

	LoadFonts();

	curArrow = SDL_CreateSystemCursor (SDL_SYSTEM_CURSOR_ARROW);
	curWait = SDL_CreateSystemCursor (SDL_SYSTEM_CURSOR_WAIT);
	curHand = SDL_CreateSystemCursor (SDL_SYSTEM_CURSOR_HAND);

	if (iNoAudio != 1)
	{
		PrIfDe ("[  OK  ] Initializing Audio\n");
		fmt.freq = 44100;
		fmt.format = AUDIO_S16;
		fmt.channels = 2;
		fmt.samples = 512;
		fmt.callback = MixAudio;
		fmt.userdata = NULL;
		if (SDL_OpenAudio (&fmt, NULL) < 0)
		{
			printf ("[FAILED] Unable to open audio: %s!\n", SDL_GetError());
			exit (EXIT_ERROR);
		}
		SDL_PauseAudio (0);
	}

	/*** icon ***/
	snprintf (sImage, MAX_IMG, "png%svarious%sleapop_icon.png",
		SLASH, SLASH);
	imgicon = IMG_Load (sImage);
	if (imgicon == NULL)
	{
		printf ("[ WARN ] Could not load \"%s\": %s!\n", sImage, strerror (errno));
	} else {
		SDL_SetWindowIcon (window, imgicon);
	}

	/*** Open the first available controller. ***/
	iController = 0;
	if (iNoController != 1)
	{
		for (iJoyNr = 0; iJoyNr < SDL_NumJoysticks(); iJoyNr++)
		{
			if (SDL_IsGameController (iJoyNr))
			{
				controller = SDL_GameControllerOpen (iJoyNr);
				if (controller)
				{
					snprintf (sControllerName, MAX_CON, "%s",
						SDL_GameControllerName (controller));
					if (iDebug == 1)
					{
						printf ("[ INFO ] Found a controller \"%s\"; \"%s\".\n",
							sControllerName, SDL_GameControllerNameForIndex (iJoyNr));
					}
					joystick = SDL_GameControllerGetJoystick (controller);
					iController = 1;

					/*** Just for fun, use haptic. ***/
					if (SDL_JoystickIsHaptic (joystick))
					{
						haptic = SDL_HapticOpenFromJoystick (joystick);
						if (SDL_HapticRumbleInit (haptic) == 0)
						{
							SDL_HapticRumblePlay (haptic, 1.0, 1000);
						} else {
							printf ("[ WARN ] Could not initialize the haptic device: %s!\n",
								SDL_GetError());
						}
					} else {
						PrIfDe ("[ INFO ] The game controller is not haptic.\n");
					}
				} else {
					printf ("[ WARN ] Could not open game controller %i: %s!\n",
						iController, SDL_GetError());
				}
			}
		}
		if (iController != 1) { PrIfDe ("[ INFO ] No controller found.\n"); }
	} else {
		PrIfDe ("[ INFO ] Using keyboard and mouse.\n");
	}

	/*** Home computer select ***/
	if (iHomeComputer == 0)
	{
		PreLoad (PNG_VARIOUS, "home_computer.png", &imghc);
		if (iController != 1)
		{
			PreLoad (PNG_VARIOUS, "Apple_II_disabled.png", &imghcadis);
			PreLoad (PNG_VARIOUS, "Apple_II_off.png", &imghcaoff);
			PreLoad (PNG_VARIOUS, "Apple_II_on.png", &imghcaon);
			PreLoad (PNG_VARIOUS, "BBC_Master_disabled.png", &imghcbdis);
			PreLoad (PNG_VARIOUS, "BBC_Master_off.png", &imghcboff);
			PreLoad (PNG_VARIOUS, "BBC_Master_on.png", &imghcbon);
			PreLoad (PNG_VARIOUS, "C64_disabled.png", &imghccdis);
			PreLoad (PNG_VARIOUS, "C64_off.png", &imghccoff);
			PreLoad (PNG_VARIOUS, "C64_on.png", &imghccon);
		} else {
			PreLoad (PNG_GAMEPAD, "Apple_II_disabled.png", &imghcadis);
			PreLoad (PNG_GAMEPAD, "Apple_II_off.png", &imghcaoff);
			PreLoad (PNG_GAMEPAD, "Apple_II_on.png", &imghcaon);
			PreLoad (PNG_GAMEPAD, "BBC_Master_disabled.png", &imghcbdis);
			PreLoad (PNG_GAMEPAD, "BBC_Master_off.png", &imghcboff);
			PreLoad (PNG_GAMEPAD, "BBC_Master_on.png", &imghcbon);
			PreLoad (PNG_GAMEPAD, "C64_disabled.png", &imghccdis);
			PreLoad (PNG_GAMEPAD, "C64_off.png", &imghccoff);
			PreLoad (PNG_GAMEPAD, "C64_on.png", &imghccon);
		}
		HomeComputer();
		PlaySound ("wav/ok_close.wav");
	}

	switch (iHomeComputer)
	{
		case 1: snprintf (sPathFile, MAX_PATHFILE, "%s", sPathFileA); break;
		case 2: snprintf (sPathFile, MAX_PATHFILE, "%s", sPathFileB); break;
		case 3: snprintf (sPathFile, MAX_PATHFILE, "%s", sPathFileC); break;
	}

	LoadLevels (iStartLevel);

	/*******************/
	/* Preload images. */
	/*******************/

	/*** Loading... ***/
	PreLoad (PNG_VARIOUS, "loading.png", &imgloading);
	ShowImage (imgloading, 0, 0, "imgloading");
	SDL_SetRenderDrawColor (ascreen, 0x22, 0x22, 0x22, SDL_ALPHA_OPAQUE);
	barbox.x = 10 * iScale;
	barbox.y = 10 * iScale;
	barbox.w = 20 * iScale;
	barbox.h = 441 * iScale;
	SDL_RenderFillRect (ascreen, &barbox);
	SDL_RenderPresent (ascreen);

	iPreLoaded = 0;
	iCurrentBarHeight = 0;
	iNrToPreLoad = 454; /*** Value can be obtained via debug mode. ***/
	SDL_SetCursor (curWait);

	/*** Dungeon and palace tiles. ***/
	PreLoadSet ('d', 0x00, 0x00); PreLoadSet ('p', 0x00, 0x00);
	PreLoadSet ('d', 0x00, 0x01); PreLoadSet ('p', 0x00, 0x01);
	PreLoadSet ('d', 0x00, 0x02); PreLoadSet ('p', 0x00, 0x02);
	PreLoadSet ('d', 0x00, 0x03); PreLoadSet ('p', 0x00, 0x03);
	PreLoadSet ('d', 0x00, 0xFF); PreLoadSet ('p', 0x00, 0xFF);
	PreLoadSet ('d', 0x01, 0x00); PreLoadSet ('p', 0x01, 0x00);
	PreLoadSet ('d', 0x01, 0x01); PreLoadSet ('p', 0x01, 0x01);
	PreLoadSet ('d', 0x01, 0x02); PreLoadSet ('p', 0x01, 0x02);
	PreLoadSet ('d', 0x01, 0x03); PreLoadSet ('p', 0x01, 0x03);
	PreLoadSet ('d', 0x01, 0xFF); PreLoadSet ('p', 0x01, 0xFF);
	PreLoadSet ('d', 0x02, 0x00); PreLoadSet ('p', 0x02, 0x00);
	PreLoadSet ('d', 0x02, 0x01); PreLoadSet ('p', 0x02, 0x01);
	PreLoadSet ('d', 0x02, 0x02); PreLoadSet ('p', 0x02, 0x02);
	PreLoadSet ('d', 0x02, 0x03); PreLoadSet ('p', 0x02, 0x03);
	PreLoadSet ('d', 0x02, 0x04); PreLoadSet ('p', 0x02, 0x04);
	PreLoadSet ('d', 0x02, 0x05); PreLoadSet ('p', 0x02, 0x05);
	PreLoadSet ('d', 0x02, 0x06); PreLoadSet ('p', 0x02, 0x06);
	PreLoadSet ('d', 0x02, 0x07); PreLoadSet ('p', 0x02, 0x07);
	PreLoadSet ('d', 0x02, 0x08); PreLoadSet ('p', 0x02, 0x08);
	PreLoadSet ('d', 0x02, 0x09); PreLoadSet ('p', 0x02, 0x09);
	PreLoadSet ('d', 0x03, 0x00); PreLoadSet ('p', 0x03, 0x00);
	PreLoadSet ('d', 0x04, 0x01); PreLoadSet ('p', 0x04, 0x01);
	PreLoadSet ('d', 0x04, 0x02); PreLoadSet ('p', 0x04, 0x02);
	PreLoadSet ('d', 0x05, 0x00); PreLoadSet ('p', 0x05, 0x00);
	PreLoadSet ('d', 0x06, 0x00); PreLoadSet ('p', 0x06, 0x00);
	PreLoadSet ('d', 0x07, 0x00); PreLoadSet ('p', 0x07, 0x00);
	PreLoadSet ('d', 0x07, 0x01); PreLoadSet ('p', 0x07, 0x01);
	PreLoadSet ('d', 0x07, 0x02); PreLoadSet ('p', 0x07, 0x02);
	PreLoadSet ('d', 0x07, 0x03); PreLoadSet ('p', 0x07, 0x03);
	PreLoadSet ('d', 0x08, 0x00); PreLoadSet ('p', 0x08, 0x00);
	PreLoadSet ('d', 0x09, 0x00); PreLoadSet ('p', 0x09, 0x00);
	PreLoadSet ('d', 0x0A, 0x00); PreLoadSet ('p', 0x0A, 0x00);
	PreLoadSet ('d', 0x0A, 0x01); PreLoadSet ('p', 0x0A, 0x01);
	PreLoadSet ('d', 0x0A, 0x02); PreLoadSet ('p', 0x0A, 0x02);
	PreLoadSet ('d', 0x0A, 0x03); PreLoadSet ('p', 0x0A, 0x03);
	PreLoadSet ('d', 0x0A, 0x04); PreLoadSet ('p', 0x0A, 0x04);
	PreLoadSet ('d', 0x0A, 0x05); PreLoadSet ('p', 0x0A, 0x05);
	PreLoadSet ('d', 0x0A, 0x06); PreLoadSet ('p', 0x0A, 0x06);
	PreLoadSet ('d', 0x0B, 0x00); PreLoadSet ('p', 0x0B, 0x00);
	PreLoadSet ('d', 0x0C, 0x00); PreLoadSet ('p', 0x0C, 0x00);
	PreLoadSet ('d', 0x0C, 0x01); PreLoadSet ('p', 0x0C, 0x01);
	PreLoadSet ('d', 0x0C, 0x02); PreLoadSet ('p', 0x0C, 0x02);
	PreLoadSet ('d', 0x0C, 0x03); PreLoadSet ('p', 0x0C, 0x03);
	PreLoadSet ('d', 0x0C, 0x04); PreLoadSet ('p', 0x0C, 0x04);
	PreLoadSet ('d', 0x0C, 0x05); PreLoadSet ('p', 0x0C, 0x05);
	PreLoadSet ('d', 0x0C, 0x06); PreLoadSet ('p', 0x0C, 0x06);
	PreLoadSet ('d', 0x0C, 0x07); PreLoadSet ('p', 0x0C, 0x07);
	PreLoadSet ('d', 0x0D, 0x00); PreLoadSet ('p', 0x0D, 0x00);
	PreLoadSet ('d', 0x0E, 0x00); PreLoadSet ('p', 0x0E, 0x00);
	PreLoadSet ('d', 0x0F, 0x00); PreLoadSet ('p', 0x0F, 0x00);
	PreLoadSet ('d', 0x10, 0x00); PreLoadSet ('p', 0x10, 0x00);
	PreLoadSet ('d', 0x11, 0x00); PreLoadSet ('p', 0x11, 0x00);
	PreLoadSet ('d', 0x12, 0x00); PreLoadSet ('p', 0x12, 0x00);
	PreLoadSet ('d', 0x12, 0x01); PreLoadSet ('p', 0x12, 0x01);
	PreLoadSet ('d', 0x12, 0x02); PreLoadSet ('p', 0x12, 0x02);
	PreLoadSet ('d', 0x12, 0x03); PreLoadSet ('p', 0x12, 0x03);
	PreLoadSet ('d', 0x12, 0x04); PreLoadSet ('p', 0x12, 0x04);
	PreLoadSet ('d', 0x12, 0x05); PreLoadSet ('p', 0x12, 0x05);
	PreLoadSet ('d', 0x12, 0x80); PreLoadSet ('p', 0x12, 0x80);
	PreLoadSet ('d', 0x12, 0x81); PreLoadSet ('p', 0x12, 0x81);
	PreLoadSet ('d', 0x12, 0x82); PreLoadSet ('p', 0x12, 0x82);
	PreLoadSet ('d', 0x12, 0x83); PreLoadSet ('p', 0x12, 0x83);
	PreLoadSet ('d', 0x12, 0x84); PreLoadSet ('p', 0x12, 0x84);
	PreLoadSet ('d', 0x12, 0x85); PreLoadSet ('p', 0x12, 0x85);
	PreLoadSet ('d', 0x13, 0x00); PreLoadSet ('p', 0x13, 0x00);
	PreLoadSet ('d', 0x14, 0x00); PreLoadSet ('p', 0x14, 0x00);
	PreLoadSet ('d', 0x14, 0x01); PreLoadSet ('p', 0x14, 0x01);
	PreLoadSet ('d', 0x15, 0x00); PreLoadSet ('p', 0x15, 0x00);
	PreLoadSet ('d', 0x16, 0x00); PreLoadSet ('p', 0x16, 0x00);
	PreLoadSet ('d', 0x17, 0x00); PreLoadSet ('p', 0x17, 0x00);
	PreLoadSet ('d', 0x18, 0x00); PreLoadSet ('p', 0x18, 0x00);
	PreLoadSet ('d', 0x19, 0x00); PreLoadSet ('p', 0x19, 0x00);
	PreLoadSet ('d', 0x1A, 0x00); PreLoadSet ('p', 0x1A, 0x00);
	PreLoadSet ('d', 0x1B, 0x00); PreLoadSet ('p', 0x1B, 0x00);
	PreLoadSet ('d', 0x1C, 0x00); PreLoadSet ('p', 0x1C, 0x00);
	PreLoadSet ('d', 0x1D, 0x00); PreLoadSet ('p', 0x1D, 0x00);
	PreLoadSet ('d', 0x1E, 0x00); PreLoadSet ('p', 0x1E, 0x00);
	PreLoadSet ('d', 0x2B, 0x00); PreLoadSet ('p', 0x2B, 0x00);
	PreLoad ("dungeon", "0x13_sprite.png", &imgspriteflamed);
	PreLoad ("palace", "0x13_sprite.png", &imgspriteflamep);

	/*** various ***/
	PreLoad (PNG_VARIOUS, "black.png", &imgblack);
	PreLoad (PNG_VARIOUS, "disabled.png", &imgdisabled);
	PreLoad (PNG_VARIOUS, "unknown.png", &imgunk[1]);
	PreLoad (PNG_VARIOUS, "sel_unknown.png", &imgunk[2]);
	PreLoad (PNG_VARIOUS, "sel_room_current.png", &imgsrc);
	PreLoad (PNG_VARIOUS, "sel_room_start.png", &imgsrs);
	PreLoad (PNG_VARIOUS, "sel_room_moving.png", &imgsrm);
	PreLoad (PNG_VARIOUS, "sel_room_cross.png", &imgsrp);
	PreLoad (PNG_VARIOUS, "sel_room_broken.png", &imgsrb);
	PreLoad (PNG_VARIOUS, "sel_event.png", &imgsele);
	PreLoad (PNG_VARIOUS, "event_unused.png", &imgeventu);
	PreLoad (PNG_VARIOUS, "sel_level.png", &imgsell);
	PreLoad (PNG_VARIOUS, "border_small_live.png", &imgbordersl);
	PreLoad (PNG_VARIOUS, "border_big_live.png", &imgborderbl);
	PreLoad (PNG_VARIOUS, "faded_l.png", &imgfadedl);
	PreLoad (PNG_VARIOUS, "popup_yn.png", &imgpopup_yn);
	PreLoad (PNG_VARIOUS, "help.png", &imghelp);
	PreLoad (PNG_VARIOUS, "exe.png", &imgexe);
	PreLoad (PNG_VARIOUS, "faded_s.png", &imgfadeds);
	PreLoad (PNG_VARIOUS, "custom_hover.png", &imgchover);
	switch (iHomeComputer)
	{
		case 1:
			switch (iDiskImageA)
			{
				case 0: /*** adamgreen (A0) ***/
					PreLoad (PNG_VARIOUS, "AppleWin_A0.png", &imgemulator);
					break;
				case 1: /*** peterferrie (A1) ***/
					PreLoad (PNG_VARIOUS, "AppleWin_A1.png", &imgemulator);
					break;
			}
			PreLoad (PNG_VARIOUS, "Apple_II_lb.png", &imghcalb);
			break;
		case 2:
			switch (iDiskImageB)
			{
				case 0: /*** kieranhj 1.0 (B0) ***/
					PreLoad (PNG_VARIOUS, "B-em_B0.png", &imgemulator);
					break;
				case 1: /*** kieranhj 1.1 (B1) ***/
					PreLoad (PNG_VARIOUS, "B-em_B1.png", &imgemulator);
					break;
			}
			PreLoad (PNG_VARIOUS, "BBC_Master_lb.png", &imghcblb);
			break;
		case 3:
			switch (iDiskImageC)
			{
				case 0: /*** mrsid (C0) ***/
					PreLoad (PNG_VARIOUS, "VICE_C0.png", &imgemulator);
					break;
			}
			PreLoad (PNG_VARIOUS, "C64_lb.png", &imghcclb);
			break;
	}
	PreLoad (PNG_VARIOUS, "exe_tab.png", &imgexetab);
	PreLoad (PNG_VARIOUS, "exe_tab_small.png", &imgexetabs);
	PreLoad (PNG_VARIOUS, "exe_env_ok.png", &imgexeenvok);
	PreLoad (PNG_VARIOUS, "exe_env_warn.png", &imgexeenvwarn);
	PreLoad (PNG_VARIOUS, "mouse.png", &imgmouse);
	PreLoad (PNG_VARIOUS, "tooltip_guard.png", &imgtooltipg);
	PreLoad (PNG_VARIOUS, "preview_back.png", &imgpreviewb);
	PreLoad (PNG_VARIOUS, "event_hover.png", &imgeventh);
	if (iController != 1)
	{
		PreLoad (PNG_VARIOUS, "border_big.png", &imgborderb);
		PreLoad (PNG_VARIOUS, "border_small.png", &imgborders);
		PreLoad (PNG_VARIOUS, "broken_room_links.png", &imgbrl);
		PreLoad (PNG_VARIOUS, "dungeon.png", &imgdungeon);
		PreLoad (PNG_VARIOUS, "events.png", &imgevents);
		PreLoad (PNG_VARIOUS, "level_bar_a.png", &imgbara);
		PreLoad (PNG_VARIOUS, "level_bar_b.png", &imgbarb);
		PreLoad (PNG_VARIOUS, "level_bar_c.png", &imgbarc);
		PreLoad (PNG_VARIOUS, "palace.png", &imgpalace);
		PreLoad (PNG_VARIOUS, "popup.png", &imgpopup);
		PreLoad (PNG_VARIOUS, "room_links.png", &imgrl);
	} else {
		PreLoad (PNG_GAMEPAD, "border_big.png", &imgborderb);
		PreLoad (PNG_GAMEPAD, "border_small.png", &imgborders);
		PreLoad (PNG_GAMEPAD, "broken_room_links.png", &imgbrl);
		PreLoad (PNG_GAMEPAD, "dungeon.png", &imgdungeon);
		PreLoad (PNG_GAMEPAD, "events.png", &imgevents);
		PreLoad (PNG_GAMEPAD, "level_bar_a.png", &imgbara);
		PreLoad (PNG_GAMEPAD, "level_bar_b.png", &imgbarb);
		PreLoad (PNG_GAMEPAD, "level_bar_c.png", &imgbarc);
		PreLoad (PNG_GAMEPAD, "palace.png", &imgpalace);
		PreLoad (PNG_GAMEPAD, "popup.png", &imgpopup);
		PreLoad (PNG_GAMEPAD, "room_links.png", &imgrl);
	}

	/*** (s)living ***/
	PreLoad (PNG_LIVING, "prince_l.png", &imgprincel[1]);
	PreLoad (PNG_SLIVING, "prince_l.png", &imgprincel[2]);
	PreLoad (PNG_LIVING, "prince_r.png", &imgprincer[1]);
	PreLoad (PNG_SLIVING, "prince_r.png", &imgprincer[2]);
	PreLoad (PNG_LIVING, "guard_l.png", &imgguardl[1]);
	PreLoad (PNG_SLIVING, "guard_l.png", &imgguardl[2]);
	PreLoad (PNG_LIVING, "guard_r.png", &imgguardr[1]);
	PreLoad (PNG_SLIVING, "guard_r.png", &imgguardr[2]);
	PreLoad (PNG_LIVING, "skel_l.png", &imgskell[1]);
	PreLoad (PNG_SLIVING, "skel_l.png", &imgskell[2]);
	PreLoad (PNG_LIVING, "skel_r.png", &imgskelr[1]);
	PreLoad (PNG_SLIVING, "skel_r.png", &imgskelr[2]);
	PreLoad (PNG_LIVING, "fat_l.png", &imgfatl[1]);
	PreLoad (PNG_SLIVING, "fat_l.png", &imgfatl[2]);
	PreLoad (PNG_LIVING, "fat_r.png", &imgfatr[1]);
	PreLoad (PNG_SLIVING, "fat_r.png", &imgfatr[2]);
	PreLoad (PNG_LIVING, "shadow_l.png", &imgshadowl[1]);
	PreLoad (PNG_SLIVING, "shadow_l.png", &imgshadowl[2]);
	PreLoad (PNG_LIVING, "shadow_r.png", &imgshadowr[1]);
	PreLoad (PNG_SLIVING, "shadow_r.png", &imgshadowr[2]);
	PreLoad (PNG_LIVING, "jaffar_l.png", &imgjaffarl[1]);
	PreLoad (PNG_SLIVING, "jaffar_l.png", &imgjaffarl[2]);
	PreLoad (PNG_LIVING, "jaffar_r.png", &imgjaffarr[1]);
	PreLoad (PNG_SLIVING, "jaffar_r.png", &imgjaffarr[2]);

	/*** buttons ***/
	PreLoad (PNG_BUTTONS, "up_0.png", &imgup_0);
	PreLoad (PNG_BUTTONS, "up_1.png", &imgup_1);
	PreLoad (PNG_BUTTONS, "down_0.png", &imgdown_0);
	PreLoad (PNG_BUTTONS, "down_1.png", &imgdown_1);
	PreLoad (PNG_BUTTONS, "left_0.png", &imgleft_0);
	PreLoad (PNG_BUTTONS, "left_1.png", &imgleft_1);
	PreLoad (PNG_BUTTONS, "right_0.png", &imgright_0);
	PreLoad (PNG_BUTTONS, "right_1.png", &imgright_1);
	PreLoad (PNG_BUTTONS, "up_down_no.png", &imgudno);
	PreLoad (PNG_BUTTONS, "left_right_no.png", &imglrno);
	if (iController != 1)
	{
		PreLoad (PNG_BUTTONS, "broken_rooms_off.png", &imgbroomsoff);
		PreLoad (PNG_BUTTONS, "broken_rooms_on_0.png", &imgbroomson_0);
		PreLoad (PNG_BUTTONS, "broken_rooms_on_1.png", &imgbroomson_1);
		PreLoad (PNG_BUTTONS, "close_big_0.png", &imgclosebig_0);
		PreLoad (PNG_BUTTONS, "close_big_1.png", &imgclosebig_1);
		PreLoad (PNG_BUTTONS, "events_off.png", &imgeventsoff);
		PreLoad (PNG_BUTTONS, "events_on_0.png", &imgeventson_0);
		PreLoad (PNG_BUTTONS, "events_on_1.png", &imgeventson_1);
		PreLoad (PNG_BUTTONS, "next_off.png", &imgnextoff);
		PreLoad (PNG_BUTTONS, "next_on_0.png", &imgnexton_0);
		PreLoad (PNG_BUTTONS, "next_on_1.png", &imgnexton_1);
		PreLoad (PNG_BUTTONS, "No.png", &imgno[1]);
		PreLoad (PNG_BUTTONS, "OK.png", &imgok[1]);
		PreLoad (PNG_BUTTONS, "previous_off.png", &imgprevoff);
		PreLoad (PNG_BUTTONS, "previous_on_0.png", &imgprevon_0);
		PreLoad (PNG_BUTTONS, "previous_on_1.png", &imgprevon_1);
		PreLoad (PNG_BUTTONS, "quit_0.png", &imgquit_0);
		PreLoad (PNG_BUTTONS, "quit_1.png", &imgquit_1);
		PreLoad (PNG_BUTTONS, "rooms_off.png", &imgroomsoff);
		PreLoad (PNG_BUTTONS, "rooms_on_0.png", &imgroomson_0);
		PreLoad (PNG_BUTTONS, "rooms_on_1.png", &imgroomson_1);
		PreLoad (PNG_BUTTONS, "save_off.png", &imgsaveoff);
		PreLoad (PNG_BUTTONS, "save_on_0.png", &imgsaveon_0);
		PreLoad (PNG_BUTTONS, "save_on_1.png", &imgsaveon_1);
		PreLoad (PNG_BUTTONS, "Save.png", &imgsave[1]);
		PreLoad (PNG_BUTTONS, "sel_No.png", &imgno[2]);
		PreLoad (PNG_BUTTONS, "sel_OK.png", &imgok[2]);
		PreLoad (PNG_BUTTONS, "sel_Save.png", &imgsave[2]);
		PreLoad (PNG_BUTTONS, "sel_Yes.png", &imgyes[2]);
		PreLoad (PNG_BUTTONS, "up_down_no_nfo.png", &imgudnonfo);
		PreLoad (PNG_BUTTONS, "Yes.png", &imgyes[1]);
	} else {
		PreLoad (PNG_GAMEPAD, "broken_rooms_off.png", &imgbroomsoff);
		PreLoad (PNG_GAMEPAD, "broken_rooms_on_0.png", &imgbroomson_0);
		PreLoad (PNG_GAMEPAD, "broken_rooms_on_1.png", &imgbroomson_1);
		PreLoad (PNG_GAMEPAD, "close_big_0.png", &imgclosebig_0);
		PreLoad (PNG_GAMEPAD, "close_big_1.png", &imgclosebig_1);
		PreLoad (PNG_GAMEPAD, "events_off.png", &imgeventsoff);
		PreLoad (PNG_GAMEPAD, "events_on_0.png", &imgeventson_0);
		PreLoad (PNG_GAMEPAD, "events_on_1.png", &imgeventson_1);
		PreLoad (PNG_GAMEPAD, "next_off.png", &imgnextoff);
		PreLoad (PNG_GAMEPAD, "next_on_0.png", &imgnexton_0);
		PreLoad (PNG_GAMEPAD, "next_on_1.png", &imgnexton_1);
		PreLoad (PNG_GAMEPAD, "No.png", &imgno[1]);
		PreLoad (PNG_GAMEPAD, "OK.png", &imgok[1]);
		PreLoad (PNG_GAMEPAD, "previous_off.png", &imgprevoff);
		PreLoad (PNG_GAMEPAD, "previous_on_0.png", &imgprevon_0);
		PreLoad (PNG_GAMEPAD, "previous_on_1.png", &imgprevon_1);
		PreLoad (PNG_GAMEPAD, "quit_0.png", &imgquit_0);
		PreLoad (PNG_GAMEPAD, "quit_1.png", &imgquit_1);
		PreLoad (PNG_GAMEPAD, "rooms_off.png", &imgroomsoff);
		PreLoad (PNG_GAMEPAD, "rooms_on_0.png", &imgroomson_0);
		PreLoad (PNG_GAMEPAD, "rooms_on_1.png", &imgroomson_1);
		PreLoad (PNG_GAMEPAD, "save_off.png", &imgsaveoff);
		PreLoad (PNG_GAMEPAD, "save_on_0.png", &imgsaveon_0);
		PreLoad (PNG_GAMEPAD, "save_on_1.png", &imgsaveon_1);
		PreLoad (PNG_GAMEPAD, "Save.png", &imgsave[1]);
		PreLoad (PNG_GAMEPAD, "sel_No.png", &imgno[2]);
		PreLoad (PNG_GAMEPAD, "sel_OK.png", &imgok[2]);
		PreLoad (PNG_GAMEPAD, "sel_Save.png", &imgsave[2]);
		PreLoad (PNG_GAMEPAD, "sel_Yes.png", &imgyes[2]);
		PreLoad (PNG_GAMEPAD, "up_down_no_nfo.png", &imgudnonfo);
		PreLoad (PNG_GAMEPAD, "Yes.png", &imgyes[1]);
	}

	/*** extras ***/
	PreLoad (PNG_EXTRAS, "extras_00.png", &imgextras[0]);
	PreLoad (PNG_EXTRAS, "extras_01.png", &imgextras[1]);
	PreLoad (PNG_EXTRAS, "extras_02.png", &imgextras[2]);
	PreLoad (PNG_EXTRAS, "extras_03.png", &imgextras[3]);
	PreLoad (PNG_EXTRAS, "extras_04.png", &imgextras[4]);
	PreLoad (PNG_EXTRAS, "extras_05.png", &imgextras[5]);
	PreLoad (PNG_EXTRAS, "extras_06.png", &imgextras[6]);
	PreLoad (PNG_EXTRAS, "extras_07.png", &imgextras[7]);
	PreLoad (PNG_EXTRAS, "extras_08.png", &imgextras[8]);
	PreLoad (PNG_EXTRAS, "extras_09.png", &imgextras[9]);
	PreLoad (PNG_EXTRAS, "extras_10.png", &imgextras[10]);

	/*** rooms ***/
	PreLoad (PNG_ROOMS, "room1.png", &imgroom[1]);
	PreLoad (PNG_ROOMS, "room2.png", &imgroom[2]);
	PreLoad (PNG_ROOMS, "room3.png", &imgroom[3]);
	PreLoad (PNG_ROOMS, "room4.png", &imgroom[4]);
	PreLoad (PNG_ROOMS, "room5.png", &imgroom[5]);
	PreLoad (PNG_ROOMS, "room6.png", &imgroom[6]);
	PreLoad (PNG_ROOMS, "room7.png", &imgroom[7]);
	PreLoad (PNG_ROOMS, "room8.png", &imgroom[8]);
	PreLoad (PNG_ROOMS, "room9.png", &imgroom[9]);
	PreLoad (PNG_ROOMS, "room10.png", &imgroom[10]);
	PreLoad (PNG_ROOMS, "room11.png", &imgroom[11]);
	PreLoad (PNG_ROOMS, "room12.png", &imgroom[12]);
	PreLoad (PNG_ROOMS, "room13.png", &imgroom[13]);
	PreLoad (PNG_ROOMS, "room14.png", &imgroom[14]);
	PreLoad (PNG_ROOMS, "room15.png", &imgroom[15]);
	PreLoad (PNG_ROOMS, "room16.png", &imgroom[16]);
	PreLoad (PNG_ROOMS, "room17.png", &imgroom[17]);
	PreLoad (PNG_ROOMS, "room18.png", &imgroom[18]);
	PreLoad (PNG_ROOMS, "room19.png", &imgroom[19]);
	PreLoad (PNG_ROOMS, "room20.png", &imgroom[20]);
	PreLoad (PNG_ROOMS, "room21.png", &imgroom[21]);
	PreLoad (PNG_ROOMS, "room22.png", &imgroom[22]);
	PreLoad (PNG_ROOMS, "room23.png", &imgroom[23]);
	PreLoad (PNG_ROOMS, "room24.png", &imgroom[24]);

	if (iDebug == 1)
		{ printf ("[ INFO ] Preloaded images: %i\n", iPreLoaded); }
	SDL_SetCursor (curArrow);

	/*** Defaults. ***/
	iCurLevel = iStartLevel;
	iCurRoom = arStartLocation[iCurLevel][1];
	iDownAt = 0;
	iSelected = 1; /*** Start with the upper left selected. ***/
	iScreen = 1;
	iChangeEvent = 1;
	iFlameFrame = 1;
	oldticks = 0;
	iEXETab = 1;

	iTTP1 = TTPD_1;
	iTTPO = TTPD_O;
	iDX = DD_X;
	iDY = DD_Y;
	iHor[0] = (iDX * -1) + OFFSETD_X;
	iHor[1] = (iDX * 0) + OFFSETD_X;
	iHor[2] = (iDX * 1) + OFFSETD_X;
	iHor[3] = (iDX * 2) + OFFSETD_X;
	iHor[4] = (iDX * 3) + OFFSETD_X;
	iHor[5] = (iDX * 4) + OFFSETD_X;
	iHor[6] = (iDX * 5) + OFFSETD_X;
	iHor[7] = (iDX * 6) + OFFSETD_X;
	iHor[8] = (iDX * 7) + OFFSETD_X;
	iHor[9] = (iDX * 8) + OFFSETD_X;
	iHor[10] = (iDX * 9) + OFFSETD_X;
	iVer0 = OFFSETD_Y - iTTP1 - (iDY * 1);
	iVer1 = OFFSETD_Y - iTTP1 + (iDY * 0);
	iVer2 = OFFSETD_Y - iTTP1 + (iDY * 1);
	iVer3 = OFFSETD_Y - iTTP1 + (iDY * 2);
	iVer4 = OFFSETD_Y - iTTP1 + (iDY * 3);

	ShowScreen();
	InitPopUp();
	while (1)
	{
		if (iNoAnim == 0)
		{
			/*** This is for the game animation. ***/
			newticks = SDL_GetTicks();
			if (newticks > oldticks + REFRESH_GAME)
			{
				ShowScreen();
				oldticks = newticks;
			}
		}

		while (SDL_PollEvent (&event))
		{
			switch (event.type)
			{
				case SDL_CONTROLLERBUTTONDOWN:
					/*** Nothing for now. ***/
					break;
				case SDL_CONTROLLERBUTTONUP:
					switch (event.cbutton.button)
					{
						case SDL_CONTROLLER_BUTTON_A:
							InitScreenAction ("enter");
							break;
						case SDL_CONTROLLER_BUTTON_B:
							switch (iScreen)
							{
								case 1:
									Quit(); break;
								case 2:
									arBrokenRoomLinks[iCurLevel] = BrokenRoomLinks (0);
									iScreen = 1; break;
								case 3:
									iScreen = 1; break;
							}
							break;
						case SDL_CONTROLLER_BUTTON_X:
							if (iScreen != 2)
							{
								iScreen = 2;
								iMovingRoom = 0;
								iMovingNewBusy = 0;
								iChangingBrokenRoom = iCurRoom;
								iChangingBrokenSide = 1;
								PlaySound ("wav/screen2or3.wav");
							} else if (arBrokenRoomLinks[iCurLevel] == 0) {
								arBrokenRoomLinks[iCurLevel] = 1;
								PlaySound ("wav/screen2or3.wav");
							}
							break;
						case SDL_CONTROLLER_BUTTON_Y:
							if (iScreen == 2)
							{
								arBrokenRoomLinks[iCurLevel] = BrokenRoomLinks (0);
							}
							if (iScreen != 3)
							{
								iScreen = 3;
								PlaySound ("wav/screen2or3.wav");
							}
							break;
						case SDL_CONTROLLER_BUTTON_BACK:
							if ((iScreen == 2) && (arBrokenRoomLinks[iCurLevel] == 1))
							{
								LinkMinus();
							}
							if (iScreen == 3)
							{
								if (arEventsNext[iCurLevel][iChangeEvent] != 1)
								{
									arEventsNext[iCurLevel][iChangeEvent] = 1;
								} else {
									arEventsNext[iCurLevel][iChangeEvent] = 0;
								}
								PlaySound ("wav/check_box.wav");
								iChanged++;
							}
							break;
						case SDL_CONTROLLER_BUTTON_GUIDE:
							if (iChanged != 0) { CallSave(); } break;
						case SDL_CONTROLLER_BUTTON_START:
							RunLevel (iCurLevel);
							break;
						case SDL_CONTROLLER_BUTTON_LEFTSHOULDER:
							if (iCurLevel != 1)
							{
								if (iChanged != 0) { InitPopUpSave(); }
								Prev();
							}
							break;
						case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER:
							if (iCurLevel != LEVELS)
							{
								if (iChanged != 0) { InitPopUpSave(); }
								Next();
							}
							break;
						case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
							InitScreenAction ("left"); break;
						case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
							InitScreenAction ("right"); break;
						case SDL_CONTROLLER_BUTTON_DPAD_UP:
							InitScreenAction ("up"); break;
						case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
							InitScreenAction ("down"); break;
					}
					ShowScreen();
					break;
				case SDL_CONTROLLERAXISMOTION: /*** triggers and analog sticks ***/
					iXJoy1 = SDL_JoystickGetAxis (joystick, 0);
					iYJoy1 = SDL_JoystickGetAxis (joystick, 1);
					iXJoy2 = SDL_JoystickGetAxis (joystick, 3);
					iYJoy2 = SDL_JoystickGetAxis (joystick, 4);
					if ((iXJoy1 < -30000) || (iXJoy2 < -30000)) /*** left ***/
					{
						if ((SDL_GetTicks() - joyleft) > 300)
						{
							if (iScreen == 1)
							{
								if (arRoomLinks[iCurLevel][iCurRoom][1] != 0)
								{
									iCurRoom = arRoomLinks[iCurLevel][iCurRoom][1];
									PlaySound ("wav/scroll.wav");
								}
							}
							if (iScreen == 3)
							{
								ChangeEvent (-1, 0);
							}
							joyleft = SDL_GetTicks();
						}
					}
					if ((iXJoy1 > 30000) || (iXJoy2 > 30000)) /*** right ***/
					{
						if ((SDL_GetTicks() - joyright) > 300)
						{
							if (iScreen == 1)
							{
								if (arRoomLinks[iCurLevel][iCurRoom][2] != 0)
								{
									iCurRoom = arRoomLinks[iCurLevel][iCurRoom][2];
									PlaySound ("wav/scroll.wav");
								}
							}
							if (iScreen == 3)
							{
								ChangeEvent (1, 0);
							}
							joyright = SDL_GetTicks();
						}
					}
					if ((iYJoy1 < -30000) || (iYJoy2 < -30000)) /*** up ***/
					{
						if ((SDL_GetTicks() - joyup) > 300)
						{
							if (iScreen == 1)
							{
								if (arRoomLinks[iCurLevel][iCurRoom][3] != 0)
								{
									iCurRoom = arRoomLinks[iCurLevel][iCurRoom][3];
									PlaySound ("wav/scroll.wav");
								}
							}
							if (iScreen == 3)
							{
								ChangeEvent (10, 0);
							}
							joyup = SDL_GetTicks();
						}
					}
					if ((iYJoy1 > 30000) || (iYJoy2 > 30000)) /*** down ***/
					{
						if ((SDL_GetTicks() - joydown) > 300)
						{
							if (iScreen == 1)
							{
								if (arRoomLinks[iCurLevel][iCurRoom][4] != 0)
								{
									iCurRoom = arRoomLinks[iCurLevel][iCurRoom][4];
									PlaySound ("wav/scroll.wav");
								}
							}
							if (iScreen == 3)
							{
								ChangeEvent (-10, 0);
							}
							joydown = SDL_GetTicks();
						}
					}
					if (event.caxis.axis == SDL_CONTROLLER_AXIS_TRIGGERLEFT)
					{
						if ((SDL_GetTicks() - trigleft) > 300)
						{
							if (iScreen == 2)
							{
								if (arBrokenRoomLinks[iCurLevel] == 0)
								{
									iMovingNewBusy = 0;
									switch (iMovingRoom)
									{
										case 0: iMovingRoom = ROOMS; break; /*** If disabled. ***/
										case 1: iMovingRoom = ROOMS; break;
										default: iMovingRoom--; break;
									}
								}
							}
							if (iScreen == 3)
							{
								InitScreenAction ("left bracket");
							}
							trigleft = SDL_GetTicks();
						}
					}
					if (event.caxis.axis == SDL_CONTROLLER_AXIS_TRIGGERRIGHT)
					{
						if ((SDL_GetTicks() - trigright) > 300)
						{
							if (iScreen == 2)
							{
								if (arBrokenRoomLinks[iCurLevel] == 0)
								{
									iMovingNewBusy = 0;
									switch (iMovingRoom)
									{
										case 0: iMovingRoom = 1; break; /*** If disabled. ***/
										case 24: iMovingRoom = 1; break;
										default: iMovingRoom++; break;
									}
								}
							}
							if (iScreen == 3)
							{
								InitScreenAction ("right bracket");
							}
							trigright = SDL_GetTicks();
						}
					}
					ShowScreen();
					break;
				case SDL_KEYDOWN: /*** https://wiki.libsdl.org/SDL2/SDL_Keycode ***/
					switch (event.key.keysym.sym)
					{
						case SDLK_F1:
							if (iScreen == 1)
							{
								Help(); SDL_SetCursor (curArrow);
							}
							break;
						case SDLK_F2:
							if (iScreen == 1)
							{
								EXE();
								SDL_SetCursor (curArrow);
							}
							break;
						case SDLK_LEFTBRACKET:
							InitScreenAction ("left bracket"); /*** [ ***/
							break;
						case SDLK_RIGHTBRACKET:
							InitScreenAction ("right bracket"); /*** ] ***/
							break;
						case SDLK_d:
							RunLevel (iCurLevel);
							break;
						case SDLK_SLASH:
							if (iScreen == 1) { ClearRoom(); }
							break;
						case SDLK_BACKSLASH:
							if (iScreen == 1)
							{
								/*** Randomize the entire level. ***/
								for (iRoomLoop = 1; iRoomLoop <= ROOMS; iRoomLoop++)
								{
									for (iTileLoop = 1; iTileLoop <= TILES; iTileLoop++)
									{
										UseTile (-1, iTileLoop, iRoomLoop);
									}
								}
								iChanged++;
								PlaySound ("wav/ok_close.wav");
							}
							break;
						case SDLK_KP_ENTER:
						case SDLK_RETURN:
						case SDLK_SPACE:
							if (((event.key.keysym.mod & KMOD_LALT) ||
								(event.key.keysym.mod & KMOD_RALT)) && (iScreen == 1))
							{
								Zoom (1);
								iExtras = 0;
								PlaySound ("wav/extras.wav");
							} else {
								InitScreenAction ("enter");
							}
							break;
						case SDLK_BACKSPACE:
							if ((iScreen == 2) && (arBrokenRoomLinks[iCurLevel] == 1))
							{
								LinkMinus();
							}
							break;
						case SDLK_ESCAPE:
						case SDLK_q:
							switch (iScreen)
							{
								case 1:
									Quit(); break;
								case 2:
									arBrokenRoomLinks[iCurLevel] = BrokenRoomLinks (0);
									iScreen = 1; break;
								case 3:
									iScreen = 1; break;
							}
							break;
						case SDLK_LEFT:
							if ((event.key.keysym.mod & KMOD_LSHIFT) ||
								(event.key.keysym.mod & KMOD_RSHIFT))
							{
								switch (iScreen)
								{
									case 1:
										if (arRoomLinks[iCurLevel][iCurRoom][1] != 0)
										{
											iCurRoom = arRoomLinks[iCurLevel][iCurRoom][1];
											PlaySound ("wav/scroll.wav");
										}
										break;
									case 3:
										ChangeEvent (-1, 0);
										break;
								}
							} else if ((event.key.keysym.mod & KMOD_LCTRL) ||
								(event.key.keysym.mod & KMOD_RCTRL))
							{
								if (iScreen == 3)
								{
									ChangeEvent (-10, 0);
								}
							} else {
								InitScreenAction ("left");
							}
							break;
						case SDLK_RIGHT:
							if ((event.key.keysym.mod & KMOD_LSHIFT) ||
								(event.key.keysym.mod & KMOD_RSHIFT))
							{
								switch (iScreen)
								{
									case 1:
										if (arRoomLinks[iCurLevel][iCurRoom][2] != 0)
										{
											iCurRoom = arRoomLinks[iCurLevel][iCurRoom][2];
											PlaySound ("wav/scroll.wav");
										}
										break;
									case 3:
										ChangeEvent (1, 0);
										break;
								}
							} else if ((event.key.keysym.mod & KMOD_LCTRL) ||
								(event.key.keysym.mod & KMOD_RCTRL))
							{
								if (iScreen == 3)
								{
									ChangeEvent (10, 0);
								}
							} else {
								InitScreenAction ("right");
							}
							break;
						case SDLK_UP:
							if ((event.key.keysym.mod & KMOD_LSHIFT) ||
								(event.key.keysym.mod & KMOD_RSHIFT))
							{
								if (iScreen == 1)
								{
									if (arRoomLinks[iCurLevel][iCurRoom][3] != 0)
									{
										iCurRoom = arRoomLinks[iCurLevel][iCurRoom][3];
										PlaySound ("wav/scroll.wav");
									}
								}
							} else {
								InitScreenAction ("up");
							}
							break;
						case SDLK_DOWN:
							if ((event.key.keysym.mod & KMOD_LSHIFT) ||
								(event.key.keysym.mod & KMOD_RSHIFT))
							{
								if (iScreen == 1)
								{
									if (arRoomLinks[iCurLevel][iCurRoom][4] != 0)
									{
										iCurRoom = arRoomLinks[iCurLevel][iCurRoom][4];
										PlaySound ("wav/scroll.wav");
									}
								}
							} else {
								InitScreenAction ("down");
							}
							break;
						case SDLK_t:
							if (iScreen == 1) { InitScreenAction ("env"); }
							break;
						case SDLK_MINUS:
						case SDLK_KP_MINUS:
							if (iCurLevel != 1)
							{
								if (iChanged != 0) { InitPopUpSave(); }
								Prev();
							}
							break;
						case SDLK_KP_PLUS:
						case SDLK_EQUALS:
							if (iCurLevel != LEVELS)
							{
								if (iChanged != 0) { InitPopUpSave(); }
								Next();
							}
							break;
						case SDLK_r:
							if (iScreen != 2)
							{
								iScreen = 2;
								iMovingRoom = 0;
								iMovingNewBusy = 0;
								iChangingBrokenRoom = iCurRoom;
								iChangingBrokenSide = 1;
								PlaySound ("wav/screen2or3.wav");
							} else if (arBrokenRoomLinks[iCurLevel] == 0) {
								arBrokenRoomLinks[iCurLevel] = 1;
								PlaySound ("wav/screen2or3.wav");
							}
							break;
						case SDLK_e:
							if (iScreen == 2)
							{
								arBrokenRoomLinks[iCurLevel] = BrokenRoomLinks (0);
							}
							if (iScreen != 3)
							{
								iScreen = 3;
								PlaySound ("wav/screen2or3.wav");
							}
							break;
						case SDLK_s:
							if (iChanged != 0) { CallSave(); } break;
						case SDLK_z:
							if (iScreen == 1)
							{
								Zoom (0);
								iExtras = 0;
								PlaySound ("wav/extras.wav");
							}
							break;
						case SDLK_f:
							if (iScreen == 1)
							{
								Zoom (1);
								iExtras = 0;
								PlaySound ("wav/extras.wav");
							}
							break;
						case SDLK_QUOTE:
							if (iScreen == 1)
							{
								if ((event.key.keysym.mod & KMOD_LSHIFT) ||
									(event.key.keysym.mod & KMOD_RSHIFT))
								{
									Sprinkle();
									PlaySound ("wav/extras.wav");
									iChanged++;
								} else {
									SetLocation (iCurRoom, iSelected,
										(32 * iLastX) + iLastTile, iLastMod);
									PlaySound ("wav/ok_close.wav");
									iChanged++;
								}
							}
							break;
						case SDLK_h:
							if (iScreen == 1)
							{
								FlipRoom (1);
								PlaySound ("wav/extras.wav");
								iChanged++;
							}
							break;
						case SDLK_y:
							if (iScreen == 3)
							{
								if (arEventsNext[iCurLevel][iChangeEvent] != 1)
								{
									arEventsNext[iCurLevel][iChangeEvent] = 1;
									PlaySound ("wav/check_box.wav");
									iChanged++;
								}
							}
							break;
						case SDLK_n:
							if (iScreen == 3)
							{
								if (arEventsNext[iCurLevel][iChangeEvent] != 0)
								{
									arEventsNext[iCurLevel][iChangeEvent] = 0;
									PlaySound ("wav/check_box.wav");
									iChanged++;
								}
							}
							break;
						case SDLK_v:
							if (iScreen == 1)
							{
								if ((event.key.keysym.mod & KMOD_LCTRL) ||
									(event.key.keysym.mod & KMOD_RCTRL))
								{
									CopyPaste (2);
									PlaySound ("wav/extras.wav");
									iChanged++;
								} else {
									FlipRoom (2);
									PlaySound ("wav/extras.wav");
									iChanged++;
								}
							}
							break;
						case SDLK_c:
							if (iScreen == 1)
							{
								if ((event.key.keysym.mod & KMOD_LCTRL) ||
									(event.key.keysym.mod & KMOD_RCTRL))
								{
									CopyPaste (1);
									PlaySound ("wav/extras.wav");
								}
							}
							break;
						case SDLK_i:
							if (iScreen == 1)
							{
								if (iInfo == 0) { iInfo = 1; } else { iInfo = 0; }
							}
							break;
						case SDLK_0: /*** empty ***/
						case SDLK_KP_0:
							if (iScreen == 1)
							{
								SetLocation (iCurRoom, iSelected, 0x00, 0x00);
								PlaySound ("wav/ok_close.wav"); iChanged++;
							}
							break;
						case SDLK_1: /*** floor ***/
						case SDLK_KP_1:
							if (iScreen == 1)
							{
								if (cCurType == 'd')
								{
									SetLocation (iCurRoom, iSelected, 0x01, 0x00);
								} else {
									SetLocation (iCurRoom, iSelected, 0x01, 0x01);
								}
								PlaySound ("wav/ok_close.wav"); iChanged++;
							}
							break;
						case SDLK_2: /*** loose tile ***/
						case SDLK_KP_2:
							if (iScreen == 1)
							{
								SetLocation (iCurRoom, iSelected, 0x0B, 0x00);
								PlaySound ("wav/ok_close.wav"); iChanged++;
							}
							break;
						case SDLK_3: /*** closed gate ***/
						case SDLK_KP_3:
							if (iScreen == 1)
							{
								SetLocation (iCurRoom, iSelected, 0x04, 0x02);
								PlaySound ("wav/ok_close.wav"); iChanged++;
							}
							break;
						case SDLK_4: /*** open gate ***/
						case SDLK_KP_4:
							if (iScreen == 1)
							{
								SetLocation (iCurRoom, iSelected, 0x04, 0x01);
								PlaySound ("wav/ok_close.wav"); iChanged++;
							}
							break;
						case SDLK_5: /*** torch ***/
						case SDLK_KP_5:
							if (iScreen == 1)
							{
								SetLocation (iCurRoom, iSelected, 0x13, 0x00);
								PlaySound ("wav/ok_close.wav"); iChanged++;
							}
							break;
						case SDLK_6: /*** spikes ***/
						case SDLK_KP_6:
							if (iScreen == 1)
							{
								SetLocation (iCurRoom, iSelected, 0x02, 0x00);
								PlaySound ("wav/ok_close.wav"); iChanged++;
							}
							break;
						case SDLK_7: /*** small pillar ***/
						case SDLK_KP_7:
							if (iScreen == 1)
							{
								SetLocation (iCurRoom, iSelected, 0x03, 0x00);
								PlaySound ("wav/ok_close.wav"); iChanged++;
							}
							break;
						case SDLK_8: /*** chomper ***/
						case SDLK_KP_8:
							if (iScreen == 1)
							{
								SetLocation (iCurRoom, iSelected, 0x12, 0x00);
								PlaySound ("wav/ok_close.wav"); iChanged++;
							}
							break;
						case SDLK_9: /*** wall ***/
						case SDLK_KP_9:
							if (iScreen == 1)
							{
								/*** Yes, 0x01. Palace without wall pattern. ***/
								SetLocation (iCurRoom, iSelected, 0x14, 0x01);
								PlaySound ("wav/ok_close.wav"); iChanged++;
							}
							break;
						default: break;
					}
					ShowScreen();
					break;
				case SDL_MOUSEMOTION:
					iOldXPos = iXPos;
					iOldYPos = iYPos;
					iXPos = event.motion.x;
					iYPos = event.motion.y;
					if ((iOldXPos == iXPos) && (iOldYPos == iYPos)) { break; }

					/*** Emulator information. ***/
					if (OnLevelBar() == 1)
					{
						if (iEmulator != 1) { iEmulator = 1; ShowScreen(); }
					} else {
						if (iEmulator != 0) { iEmulator = 0; ShowScreen(); }
					}

					if (iScreen == 3)
					{
						/* A tiny Easter egg: the mouse looks up if the user
						 * hovers over it.
						 */
						if (InArea (538, 81, 538 + 34, 81 + 8) == 1)
						{
							if (iMouse == 0) { iMouse = 1; ShowScreen(); }
						} else {
							if (iMouse == 1) { iMouse = 0; ShowScreen(); }
						}
					}

					if (iScreen == 2)
					{
						if (iMovingRoom != 0) { ShowScreen(); }
					}

					if (iScreen == 1)
					{
						/*** User hovers over tiles in the upper row. ***/
						if ((InArea (iHor[1], iVer1 + iTTP1, iHor[2], iVer2 + iTTPO)
							== 1) && (iSelected != 1))
							{ iSelected = 1; ShowScreen(); }
						else if ((InArea (iHor[2], iVer1 + iTTP1, iHor[3], iVer2 + iTTPO)
							== 1) && (iSelected != 2))
							{ iSelected = 2; ShowScreen(); }
						else if ((InArea (iHor[3], iVer1 + iTTP1, iHor[4], iVer2 + iTTPO)
							== 1) && (iSelected != 3))
							{ iSelected = 3; ShowScreen(); }
						else if ((InArea (iHor[4], iVer1 + iTTP1, iHor[5], iVer2 + iTTPO)
							== 1) && (iSelected != 4))
							{ iSelected = 4; ShowScreen(); }
						else if ((InArea (iHor[5], iVer1 + iTTP1, iHor[6], iVer2 + iTTPO)
							== 1) && (iSelected != 5))
							{ iSelected = 5; ShowScreen(); }
						else if ((InArea (iHor[6], iVer1 + iTTP1, iHor[7], iVer2 + iTTPO)
							== 1) && (iSelected != 6))
							{ iSelected = 6; ShowScreen(); }
						else if ((InArea (iHor[7], iVer1 + iTTP1, iHor[8], iVer2 + iTTPO)
							== 1) && (iSelected != 7))
							{ iSelected = 7; ShowScreen(); }
						else if ((InArea (iHor[8], iVer1 + iTTP1, iHor[9], iVer2 + iTTPO)
							== 1) && (iSelected != 8))
							{ iSelected = 8; ShowScreen(); }
						else if ((InArea (iHor[9], iVer1 + iTTP1, iHor[10], iVer2 + iTTPO)
							== 1) && (iSelected != 9))
							{ iSelected = 9; ShowScreen(); }
						else if ((InArea (iHor[10], iVer1 + iTTP1, iHor[10] + iDX,
							iVer2 + iTTPO) == 1) && (iSelected != 10))
						{ iSelected = 10; ShowScreen(); }

						/*** User hovers over tiles in the middle row. ***/
						else if ((InArea (iHor[1], iVer2 + iTTPO, iHor[2], iVer3 + iTTPO)
							== 1) && (iSelected != 11))
							{ iSelected = 11; ShowScreen(); }
						else if ((InArea (iHor[2], iVer2 + iTTPO, iHor[3], iVer3 + iTTPO)
							== 1) && (iSelected != 12))
							{ iSelected = 12; ShowScreen(); }
						else if ((InArea (iHor[3], iVer2 + iTTPO, iHor[4], iVer3 + iTTPO)
							== 1) && (iSelected != 13))
							{ iSelected = 13; ShowScreen(); }
						else if ((InArea (iHor[4], iVer2 + iTTPO, iHor[5], iVer3 + iTTPO)
							== 1) && (iSelected != 14))
							{ iSelected = 14; ShowScreen(); }
						else if ((InArea (iHor[5], iVer2 + iTTPO, iHor[6], iVer3 + iTTPO)
							== 1) && (iSelected != 15))
							{ iSelected = 15; ShowScreen(); }
						else if ((InArea (iHor[6], iVer2 + iTTPO, iHor[7], iVer3 + iTTPO)
							== 1) && (iSelected != 16))
							{ iSelected = 16; ShowScreen(); }
						else if ((InArea (iHor[7], iVer2 + iTTPO, iHor[8], iVer3 + iTTPO)
							== 1) && (iSelected != 17))
							{ iSelected = 17; ShowScreen(); }
						else if ((InArea (iHor[8], iVer2 + iTTPO, iHor[9], iVer3 + iTTPO)
							== 1) && (iSelected != 18))
							{ iSelected = 18; ShowScreen(); }
						else if ((InArea (iHor[9], iVer2 + iTTPO, iHor[10], iVer3 + iTTPO)
							== 1) && (iSelected != 19))
							{ iSelected = 19; ShowScreen(); }
						else if ((InArea (iHor[10], iVer2 + iTTPO, iHor[10] + iDX,
							iVer3 + iTTPO) == 1) && (iSelected != 20))
						{ iSelected = 20; ShowScreen(); }

						/*** User hovers over tiles in the bottom row. ***/
						else if ((InArea (iHor[1], iVer3 + iTTPO, iHor[2],
							iVer3 + iDY + iTTPO) == 1) && (iSelected != 21))
							{ iSelected = 21; ShowScreen(); }
						else if ((InArea (iHor[2], iVer3 + iTTPO, iHor[3],
							iVer3 + iDY + iTTPO) == 1) && (iSelected != 22))
							{ iSelected = 22; ShowScreen(); }
						else if ((InArea (iHor[3], iVer3 + iTTPO, iHor[4],
							iVer3 + iDY + iTTPO) == 1) && (iSelected != 23))
							{ iSelected = 23; ShowScreen(); }
						else if ((InArea (iHor[4], iVer3 + iTTPO, iHor[5],
							iVer3 + iDY + iTTPO) == 1) && (iSelected != 24))
							{ iSelected = 24; ShowScreen(); }
						else if ((InArea (iHor[5], iVer3 + iTTPO, iHor[6],
							iVer3 + iDY + iTTPO) == 1) && (iSelected != 25))
							{ iSelected = 25; ShowScreen(); }
						else if ((InArea (iHor[6], iVer3 + iTTPO, iHor[7],
							iVer3 + iDY + iTTPO) == 1) && (iSelected != 26))
							{ iSelected = 26; ShowScreen(); }
						else if ((InArea (iHor[7], iVer3 + iTTPO, iHor[8],
							iVer3 + iDY + iTTPO) == 1) && (iSelected != 27))
							{ iSelected = 27; ShowScreen(); }
						else if ((InArea (iHor[8], iVer3 + iTTPO, iHor[9],
							iVer3 + iDY + iTTPO) == 1) && (iSelected != 28))
							{ iSelected = 28; ShowScreen(); }
						else if ((InArea (iHor[9], iVer3 + iTTPO, iHor[10],
							iVer3 + iDY + iTTPO) == 1) && (iSelected != 29))
							{ iSelected = 29; ShowScreen(); }
						else if ((InArea (iHor[10], iVer3 + iTTPO, iHor[10] + iDX,
							iVer3 + iDY + iTTPO) == 1) && (iSelected != 30))
						{ iSelected = 30; ShowScreen(); }

						/*** extras ***/
						if ((InArea (530, 3, 539, 12) == 1) && (iExtras != 1))
							{ iExtras = 1; ShowScreen(); }
						else if ((InArea (540, 3, 549, 12) == 1) && (iExtras != 2))
							{ iExtras = 2; ShowScreen(); }
						else if ((InArea (550, 3, 559, 12) == 1) && (iExtras != 3))
							{ iExtras = 3; ShowScreen(); }
						else if ((InArea (560, 3, 569, 12) == 1) && (iExtras != 4))
							{ iExtras = 4; ShowScreen(); }
						else if ((InArea (570, 3, 579, 12) == 1) && (iExtras != 5))
							{ iExtras = 5; ShowScreen(); }
						else if ((InArea (530, 13, 539, 22) == 1) && (iExtras != 6))
							{ iExtras = 6; ShowScreen(); }
						else if ((InArea (540, 13, 549, 22) == 1) && (iExtras != 7))
							{ iExtras = 7; ShowScreen(); }
						else if ((InArea (550, 13, 559, 22) == 1) && (iExtras != 8))
							{ iExtras = 8; ShowScreen(); }
						else if ((InArea (560, 13, 569, 22) == 1) && (iExtras != 9))
							{ iExtras = 9; ShowScreen(); }
						else if ((InArea (570, 13, 579, 22) == 1) && (iExtras != 10))
							{ iExtras = 10; ShowScreen(); }
						else if ((InArea (530, 3, 579, 22) == 0) && (iExtras != 0))
							{ iExtras = 0; ShowScreen(); }
					}

					break;
				case SDL_MOUSEBUTTONDOWN:
					if (event.button.button == 1)
					{
						if (InArea (0, 50, 0 + 25, 50 + 386) == 1) /*** left arrow ***/
						{
							if (arRoomLinks[iCurLevel][iCurRoom][1] != 0) { iDownAt = 1; }
						}
						if (InArea (587, 50, 587 + 25, 50 + 386) == 1) /*** right arrow ***/
						{
							if (arRoomLinks[iCurLevel][iCurRoom][2] != 0) { iDownAt = 2; }
						}
						if (InArea (25, 25, 25 + 562, 25 + 25) == 1) /*** up arrow ***/
						{
							if (arRoomLinks[iCurLevel][iCurRoom][3] != 0) { iDownAt = 3; }
						}
						if (InArea (25, 436, 25 + 562, 436 + 25) == 1) /*** down arrow ***/
						{
							if (arRoomLinks[iCurLevel][iCurRoom][4] != 0) { iDownAt = 4; }
						}
						if (InArea (0, 25, 25, 50) == 1) /*** rooms ***/
						{
							if (arBrokenRoomLinks[iCurLevel] == 0)
							{
								iDownAt = 5;
							} else {
								iDownAt = 11;
							}
						}
						if (InArea (587, 25, 587 + 25, 25 + 25) == 1) /*** events ***/
						{
							iDownAt = 6;
						}
						if (InArea (0, 436, 0 + 25, 436 + 25) == 1) /*** save ***/
						{
							iDownAt = 7;
						}
						if (InArea (587, 436, 587 + 25, 436 + 25) == 1) /*** quit ***/
						{
							iDownAt = 8;
						}
						if (InArea (0, 0, 25, 25) == 1) /*** previous ***/
						{
							iDownAt = 9;
						}
						if (InArea (587, 0, 587 + 25, 0 + 25) == 1) /*** next ***/
						{
							iDownAt = 10;
						}

						if (iScreen == 2)
						{
							if (arBrokenRoomLinks[iCurLevel] == 0)
							{
								for (iRoomLoop = 0; iRoomLoop < ROOMS; iRoomLoop++) /*** x ***/
								{
									/*** y ***/
									for (iRoomLoop2 = 0; iRoomLoop2 < ROOMS; iRoomLoop2++)
									{
										if (InArea (214 + (iRoomLoop * 15), 64 + (iRoomLoop2 * 15),
											228 + (iRoomLoop * 15), 78 + (iRoomLoop2 * 15)) == 1)
										{
											if (arMovingRooms[iRoomLoop + 1][iRoomLoop2 + 1] != 0)
											{
												iMovingNewBusy = 0;
												iMovingRoom =
													arMovingRooms[iRoomLoop + 1][iRoomLoop2 + 1];
											}
										}
									}
								}
								/*** y ***/
								for (iRoomLoop2 = 0; iRoomLoop2 < ROOMS; iRoomLoop2++)
								{
									if (InArea (189, 64 + (iRoomLoop2 * 15),
										189 + 14, 64 + 14 + (iRoomLoop2 * 15)) == 1)
									{
										if (arMovingRooms[25][iRoomLoop2 + 1] != 0)
										{
											iMovingNewBusy = 0;
											iMovingRoom = arMovingRooms[25][iRoomLoop2 + 1];
										}
									}
								}

								/*** rooms broken ***/
								if (InArea (546, 66, 546 + 25, 66 + 25) == 1)
								{
									iDownAt = 11;
								}
							} else {
								MouseSelectAdj();
							}
						}
					}
					ShowScreen();
					break;
				case SDL_MOUSEBUTTONUP:
					iDownAt = 0;
					if (event.button.button == 1) /*** left mouse button, change ***/
					{
						if (InArea (0, 50, 0 + 25, 50 + 386) == 1) /*** left arrow ***/
						{
							if (arRoomLinks[iCurLevel][iCurRoom][1] != 0)
							{
								iCurRoom = arRoomLinks[iCurLevel][iCurRoom][1];
								PlaySound ("wav/scroll.wav");
							}
						}
						if (InArea (587, 50, 587 + 25, 50 + 386) == 1) /*** right arrow ***/
						{
							if (arRoomLinks[iCurLevel][iCurRoom][2] != 0)
							{
								iCurRoom = arRoomLinks[iCurLevel][iCurRoom][2];
								PlaySound ("wav/scroll.wav");
							}
						}
						if (InArea (25, 25, 25 + 562, 25 + 25) == 1) /*** up arrow ***/
						{
							if (arRoomLinks[iCurLevel][iCurRoom][3] != 0)
							{
								iCurRoom = arRoomLinks[iCurLevel][iCurRoom][3];
								PlaySound ("wav/scroll.wav");
							}
						}
						if (InArea (25, 436, 25 + 562, 436 + 25) == 1) /*** down arrow ***/
						{
							if (arRoomLinks[iCurLevel][iCurRoom][4] != 0)
							{
								iCurRoom = arRoomLinks[iCurLevel][iCurRoom][4];
								PlaySound ("wav/scroll.wav");
							}
						}
						if (InArea (0, 25, 25, 50) == 1) /*** rooms ***/
						{
							if (iScreen != 2)
							{
								iScreen = 2; iMovingRoom = 0; iMovingNewBusy = 0;
								iChangingBrokenRoom = iCurRoom;
								iChangingBrokenSide = 1;
								PlaySound ("wav/screen2or3.wav");
							}
						}
						if (InArea (587, 25, 587 + 25, 25 + 25) == 1) /*** events ***/
						{
							if (iScreen == 2)
							{
								arBrokenRoomLinks[iCurLevel] = BrokenRoomLinks (0);
							}
							if (iScreen != 3)
							{
								iScreen = 3;
								PlaySound ("wav/screen2or3.wav");
							}
						}
						if (InArea (0, 436, 0 + 25, 436 + 25) == 1) /*** save ***/
						{
							if (iChanged != 0) { CallSave(); }
						}
						if (InArea (587, 436, 587 + 25, 436 + 25) == 1) /*** quit ***/
						{
							switch (iScreen)
							{
								case 1:
									Quit(); break;
								case 2:
									arBrokenRoomLinks[iCurLevel] = BrokenRoomLinks (0);
									iScreen = 1; break;
								case 3:
									iScreen = 1; break;
							}
						}
						if (InArea (0, 0, 25, 25) == 1) /*** previous ***/
						{
							if (iChanged != 0) { InitPopUpSave(); }
							Prev();
							ShowScreen(); break; /*** ? ***/
						}
						if (InArea (587, 0, 587 + 25, 0 + 25) == 1) /*** next ***/
						{
							if (iChanged != 0) { InitPopUpSave(); }
							Next();
							ShowScreen(); break; /*** ? ***/
						}
						if (OnLevelBar() == 1) /*** level bar ***/
						{
							RunLevel (iCurLevel);
						}

						if (iScreen == 1)
						{
							if (InArea (iHor[1], iVer1 + iTTP1, iHor[10] + iDX,
								iVer3 + iDY + iTTPO) == 1) /*** middle field ***/
							{
								keystate = SDL_GetKeyboardState (NULL);
								if ((keystate[SDL_SCANCODE_LSHIFT]) ||
									(keystate[SDL_SCANCODE_RSHIFT]))
								{
									SetLocation (iCurRoom, iSelected,
										(32 * iLastX) + iLastTile, iLastMod);
									PlaySound ("wav/ok_close.wav"); iChanged++;
								} else {
									ChangePos();
									ShowScreen(); break; /*** ? ***/
								}
							}

							/*** 1 ***/
							if (InArea (530, 3, 539, 12) == 1)
							{
								Zoom (0);
								iExtras = 0;
								PlaySound ("wav/extras.wav");
							}

							/*** 4 ***/
							if (InArea (560, 3, 569, 12) == 1)
							{
								InitScreenAction ("env");
							}

							/*** 6 ***/
							if (InArea (530, 13, 539, 22) == 1)
							{
								Sprinkle();
								PlaySound ("wav/extras.wav");
								iChanged++;
							}

							/*** 8 ***/
							if (InArea (550, 13, 559, 22) == 1)
							{
								FlipRoom (1);
								PlaySound ("wav/extras.wav");
								iChanged++;
							}

							/*** 3 ***/
							if (InArea (550, 3, 559, 12) == 1)
							{
								FlipRoom (2);
								PlaySound ("wav/extras.wav");
								iChanged++;
							}

							/*** 2 ***/
							if (InArea (540, 3, 549, 12) == 1)
							{
								CopyPaste (1);
								PlaySound ("wav/extras.wav");
							}

							/*** 7 ***/
							if (InArea (540, 13, 549, 22) == 1)
							{
								CopyPaste (2);
								PlaySound ("wav/extras.wav");
								iChanged++;
							}

							/*** 5 ***/
							if (InArea (570, 3, 579, 12) == 1)
							{
								Help(); SDL_SetCursor (curArrow);
							}

							/*** 10 ***/
							if (InArea (570, 13, 579, 22) == 1)
							{
								EXE();
								SDL_SetCursor (curArrow);
							}
						}

						if (iScreen == 2) /*** room links screen ***/
						{
							if (arBrokenRoomLinks[iCurLevel] == 0)
							{
								for (iRoomLoop = 0; iRoomLoop < ROOMS; iRoomLoop++) /*** x ***/
								{
									/*** y ***/
									for (iRoomLoop2 = 0; iRoomLoop2 < ROOMS; iRoomLoop2++)
									{
										if (InArea (214 + (iRoomLoop * 15), 64 + (iRoomLoop2 * 15),
											228 + (iRoomLoop * 15), 78 + (iRoomLoop2 * 15)) == 1)
										{
											if (iMovingRoom != 0)
											{
												if (arMovingRooms[iRoomLoop + 1][iRoomLoop2 + 1] == 0)
												{
													RemoveOldRoom();
													AddNewRoom (iRoomLoop + 1,
														iRoomLoop2 + 1, iMovingRoom);
													iChanged++;
												}
												iMovingRoom = 0; iMovingNewBusy = 0;
											}
										}
									}
								}
								/*** y ***/
								for (iRoomLoop2 = 0; iRoomLoop2 < ROOMS; iRoomLoop2++)
								{
									if (InArea (189, 64 + (iRoomLoop2 * 15),
										189 + 14, 64 + 14 + (iRoomLoop2 * 15)) == 1)
									{
										if (iMovingRoom != 0)
										{
											if (arMovingRooms[25][iRoomLoop2 + 1] == 0)
											{
												RemoveOldRoom();
												AddNewRoom (25, iRoomLoop2 + 1, iMovingRoom);
												iChanged++;
											}
											iMovingRoom = 0; iMovingNewBusy = 0;
										}
									}
								}

								/*** rooms broken ***/
								if (InArea (546, 66, 546 + 25, 66 + 25) == 1)
								{
									arBrokenRoomLinks[iCurLevel] = 1;
									PlaySound ("wav/screen2or3.wav");
								}
							} else {
								if (MouseSelectAdj() == 1)
								{
									LinkPlus();
								}
							}
						}

						if (iScreen == 3) /*** events screen ***/
						{
							/*** edit this event ***/
							if (InArea (263, 60, 263 + 13, 60 + 20) == 1)
								{ ChangeEvent (-10, 0); }
							if (InArea (278, 60, 278 + 13, 60 + 20) == 1)
								{ ChangeEvent (-1, 0); }
							if (InArea (348, 60, 348 + 13, 60 + 20) == 1)
								{ ChangeEvent (1, 0); }
							if (InArea (363, 60, 363 + 13, 60 + 20) == 1)
								{ ChangeEvent (10, 0); }

							/*** room ***/
							if ((iYPos >= 115 * iScale) &&
								(iYPos <= (115 + 14) * iScale))
							{
								for (iRoomLoop = 1; iRoomLoop <= ROOMS; iRoomLoop++)
								{
									if ((iXPos >= (217 + ((iRoomLoop - 1) * 15)) * iScale)
										&& (iXPos <= ((217 + 14) +
										((iRoomLoop - 1) * 15)) * iScale))
									{
										arEventsRoom[iCurLevel][iChangeEvent] = iRoomLoop;
										PlaySound ("wav/check_box.wav");
										iChanged++;
									}
								}
							}

							/*** tile ***/
							for (iColLoop = 1; iColLoop <= 3; iColLoop++)
							{
								for (iRowLoop = 1; iRowLoop <= 10; iRowLoop++)
								{
									if ((iXPos >= (382 + ((iRowLoop - 1) * 15)) * iScale)
										&& (iXPos <= ((382 + 14) +
										((iRowLoop - 1) * 15)) * iScale))
									{
										if ((iYPos >= (155 + ((iColLoop - 1) * 15)) * iScale)
											&& (iYPos <= ((155 + 14) +
											((iColLoop - 1) * 15)) * iScale))
										{
											arEventsTile[iCurLevel][iChangeEvent] =
												((iColLoop - 1) * 10) + iRowLoop;
											PlaySound ("wav/check_box.wav");
											iChanged++;
										}
									}
								}
							}

							/*** next ***/
							if (InArea (502, 225, 502 + 14, 225 + 14) == 1) /*** N ***/
							{
								if (arEventsNext[iCurLevel][iChangeEvent] != 0)
								{
									arEventsNext[iCurLevel][iChangeEvent] = 0;
									PlaySound ("wav/check_box.wav");
									iChanged++;
								}
							}
							if (InArea (517, 225, 517 + 14, 225 + 14) == 1) /*** Y ***/
							{
								if (arEventsNext[iCurLevel][iChangeEvent] != 1)
								{
									arEventsNext[iCurLevel][iChangeEvent] = 1;
									PlaySound ("wav/check_box.wav");
									iChanged++;
								}
							}
						}
					}
					if (event.button.button == 2) /*** middle mouse button, clear ***/
					{
						if (iScreen == 1) { ClearRoom(); }
					}
					if (event.button.button == 3) /*** right mouse button, randomize ***/
					{
						if (iScreen == 1)
						{
							for (iRoomLoop = 1; iRoomLoop <= ROOMS; iRoomLoop++)
							{
								for (iTileLoop = 1; iTileLoop <= TILES; iTileLoop++)
								{
									UseTile (-1, iTileLoop, iRoomLoop);
								}
							}
							PlaySound ("wav/ok_close.wav");
							iChanged++;
						}
						if (iScreen == 2)
						{
							if (arBrokenRoomLinks[iCurLevel] == 1)
							{
								if (MouseSelectAdj() == 1)
								{
									LinkMinus();
								}
							}
						}
					}
					ShowScreen();
					break;
				case SDL_MOUSEWHEEL:
					if (event.wheel.y > 0) /*** scroll wheel up ***/
					{
						if (InArea (iHor[1], iVer1 + iTTP1, iHor[10] + iDX,
							iVer3 + iDY + iTTPO) == 1) /*** middle field ***/
						{
							keystate = SDL_GetKeyboardState (NULL);
							if ((keystate[SDL_SCANCODE_LSHIFT]) ||
								(keystate[SDL_SCANCODE_RSHIFT]))
							{ /*** right ***/
								if (arRoomLinks[iCurLevel][iCurRoom][2] != 0)
								{
									iCurRoom = arRoomLinks[iCurLevel][iCurRoom][2];
									PlaySound ("wav/scroll.wav");
								}
							} else { /*** up ***/
								if (arRoomLinks[iCurLevel][iCurRoom][3] != 0)
								{
									iCurRoom = arRoomLinks[iCurLevel][iCurRoom][3];
									PlaySound ("wav/scroll.wav");
								}
							}
						}
					}
					if (event.wheel.y < 0) /*** scroll wheel down ***/
					{
						if (InArea (iHor[1], iVer1 + iTTP1, iHor[10] + iDX,
							iVer3 + iDY + iTTPO) == 1) /*** middle field ***/
						{
							keystate = SDL_GetKeyboardState (NULL);
							if ((keystate[SDL_SCANCODE_LSHIFT]) ||
								(keystate[SDL_SCANCODE_RSHIFT]))
							{ /*** left ***/
								if (arRoomLinks[iCurLevel][iCurRoom][1] != 0)
								{
									iCurRoom = arRoomLinks[iCurLevel][iCurRoom][1];
									PlaySound ("wav/scroll.wav");
								}
							} else { /*** down ***/
								if (arRoomLinks[iCurLevel][iCurRoom][4] != 0)
								{
									iCurRoom = arRoomLinks[iCurLevel][iCurRoom][4];
									PlaySound ("wav/scroll.wav");
								}
							}
						}
					}
					ShowScreen();
					break;
				case SDL_WINDOWEVENT:
					if (event.window.event == SDL_WINDOWEVENT_EXPOSED)
						{ ShowScreen(); } break;
				case SDL_QUIT:
					Quit(); break;
				default: break;
			}
		}

		/*** prevent CPU eating ***/
		gamespeed = REFRESH_PROG;
		while ((SDL_GetTicks() - looptime) < gamespeed)
		{
			SDL_Delay (10);
		}
		looptime = SDL_GetTicks();
	}
}
/*****************************************************************************/
void InitPopUpSave (void)
/*****************************************************************************/
{
	int iPopUp;
	SDL_Event event;

	iPopUp = 1;

	PlaySound ("wav/popup_yn.wav");
	ShowPopUpSave();
	while (iPopUp == 1)
	{
		while (SDL_PollEvent (&event))
		{
			switch (event.type)
			{
				case SDL_CONTROLLERBUTTONDOWN:
					/*** Nothing for now. ***/
					break;
				case SDL_CONTROLLERBUTTONUP:
					switch (event.cbutton.button)
					{
						case SDL_CONTROLLER_BUTTON_A:
							CallSave(); iPopUp = 0; break;
						case SDL_CONTROLLER_BUTTON_B:
							iPopUp = 0; break;
					}
					break;
				case SDL_KEYDOWN:
					switch (event.key.keysym.sym)
					{
						case SDLK_ESCAPE:
						case SDLK_n:
							iPopUp = 0; break;
						case SDLK_y:
							CallSave(); iPopUp = 0; break;
						default: break;
					}
					break;
				case SDL_MOUSEMOTION:
					iXPos = event.motion.x;
					iYPos = event.motion.y;
					break;
				case SDL_MOUSEBUTTONDOWN:
					if (event.button.button == 1)
					{
						if (InArea (400, 320, 400 + 85, 320 + 32) == 1) /*** Yes ***/
						{
							iYesOn = 1;
							ShowPopUpSave();
						}
						if (InArea (127, 320, 127 + 85, 320 + 32) == 1) /*** No ***/
						{
							iNoOn = 1;
							ShowPopUpSave();
						}
					}
					break;
				case SDL_MOUSEBUTTONUP:
					iYesOn = 0;
					iNoOn = 0;
					if (event.button.button == 1)
					{
						if (InArea (400, 320, 400 + 85, 320 + 32) == 1) /*** Yes ***/
						{
							CallSave(); iPopUp = 0;
						}
						if (InArea (127, 320, 127 + 85, 320 + 32) == 1) /*** No ***/
						{
							iPopUp = 0;
						}
					}
					ShowPopUpSave(); break;
				case SDL_WINDOWEVENT:
					if (event.window.event == SDL_WINDOWEVENT_EXPOSED)
						{ ShowScreen(); ShowPopUpSave(); } break;
				case SDL_QUIT:
					Quit(); break;
			}
		}

		/*** prevent CPU eating ***/
		gamespeed = REFRESH_PROG;
		while ((SDL_GetTicks() - looptime) < gamespeed)
		{
			SDL_Delay (10);
		}
		looptime = SDL_GetTicks();
	}
	PlaySound ("wav/popup_close.wav");
	ShowScreen();
}
/*****************************************************************************/
void ShowPopUpSave (void)
/*****************************************************************************/
{
	char arText[9 + 2][MAX_TEXT + 2];

	/*** faded background ***/
	ShowImage (imgfadedl, 0, 0, "imgfadedl");

	/*** popup ***/
	ShowImage (imgpopup_yn, 110, 92, "imgpopup_yn");

	/*** Yes ***/
	switch (iYesOn)
	{
		case 0: ShowImage (imgyes[1], 400, 320, "imgyes[1]"); break; /*** off ***/
		case 1: ShowImage (imgyes[2], 400, 320, "imgyes[2]"); break; /*** on ***/
	}

	/*** No ***/
	switch (iNoOn)
	{
		case 0: ShowImage (imgno[1], 127, 320, "imgno[1]"); break; /*** off ***/
		case 1: ShowImage (imgno[2], 127, 320, "imgno[2]"); break; /*** on ***/
	}

	if (iChanged == 1)
	{
		snprintf (arText[0], MAX_TEXT, "%s", "You made an unsaved change.");
		snprintf (arText[1], MAX_TEXT, "%s", "Do you want to save it?");
	} else {
		snprintf (arText[0], MAX_TEXT, "%s", "There are unsaved changes.");
		snprintf (arText[1], MAX_TEXT, "%s", "Do you wish to save these?");
	}

	DisplayText (140, 121, FONT_SIZE_15, arText, 2, font1);

	/*** refresh screen ***/
	SDL_RenderPresent (ascreen);
}
/*****************************************************************************/
void LoadFonts (void)
/*****************************************************************************/
{
	font1 = TTF_OpenFont ("ttf/Bitstream-Vera-Sans-Bold.ttf",
		FONT_SIZE_15 * iScale);
	if (font1 == NULL) { printf ("[FAILED] Font gone!\n"); exit (EXIT_ERROR); }
	font2 = TTF_OpenFont ("ttf/Bitstream-Vera-Sans-Bold.ttf",
		FONT_SIZE_11 * iScale);
	if (font2 == NULL) { printf ("[FAILED] Font gone!\n"); exit (EXIT_ERROR); }
	font3 = TTF_OpenFont ("ttf/Bitstream-Vera-Sans-Bold.ttf",
		FONT_SIZE_20 * iScale);
	if (font3 == NULL) { printf ("[FAILED] Font gone!\n"); exit (EXIT_ERROR); }
}
/*****************************************************************************/
void MixAudio (void *unused, Uint8 *stream, int iLen)
/*****************************************************************************/
{
	int iTemp;
	int iAmount;

	if (unused != NULL) { } /*** To prevent warnings. ***/

	SDL_memset (stream, 0, iLen); /*** SDL2 ***/
	for (iTemp = 0; iTemp < NUM_SOUNDS; iTemp++)
	{
		iAmount = (sounds[iTemp].dlen-sounds[iTemp].dpos);
		if (iAmount > iLen)
		{
			iAmount = iLen;
		}
		SDL_MixAudio (stream, &sounds[iTemp].data[sounds[iTemp].dpos], iAmount,
			SDL_MIX_MAXVOLUME);
		sounds[iTemp].dpos += iAmount;
	}
}
/*****************************************************************************/
void PlaySound (char *sFile)
/*****************************************************************************/
{
	int iIndex;
	SDL_AudioSpec wave;
	Uint8 *data;
	Uint32 dlen;
	SDL_AudioCVT cvt;

	if (iNoAudio == 1) { return; }
	for (iIndex = 0; iIndex < NUM_SOUNDS; iIndex++)
	{
		if (sounds[iIndex].dpos == sounds[iIndex].dlen)
		{
			break;
		}
	}
	if (iIndex == NUM_SOUNDS) { return; }

	if (SDL_LoadWAV (sFile, &wave, &data, &dlen) == NULL)
	{
		printf ("[FAILED] Could not load %s: %s!\n", sFile, SDL_GetError());
		exit (EXIT_ERROR);
	}
	SDL_BuildAudioCVT (&cvt, wave.format, wave.channels, wave.freq, AUDIO_S16, 2,
		44100);
	/*** The "+ 1" is a workaround for SDL bug #2274. ***/
	cvt.buf = (Uint8 *)malloc (dlen * (cvt.len_mult + 1));
	memcpy (cvt.buf, data, dlen);
	cvt.len = dlen;
	SDL_ConvertAudio (&cvt);
	SDL_FreeWAV (data);

	if (sounds[iIndex].data)
	{
		free (sounds[iIndex].data);
	}
	SDL_LockAudio();
	sounds[iIndex].data = cvt.buf;
	sounds[iIndex].dlen = cvt.len_cvt;
	sounds[iIndex].dpos = 0;
	SDL_UnlockAudio();
}
/*****************************************************************************/
void PreLoadSet (char cTypeP, int iTile, int iMod)
/*****************************************************************************/
{
	char sDir[MAX_PATHFILE + 2];
	char sImage[MAX_IMG + 2];
	int iBarHeight;

	switch (cTypeP)
	{
		case 'd':
			/*** regular ***/
			snprintf (sDir, MAX_PATHFILE, "png%sdungeon%s", SLASH, SLASH);
			snprintf (sImage, MAX_IMG, "%s%02x_%02x.png", sDir, iTile, iMod);
			imgd[iTile][iMod][1] = IMG_LoadTexture (ascreen, sImage);
			/*** selected ***/
			snprintf (sDir, MAX_PATHFILE, "png%ssdungeon%s", SLASH, SLASH);
			snprintf (sImage, MAX_IMG, "%s%02x_%02x.png", sDir, iTile, iMod);
			imgd[iTile][iMod][2] = IMG_LoadTexture (ascreen, sImage);
			if ((!imgd[iTile][iMod][1]) || (!imgd[iTile][iMod][2]))
			{
				printf ("[FAILED] IMG_LoadTexture: %s!\n", IMG_GetError());
				exit (EXIT_ERROR);
			}
			break;
		case 'p':
			/*** regular ***/
			snprintf (sDir, MAX_PATHFILE, "png%spalace%s", SLASH, SLASH);
			snprintf (sImage, MAX_IMG, "%s%02x_%02x.png", sDir, iTile, iMod);
			imgp[iTile][iMod][1] = IMG_LoadTexture (ascreen, sImage);
			/*** selected ***/
			snprintf (sDir, MAX_PATHFILE, "png%sspalace%s", SLASH, SLASH);
			snprintf (sImage, MAX_IMG, "%s%02x_%02x.png", sDir, iTile, iMod);
			imgp[iTile][iMod][2] = IMG_LoadTexture (ascreen, sImage);
			if ((!imgp[iTile][iMod][1]) || (!imgp[iTile][iMod][2]))
			{
				printf ("[FAILED] IMG_LoadTexture: %s!\n", IMG_GetError());
				exit (EXIT_ERROR);
			}
			break;
	}

	iPreLoaded+=2;
	iBarHeight = (int)(((float)iPreLoaded/(float)iNrToPreLoad) * BAR_FULL);
	if (iBarHeight >= iCurrentBarHeight + 10) { LoadingBar (iBarHeight); }
}
/*****************************************************************************/
void PreLoad (char *sPath, char *sPNG, SDL_Texture **imgImage)
/*****************************************************************************/
{
	char sImage[MAX_IMG + 2];
	int iBarHeight;

	snprintf (sImage, MAX_IMG, "png%s%s%s%s", SLASH, sPath, SLASH, sPNG);
	*imgImage = IMG_LoadTexture (ascreen, sImage);
	if (!*imgImage)
	{
		printf ("[FAILED] IMG_LoadTexture: %s!\n", IMG_GetError());
		exit (EXIT_ERROR);
	}

	iPreLoaded++;
	iBarHeight = (int)(((float)iPreLoaded/(float)iNrToPreLoad) * BAR_FULL);
	if (iBarHeight >= iCurrentBarHeight + 10) { LoadingBar (iBarHeight); }
}
/*****************************************************************************/
void ShowScreen (void)
/*****************************************************************************/
{
	int iTile;
	int iMod;
	int iLoc;
	int iHorL, iVerL;
	char sLevelBar[MAX_TEXT + 2];
	char sLevelBarF[MAX_TEXT + 2];
	int iUnusedRooms;
	int iX, iY;
	SDL_Texture *imgskel[2 + 2];
	SDL_Texture *imgfat[2 + 2];
	SDL_Texture *imgshadow[2 + 2];
	SDL_Texture *imgjaffar[2 + 2];
	SDL_Texture *imgguard[2 + 2];
	int iEventUnused;
	int iToRoom;
	int iEventRoom, iEventTile, iEventNext, iShowTile, iShowMod;
	char sText[MAX_TEXT + 2];
	int iHorSkel, iHorFat, iHorShadow, iHorJaffar, iHorGuard;

	/*** Used for looping. ***/
	int iTileLoop;
	int iRoomLoop;
	int iSideLoop;

	/*** black background ***/
	ShowImage (imgblack, 0, 0, "imgblack");

	if (iScreen == 1)
	{
		/*** One tile: 'top' row, room left down. ***/
		if (arRoomLinks[iCurLevel][iCurRoom][1] != 0) /*** left ***/
		{
			if (arRoomLinks[iCurLevel]
				[arRoomLinks[iCurLevel][iCurRoom][1]][4] != 0) /*** down ***/
			{
				GetTileMod (arRoomLinks[iCurLevel][arRoomLinks[iCurLevel]
					[iCurRoom][1]][4], 10, &iTile, &iMod);
			} else {
				iTile = 0x14;
				iMod = 0x01; /*** Yes, 0x01. Palace without wall pattern. ***/
			}
		} else {
			iTile = 0x14;
			iMod = 0x01; /*** Yes, 0x01. Palace without wall pattern. ***/
		}
		snprintf (sInfo, MAX_INFO, "tile=%02x_%02x", iTile, iMod);
		ShowImage (NULL, iHor[0], iVer4, sInfo);
		ShowImage (imgfadeds, iHor[0], iVer4, "imgfadeds");

		/*** One tile: bottom row, room left. ***/
		if (arRoomLinks[iCurLevel][iCurRoom][1] != 0)
		{
			GetTileMod (arRoomLinks[iCurLevel][iCurRoom][1], 30, &iTile, &iMod);
		} else {
			iTile = 0x14;
			iMod = 0x01; /*** Yes, 0x01. Palace without wall pattern. ***/
		}
		snprintf (sInfo, MAX_INFO, "tile=%02x_%02x", iTile, iMod);
		ShowImage (NULL, iHor[0], iVer3, sInfo);
		ShowImage (imgfadeds, iHor[0], iVer3, "imgfadeds");

		/*** One tile: middle row, room left. ***/
		if (arRoomLinks[iCurLevel][iCurRoom][1] != 0)
		{
			GetTileMod (arRoomLinks[iCurLevel][iCurRoom][1], 20, &iTile, &iMod);
		} else {
			iTile = 0x14;
			iMod = 0x01; /*** Yes, 0x01. Palace without wall pattern. ***/
		}
		snprintf (sInfo, MAX_INFO, "tile=%02x_%02x", iTile, iMod);
		ShowImage (NULL, iHor[0], iVer2, sInfo);
		ShowImage (imgfadeds, iHor[0], iVer2, "imgfadeds");

		/*** One tile: top row, room left. ***/
		if (arRoomLinks[iCurLevel][iCurRoom][1] != 0)
		{
			GetTileMod (arRoomLinks[iCurLevel][iCurRoom][1], 10, &iTile, &iMod);
		} else {
			iTile = 0x14;
			iMod = 0x01; /*** Yes, 0x01. Palace without wall pattern. ***/
		}
		snprintf (sInfo, MAX_INFO, "tile=%02x_%02x", iTile, iMod);
		ShowImage (NULL, iHor[0], iVer1, sInfo);
		ShowImage (imgfadeds, iHor[0], iVer1, "imgfadeds");

		/*** Under this room. ***/
		if (arRoomLinks[iCurLevel][iCurRoom][4] != 0)
		{
			for (iTileLoop = 1; iTileLoop <= (TILES / 3); iTileLoop++)
			{
				GetTileMod (arRoomLinks[iCurLevel][iCurRoom][4], iTileLoop,
					&iTile, &iMod);
				snprintf (sInfo, MAX_INFO, "tile=%02x_%02x", iTile, iMod);
				ShowImage (NULL, iHor[iTileLoop], iVer4, sInfo);
			}
		}

		/*** Inside the room. ***/
		for (iTileLoop = 1; iTileLoop <= 30; iTileLoop++)
		{
			iLoc = 0;
			iHorL = iHor[0]; /*** To prevent warnings. ***/
			iVerL = iVer0; /*** To prevent warnings. ***/
			if ((iTileLoop >= 1) && (iTileLoop <= 10))
			{
				iLoc = 20 + iTileLoop;
				iHorL = iHor[iTileLoop];
				iVerL = iVer3;
			}
			if ((iTileLoop >= 11) && (iTileLoop <= 20))
			{
				iLoc = iTileLoop;
				iHorL = iHor[iTileLoop - 10];
				iVerL = iVer2;
			}
			if ((iTileLoop >= 21) && (iTileLoop <= 30))
			{
				iLoc = -20 + iTileLoop;
				iHorL = iHor[iTileLoop - 20];
				iVerL = iVer1;
			}
			GetTileMod (iCurRoom, iLoc, &iTile, &iMod);
			snprintf (sInfo, MAX_INFO, "tile=%02x_%02x", iTile, iMod);
			ShowImage (NULL, iHorL, iVerL, sInfo);
			if (iLoc == iSelected)
			{
				snprintf (sInfo, MAX_INFO, "high=%02x_%02x", iTile, iMod);
				ShowImage (NULL, iHorL, iVerL, sInfo);
			}

			/*** prince ***/
			if ((iCurRoom == arStartLocation[iCurLevel][1]) &&
				(iLoc == arStartLocation[iCurLevel][2]))
			{
				switch (arStartLocation[iCurLevel][3])
				{
					case 0x00: /*** looks right ***/
						ShowImage (imgprincer[1], iHorL + 64, iVerL + 42, "imgprincer[1]");
						if (iSelected == iLoc)
							{ ShowImage (imgprincer[2], iHorL + 64,
							iVerL + 42, "imgprincer[2]"); }
						break;
					case 0xFF: /*** looks left ***/
						ShowImage (imgprincel[1], iHorL + 35, iVerL + 42, "imgprincel[1]");
						if (iSelected == iLoc)
							{ ShowImage (imgprincel[2], iHorL + 35,
							iVerL + 42, "imgprincel[2]"); }
						break;
					default:
						printf ("[ WARN ] Strange prince direction: 0x%02x!\n",
							arStartLocation[iCurLevel][3]);
				}
			}

			/*** guard ***/
			if (arGuardTile[iCurLevel][iCurRoom] == iLoc)
			{
				switch (arGuardDir[iCurLevel][iCurRoom])
				{
					case 0xFF: /*** l ***/
						imgskel[1] = imgskell[1];
						imgfat[1] = imgfatl[1];
						imgshadow[1] = imgshadowl[1];
						imgjaffar[1] = imgjaffarl[1];
						imgguard[1] = imgguardl[1];
						imgskel[2] = imgskell[2];
						imgfat[2] = imgfatl[2];
						imgshadow[2] = imgshadowl[2];
						imgjaffar[2] = imgjaffarl[2];
						imgguard[2] = imgguardl[2];
						iHorSkel = 28;
						iHorFat = 48;
						iHorShadow = 26; /*** Informed guess. ***/
						iHorJaffar = 48;
						iHorGuard = 48;
						break;
					case 0x00: /*** r ***/
						imgskel[1] = imgskelr[1];
						imgfat[1] = imgfatr[1];
						imgshadow[1] = imgshadowr[1];
						imgjaffar[1] = imgjaffarr[1];
						imgguard[1] = imgguardr[1];
						imgskel[2] = imgskelr[2];
						imgfat[2] = imgfatr[2];
						imgshadow[2] = imgshadowr[2];
						imgjaffar[2] = imgjaffarr[2];
						imgguard[2] = imgguardr[2];
						iHorSkel = 2;
						iHorFat = 6;
						iHorShadow = 6; /*** Guess. ***/
						iHorJaffar = 16;
						iHorGuard = 2;
						break;
					default:
						printf ("[FAILED] Incorrect guard direction: 0x%02x!\n",
							arGuardDir[iCurLevel][iCurRoom]);
						exit (EXIT_ERROR);
				}
				switch (iCurGuard)
				{
					case 0x01: /*** skeleton ***/
						ShowImage (imgskel[1], iHorL + iHorSkel,
							iVerL + 54, "imgskel[1]");
						if (iLoc == iSelected)
							{ ShowImage (imgskel[2], iHorL + iHorSkel,
							iVerL + 54, "imgskel[2]"); }
						break;
					case 0x03: /*** fat ***/
						ShowImage (imgfat[1], iHorL + iHorFat,
							iVerL + 42, "imgfat[1]");
						if (iLoc == iSelected)
							{ ShowImage (imgfat[2], iHorL + iHorFat,
							iVerL + 42, "imgfat[2]"); }
						break;
					case 0x04: /*** shadow ***/
						ShowImage (imgshadow[1], iHorL + iHorShadow,
							iVerL + 52, "imgshadow[1]"); /*** Informed guess. ***/
						if (iLoc == iSelected)
							{ ShowImage (imgshadow[2], iHorL + iHorShadow,
							iVerL + 52, "imgshadow[2]"); } /*** Informed guess. ***/
						break;
					case 0x05: /*** Jaffar ***/
						ShowImage (imgjaffar[1], iHorL + iHorJaffar,
							iVerL + 34, "imgjaffar[1]");
						if (iLoc == iSelected)
							{ ShowImage (imgjaffar[2], iHorL + iHorJaffar,
							iVerL + 34, "imgjaffar[2]"); }
						break;
					default: /*** guard; 0x00 and 0x02 ***/
						ShowImage (imgguard[1], iHorL + iHorGuard,
							iVerL + 42, "imgguard[1]");
						if (iLoc == iSelected)
							{ ShowImage (imgguard[2], iHorL + iHorGuard,
							iVerL + 42, "imgguard[2]"); }
						break;
				}
				switch (arGuardSkill[iCurLevel][iCurRoom])
				{
					case 10:
						snprintf (sText, MAX_TEXT, "%s", "T:a"); break;
					case 11:
						snprintf (sText, MAX_TEXT, "%s", "T:b"); break;
					default:
						snprintf (sText, MAX_TEXT, "T:%i",
							arGuardSkill[iCurLevel][iCurRoom]); break;
				}
				message = TTF_RenderText_Shaded (font2, sText, color_bl, color_wh);
				messaget = SDL_CreateTextureFromSurface (ascreen, message);
				offset.x = iHorL;
				offset.y = iVerL + 125;
				offset.w = message->w; offset.h = message->h;
				CustomRenderCopy (messaget, NULL, &offset, "message");
				SDL_DestroyTexture (messaget); SDL_FreeSurface (message);
			}
		}

		/*** Above this room. ***/
		for (iTileLoop = 1; iTileLoop <= (TILES / 3); iTileLoop++)
		{
			if (arRoomLinks[iCurLevel][iCurRoom][3] != 0)
			{
				GetTileMod (arRoomLinks[iCurLevel][iCurRoom][3], iTileLoop + 20,
					&iTile, &iMod);
			} else {
				iTile = 0x01; iMod = 0x00; /*** Floor. ***/
			}
			snprintf (sInfo, MAX_INFO, "tile=%02x_%02x", iTile, iMod);
			ShowImage (NULL, iHor[iTileLoop], iVer0, sInfo);
		}
	}
	if (iScreen == 2) /*** R ***/
	{
		if (arBrokenRoomLinks[iCurLevel] == 0)
		{
			InitRooms();
			/*** room links ***/
			ShowImage (imgrl, 25, 50, "imgrl");
			/*** rooms broken on ***/
			if (iDownAt == 11)
			{
				ShowImage (imgbroomson_1, 546, 66, "imgbroomson_1"); /*** down ***/
			} else {
				ShowImage (imgbroomson_0, 546, 66, "imgbroomson_0"); /*** up ***/
			}
			WhereToStart();
			for (iRoomLoop = 1; iRoomLoop <= ROOMS; iRoomLoop++)
			{
				arDone[iRoomLoop] = 0;
			}
			ShowRooms (arStartLocation[iCurLevel][1], iStartRoomsX, iStartRoomsY, 1);
			iUnusedRooms = 0;
			for (iRoomLoop = 1; iRoomLoop <= ROOMS; iRoomLoop++)
			{
				if (arDone[iRoomLoop] != 1)
				{
					iUnusedRooms++;
					ShowRooms (iRoomLoop, 25, iUnusedRooms, 0);
				}
			}
			if (iMovingRoom != 0)
			{
				iX = (iXPos + 10) / iScale;
				iY = (iYPos + 10) / iScale;
				ShowImage (imgroom[iMovingRoom], iX, iY, "imgroom[...]");
				if (iCurRoom == iMovingRoom)
				{
					ShowImage (imgsrc, iX, iY, "imgsrc"); /*** green stripes ***/
				}
				if (arStartLocation[iCurLevel][1] == iMovingRoom)
				{
					ShowImage (imgsrs, iX, iY, "imgsrs"); /*** blue border ***/
				}
				ShowImage (imgsrm, iX, iY, "imgsrm"); /*** red stripes ***/
				ShowRooms (-1, iMovingNewX, iMovingNewY, 0);
			}
		} else {
			/*** broken room links ***/
			ShowImage (imgbrl, 25, 50, "imgbrl");
			for (iRoomLoop = 1; iRoomLoop <= ROOMS; iRoomLoop++)
			{
				/*** green stripes ***/
				if (iCurRoom == iRoomLoop)
				{
					BrokenRoomChange (iRoomLoop, 0, &iX, &iY);
					ShowImage (imgsrc, iX, iY, "imgsrc");
				}

				/*** blue border ***/
				if (arStartLocation[iCurLevel][1] == iRoomLoop)
				{
					BrokenRoomChange (iRoomLoop, 0, &iX, &iY);
					ShowImage (imgsrs, iX, iY, "imgsrs");
				}

				for (iSideLoop = 1; iSideLoop <= 4; iSideLoop++)
				{
					if (arRoomLinks[iCurLevel][iRoomLoop][iSideLoop] != 0)
					{
						BrokenRoomChange (iRoomLoop, iSideLoop, &iX, &iY);
						iToRoom = arRoomLinks[iCurLevel][iRoomLoop][iSideLoop];
						ShowImage (imgroom[iToRoom], iX, iY, "imgroom[...]");

						/*** blue square ***/
						if (arRoomConnectionsBroken[iCurLevel][iRoomLoop][iSideLoop] == 1)
						{
							BrokenRoomChange (iRoomLoop, iSideLoop, &iX, &iY);
							ShowImage (imgsrb, iX, iY, "imgsrb");
						}
					}

					/*** red stripes ***/
					if ((iChangingBrokenRoom == iRoomLoop) &&
						(iChangingBrokenSide == iSideLoop))
					{
						BrokenRoomChange (iRoomLoop, iSideLoop, &iX, &iY);
						ShowImage (imgsrm, iX, iY, "imgsrm");
					}
				}
			}
		}
	}
	if (iScreen == 3) /*** E ***/
	{
		iEventRoom = arEventsRoom[iCurLevel][iChangeEvent];
		iEventTile = arEventsTile[iCurLevel][iChangeEvent];
		iEventNext = arEventsNext[iCurLevel][iChangeEvent];
		GetTileMod (iEventRoom, iEventTile, &iShowTile, &iShowMod);

		iEventUnused = 0;

		/*** background ***/
		ShowImage (imgevents, 25, 50, "imgevents");

		/*** edit this event ***/
		CenterNumber (iChangeEvent, 291, 60, color_wh, 0);

		/*** sel event, room ***/
		if ((iEventRoom >= 1) && (iEventRoom <= 24))
		{
			ShowImage (imgsele, 217 + (15 * (iEventRoom - 1)), 115, "imgsele");
		} else { iEventUnused = 1; }

		/*** sel event, tile ***/
		if ((iEventTile >= 1) && (iEventTile <= 30))
		{
			if ((iEventTile >= 1) && (iEventTile <= 10))
				{ ShowImage (imgsele, 367 + (iEventTile * 15), 155, "imgsele"); }
			if ((iEventTile >= 11) && (iEventTile <= 20))
				{ ShowImage (imgsele, 367 + ((iEventTile - 10) * 15), 170, "imgsele"); }
			if ((iEventTile >= 21) && (iEventTile <= 30))
				{ ShowImage (imgsele, 367 + ((iEventTile - 20) * 15), 185, "imgsele"); }
		} else { iEventUnused = 1; }

		/*** sel event, next ***/
		switch (iEventNext)
		{
			case 0: ShowImage (imgsele, 502, 225, "imgsele"); break; /*** N ***/
			case 1: ShowImage (imgsele, 517, 225, "imgsele"); break; /*** Y ***/
		}

		/*** Show target. ***/
		if (iEventUnused == 0)
		{
			snprintf (sInfo, MAX_INFO, "tile=%02x_%02x", iShowTile, iShowMod);
			ShowImage (NULL, 445, 264, sInfo);
		} else {
			ShowImage (imgeventu, 445, 264, "imgeventu");
		}

		if (iMouse == 1) { ShowImage (imgmouse, 538, 73, "imgmouse"); }
	}

	/*** left ***/
	if (arRoomLinks[iCurLevel][iCurRoom][1] != 0)
	{
		/*** yes ***/
		if (iDownAt == 1)
		{
			ShowImage (imgleft_1, 0, 50, "imgleft_1"); /*** down ***/
		} else {
			ShowImage (imgleft_0, 0, 50, "imgleft_0"); /*** up ***/
		}
	} else {
		/*** no ***/
		ShowImage (imglrno, 0, 50, "imglrno");
	}

	/*** right ***/
	if (arRoomLinks[iCurLevel][iCurRoom][2] != 0)
	{
		/*** yes ***/
		if (iDownAt == 2)
		{
			ShowImage (imgright_1, 587, 50, "imgright_1"); /*** down ***/
		} else {
			ShowImage (imgright_0, 587, 50, "imgright_0"); /*** up ***/
		}
	} else {
		/*** no ***/
		ShowImage (imglrno, 587, 50, "imglrno");
	}

	/*** up ***/
	if (arRoomLinks[iCurLevel][iCurRoom][3] != 0)
	{
		/*** yes ***/
		if (iDownAt == 3)
		{
			ShowImage (imgup_1, 25, 25, "imgup_1"); /*** down ***/
		} else {
			ShowImage (imgup_0, 25, 25, "imgup_0"); /*** up ***/
		}
	} else {
		/*** no ***/
		if (iScreen != 1)
		{
			ShowImage (imgudno, 25, 25, "imgudno"); /*** without info ***/
		} else {
			ShowImage (imgudnonfo, 25, 25, "imgudnonfo"); /*** with info ***/
		}
	}

	/*** down ***/
	if (arRoomLinks[iCurLevel][iCurRoom][4] != 0)
	{
		/*** yes ***/
		if (iDownAt == 4)
		{
			ShowImage (imgdown_1, 25, 436, "imgdown_1"); /*** down ***/
		} else {
			ShowImage (imgdown_0, 25, 436, "imgdown_0"); /*** up ***/
		}
	} else {
		/*** no ***/
		ShowImage (imgudno, 25, 436, "imgudno");
	}

	switch (iScreen)
	{
		case 1:
			if (arBrokenRoomLinks[iCurLevel] == 1)
			{
				/*** rooms broken on ***/
				if (iDownAt == 11)
				{
					ShowImage (imgbroomson_1, 0, 25, "imgbroomson_1"); /*** down ***/
				} else {
					ShowImage (imgbroomson_0, 0, 25, "imgbroomson_0"); /*** up ***/
				}
			} else {
				/*** rooms on ***/
				if (iDownAt == 5)
				{
					ShowImage (imgroomson_1, 0, 25, "imgroomson_1"); /*** down ***/
				} else {
					ShowImage (imgroomson_0, 0, 25, "imgroomson_0"); /*** up ***/
				}
			}
			/*** events on ***/
			if (iDownAt == 6)
			{
				ShowImage (imgeventson_1, 587, 25, "imgeventson_1"); /*** down ***/
			} else {
				ShowImage (imgeventson_0, 587, 25, "imgeventson_0"); /*** up ***/
			}
			break;
		case 2:
			/*** rooms off ***/
			if (arBrokenRoomLinks[iCurLevel] == 1)
			{
				ShowImage (imgbroomsoff, 0, 25, "imgbroomsoff"); /*** broken ***/
			} else {
				ShowImage (imgroomsoff, 0, 25, "imgroomsoff");
			}

			/*** events on ***/
			if (iDownAt == 6)
			{
				ShowImage (imgeventson_1, 587, 25, "imgeventson_1"); /*** down ***/
			} else {
				ShowImage (imgeventson_0, 587, 25, "imgeventson_0"); /*** up ***/
			}
			break;
		case 3:
			if (arBrokenRoomLinks[iCurLevel] == 1)
			{
				/*** rooms broken on ***/
				if (iDownAt == 11)
				{
					ShowImage (imgbroomson_1, 0, 25, "imgbroomson_1"); /*** down ***/
				} else {
					ShowImage (imgbroomson_0, 0, 25, "imgbroomson_0"); /*** up ***/
				}
			} else {
				/*** rooms on ***/
				if (iDownAt == 5)
				{
					ShowImage (imgroomson_1, 0, 25, "imgroomson_1"); /*** down ***/
				} else {
					ShowImage (imgroomson_0, 0, 25, "imgroomson_0"); /*** up ***/
				}
			}
			/*** events off ***/
			ShowImage (imgeventsoff, 587, 25, "imgeventsoff");
			break;
	}

	/*** save ***/
	if (iChanged != 0)
	{
		/*** on ***/
		if (iDownAt == 7)
		{
			ShowImage (imgsaveon_1, 0, 436, "imgsaveon_1"); /*** down ***/
		} else {
			ShowImage (imgsaveon_0, 0, 436, "imgsaveon_0"); /*** up ***/
		}
	} else {
		/*** off ***/
		ShowImage (imgsaveoff, 0, 436, "imgsaveoff");
	}

	/*** quit ***/
	if (iDownAt == 8)
	{
		ShowImage (imgquit_1, 587, 436, "imgquit_1"); /*** down ***/
	} else {
		ShowImage (imgquit_0, 587, 436, "imgquit_0"); /*** up ***/
	}

	/*** previous ***/
	if (iCurLevel != 1)
	{
		/*** on ***/
		if (iDownAt == 9)
		{
			ShowImage (imgprevon_1, 0, 0, "imgprevon_1"); /*** down ***/
		} else {
			ShowImage (imgprevon_0, 0, 0, "imgprevon_0"); /*** up ***/
		}
	} else {
		/*** off ***/
		ShowImage (imgprevoff, 0, 0, "imgprevoff");
	}

	/*** next ***/
	if (iCurLevel != LEVELS)
	{
		/*** on ***/
		if (iDownAt == 10)
		{
			ShowImage (imgnexton_1, 587, 0, "imgnexton_1"); /*** down ***/
		} else {
			ShowImage (imgnexton_0, 587, 0, "imgnexton_0"); /*** up ***/
		}
	} else {
		/*** off ***/
		ShowImage (imgnextoff, 587, 0, "imgnextoff");
	}

	/*** level bar ***/
	switch (iHomeComputer)
	{
		case 1: ShowImage (imgbara, 25, 0, "imgbara"); break;
		case 2: ShowImage (imgbarb, 25, 0, "imgbarb"); break;
		case 3: ShowImage (imgbarc, 25, 0, "imgbarc"); break;
	}

	/*** Assemble level bar text. ***/
	switch (iCurLevel)
	{
		case 1: snprintf (sLevelBar, MAX_TEXT, "level 1 (prison),"); break;
		case 2: snprintf (sLevelBar, MAX_TEXT, "level 2 (guards),"); break;
		case 3: snprintf (sLevelBar, MAX_TEXT, "level 3 (skeleton),"); break;
		case 4: snprintf (sLevelBar, MAX_TEXT, "level 4 (mirror),"); break;
		case 5: snprintf (sLevelBar, MAX_TEXT, "level 5 (thief),"); break;
		case 6: snprintf (sLevelBar, MAX_TEXT, "level 6 (plunge),"); break;
		case 7: snprintf (sLevelBar, MAX_TEXT, "level 7 (weightless),"); break;
		case 8: snprintf (sLevelBar, MAX_TEXT, "level 8 (mouse),"); break;
		case 9: snprintf (sLevelBar, MAX_TEXT, "level 9 (twisty),"); break;
		case 10: snprintf (sLevelBar, MAX_TEXT, "level 10 (quad),"); break;
		case 11: snprintf (sLevelBar, MAX_TEXT, "level 11 (fragile),"); break;
		case 12: snprintf (sLevelBar, MAX_TEXT, "level 12 (12a; tower),"); break;
		case 13: snprintf (sLevelBar, MAX_TEXT, "level 13 (12b; jaffar),"); break;
		case 14: snprintf (sLevelBar, MAX_TEXT, "level 14 (rescue),"); break;
		case 15: snprintf (sLevelBar, MAX_TEXT, "level 0 (demo),"); break;
	}
	switch (iScreen)
	{
		case 1:
			snprintf (sLevelBarF, MAX_TEXT, "%s room %i", sLevelBar, iCurRoom);
			ShowImage (imgextras[iExtras], 530, 3, "imgextras[...]");
			break;
		case 2:
			snprintf (sLevelBarF, MAX_TEXT, "%s room links", sLevelBar); break;
		case 3:
			snprintf (sLevelBarF, MAX_TEXT, "%s events", sLevelBar); break;
	}

	/*** Emulator information. ***/
	if (iEmulator == 1) { ShowImage (imgemulator, 25, 50, "imgemulator"); }

	/*** Home computer icon on level bar. ***/
	switch (iHomeComputer)
	{
		case 1: ShowImage (imghcalb, 32, 7, "imghcalb"); break;
		case 2: ShowImage (imghcblb, 32, 7, "imghcblb"); break;
		case 3: ShowImage (imghcclb, 32, 7, "imghcclb"); break;
	}

	/*** Display level bar text. ***/
	message = TTF_RenderText_Shaded (font1, sLevelBarF, color_bl, color_wh);
	messaget = SDL_CreateTextureFromSurface (ascreen, message);
	offset.x = 48;
	offset.y = 4;
	offset.w = message->w; offset.h = message->h;
	CustomRenderCopy (messaget, NULL, &offset, "message");
	SDL_DestroyTexture (messaget); SDL_FreeSurface (message);

	/*** refresh screen ***/
	SDL_RenderPresent (ascreen);
}
/*****************************************************************************/
void InitPopUp (void)
/*****************************************************************************/
{
	int iPopUp;
	SDL_Event event;

	iPopUp = 1;

	PlaySound ("wav/popup.wav");
	ShowPopUp();
	while (iPopUp == 1)
	{
		while (SDL_PollEvent (&event))
		{
			switch (event.type)
			{
				case SDL_CONTROLLERBUTTONDOWN:
					/*** Nothing for now. ***/
					break;
				case SDL_CONTROLLERBUTTONUP:
					switch (event.cbutton.button)
					{
						case SDL_CONTROLLER_BUTTON_A:
							iPopUp = 0; break;
					}
					break;
				case SDL_KEYDOWN:
					switch (event.key.keysym.sym)
					{
						case SDLK_ESCAPE:
						case SDLK_KP_ENTER:
						case SDLK_RETURN:
						case SDLK_SPACE:
						case SDLK_o:
							iPopUp = 0;
						default: break;
					}
					break;
				case SDL_MOUSEMOTION:
					iXPos = event.motion.x;
					iYPos = event.motion.y;
					break;
				case SDL_MOUSEBUTTONDOWN:
					if (event.button.button == 1)
					{
						if (InArea (400, 320, 400 + 85, 320 + 32) == 1) /*** OK ***/
						{
							iOKOn = 1;
							ShowPopUp();
						}
					}
					break;
				case SDL_MOUSEBUTTONUP:
					iOKOn = 0;
					if (event.button.button == 1)
					{
						if (InArea (400, 320, 400 + 85, 320 + 32) == 1) /*** OK ***/
						{
							iPopUp = 0;
						}
					}
					ShowPopUp(); break;
				case SDL_WINDOWEVENT:
					if (event.window.event == SDL_WINDOWEVENT_EXPOSED)
						{ ShowScreen(); ShowPopUp(); } break;
				case SDL_QUIT:
					Quit(); break;
			}
		}

		/*** prevent CPU eating ***/
		gamespeed = REFRESH_PROG;
		while ((SDL_GetTicks() - looptime) < gamespeed)
		{
			SDL_Delay (10);
		}
		looptime = SDL_GetTicks();
	}
	PlaySound ("wav/popup_close.wav");
	ShowScreen();
}
/*****************************************************************************/
void ShowPopUp (void)
/*****************************************************************************/
{
	char arText[9 + 2][MAX_TEXT + 2];

	/*** faded background ***/
	ShowImage (imgfadedl, 0, 0, "imgfadedl");

	/*** popup ***/
	ShowImage (imgpopup, 61, 7, "imgpopup");

	/*** OK ***/
	switch (iOKOn)
	{
		case 0: ShowImage (imgok[1], 400, 320, "imgok[1]"); break; /*** off ***/
		case 1: ShowImage (imgok[2], 400, 320, "imgok[2]"); break; /*** on ***/
	}

	snprintf (arText[0], MAX_TEXT, "%s %s", EDITOR_NAME, EDITOR_VERSION);
	snprintf (arText[1], MAX_TEXT, "%s", COPYRIGHT);
	snprintf (arText[2], MAX_TEXT, "%s", "");
	if (iController != 1)
	{
		snprintf (arText[3], MAX_TEXT, "%s", "single tile (change or select)");
		snprintf (arText[4], MAX_TEXT, "%s", "entire room (clear or fill)");
		snprintf (arText[5], MAX_TEXT, "%s", "entire level (randomize or fill)");
	} else {
		snprintf (arText[3], MAX_TEXT, "%s", "The detected game controller:");
		snprintf (arText[4], MAX_TEXT, "%s", sControllerName);
		snprintf (arText[5], MAX_TEXT, "%s", "Have fun using it!");
	}
	snprintf (arText[6], MAX_TEXT, "%s", "");
	snprintf (arText[7], MAX_TEXT, "%s", "You may use one guard per room.");
	snprintf (arText[8], MAX_TEXT, "%s", "The tile behavior may differ per"
		" level.");

	DisplayText (140, 121, FONT_SIZE_15, arText, 9, font1);

	/*** refresh screen ***/
	SDL_RenderPresent (ascreen);
}
/*****************************************************************************/
void Help (void)
/*****************************************************************************/
{
	int iHelp;
	SDL_Event event;

	iHelp = 1;

	PlaySound ("wav/popup.wav");
	ShowHelp();
	while (iHelp == 1)
	{
		while (SDL_PollEvent (&event))
		{
			switch (event.type)
			{
				case SDL_CONTROLLERBUTTONDOWN:
					/*** Nothing for now. ***/
					break;
				case SDL_CONTROLLERBUTTONUP:
					switch (event.cbutton.button)
					{
						case SDL_CONTROLLER_BUTTON_A:
							iHelp = 0; break;
					}
					break;
				case SDL_KEYDOWN:
					switch (event.key.keysym.sym)
					{
						case SDLK_ESCAPE:
						case SDLK_KP_ENTER:
						case SDLK_RETURN:
						case SDLK_SPACE:
						case SDLK_o:
							iHelp = 0;
						default: break;
					}
					break;
				case SDL_MOUSEMOTION:
					iXPos = event.motion.x;
					iYPos = event.motion.y;
					if (InArea (48, 322, 48 + 517, 322 + 20) == 1)
					{
						SDL_SetCursor (curHand);
					} else {
						SDL_SetCursor (curArrow);
					}
					break;
				case SDL_MOUSEBUTTONDOWN:
					if (event.button.button == 1)
					{
						if (InArea (510, 411, 510 + 85, 411 + 32) == 1) /*** OK ***/
						{
							iHelpOK = 1;
							ShowHelp();
						}
					}
					break;
				case SDL_MOUSEBUTTONUP:
					iHelpOK = 0;
					if (event.button.button == 1) /*** left mouse button ***/
					{
						if (InArea (510, 411, 510 + 85, 411 + 32) == 1) /*** OK ***/
							{ iHelp = 0; }
						if (InArea (48, 322, 48 + 517, 322 + 20) == 1)
							{ OpenURL ("https://github.com/EndeavourAccuracy/leapop"); }
					}
					ShowHelp(); break;
				case SDL_WINDOWEVENT:
					if (event.window.event == SDL_WINDOWEVENT_EXPOSED)
						{ ShowHelp(); } break;
				case SDL_QUIT:
					Quit(); break;
			}
		}

		/*** prevent CPU eating ***/
		gamespeed = REFRESH_PROG;
		while ((SDL_GetTicks() - looptime) < gamespeed)
		{
			SDL_Delay (10);
		}
		looptime = SDL_GetTicks();
	}
	PlaySound ("wav/popup_close.wav");
	ShowScreen();
}
/*****************************************************************************/
void ShowHelp (void)
/*****************************************************************************/
{
	/*** background ***/
	ShowImage (imghelp, 0, 0, "imghelp");

	/*** OK ***/
	switch (iHelpOK)
	{
		case 0: ShowImage (imgok[1], 510, 411, "imgok[1]"); break; /*** off ***/
		case 1: ShowImage (imgok[2], 510, 411, "imgok[2]"); break; /*** on ***/
	}

	/*** refresh screen ***/
	SDL_RenderPresent (ascreen);
}
/*****************************************************************************/
void EXE (void)
/*****************************************************************************/
{
	int iEXE;
	SDL_Event event;
	int iOldXPos, iOldYPos;
	int iRow;
	int iUseL, iUseR;
	int iTabMax;
	int iMin;

	iEXE = 1;
	iEXETabS = iCurLevel;
	if (iEXETabS == 15) { iEXETabS = 0; }

	EXELoad();

	PlaySound ("wav/popup.wav");
	ShowEXE();
	while (iEXE == 1)
	{
		while (SDL_PollEvent (&event))
		{
			switch (event.type)
			{
				case SDL_CONTROLLERBUTTONDOWN:
					/*** Nothing for now. ***/
					break;
				case SDL_CONTROLLERBUTTONUP:
					switch (event.cbutton.button)
					{
						case SDL_CONTROLLER_BUTTON_A:
							EXESave(); iEXE = 0; break;
					}
					break;
				case SDL_KEYDOWN:
					switch (event.key.keysym.sym)
					{
						case SDLK_ESCAPE:
							iEXE = 0; break;
						case SDLK_KP_ENTER:
						case SDLK_RETURN:
						case SDLK_SPACE:
						case SDLK_s:
							EXESave(); iEXE = 0;
							break;
						default: break;
					}
					ShowEXE();
					break;
				case SDL_MOUSEMOTION:
					iOldXPos = iXPos;
					iOldYPos = iYPos;
					iXPos = event.motion.x;
					iYPos = event.motion.y;
					if ((iOldXPos == iXPos) && (iOldYPos == iYPos)) { break; }

					ShowEXE();
					break;
				case SDL_MOUSEBUTTONDOWN:
					if (event.button.button == 1)
					{
						if (InArea (510, 411, 510 + 85, 411 + 32) == 1) /*** Save ***/
						{
							iEXESave = 1;
							ShowEXE();
						}
					}
					break;
				case SDL_MOUSEBUTTONUP:
					iEXESave = 0;
					if (event.button.button == 1) /*** left mouse button ***/
					{
						if (InArea (510, 411, 510 + 85, 411 + 32) == 1) /*** Save ***/
						{
							EXESave(); iEXE = 0;
						}

						/*******************/
						/* global settings */
						/*******************/

						/*** Prince HP. ***/
						if (((iHomeComputer == 1) && (ulPrinceHPA[iDiskImageA] != 0x00)) ||
							((iHomeComputer == 2) && (ulPrinceHPB[iDiskImageB] != 0x00)) ||
							((iHomeComputer == 3) && (ulPrinceHPC[iDiskImageC] != 0x00)))
						{
							PlusMinus (&iEXEPrinceHP, 139, 58, 0, 255, -10, 0);
							PlusMinus (&iEXEPrinceHP, 154, 58, 0, 255, -1, 0);
							PlusMinus (&iEXEPrinceHP, 224, 58, 0, 255, +1, 0);
							PlusMinus (&iEXEPrinceHP, 239, 58, 0, 255, +10, 0);
						}

						/*** Shadow HP. ***/
						if (((iHomeComputer == 1) && (ulShadowHPA[iDiskImageA] != 0x00)) ||
							((iHomeComputer == 2) && (ulShadowHPB[iDiskImageB] != 0x00)) ||
							((iHomeComputer == 3) && (ulShadowHPC[iDiskImageC] != 0x00)))
						{
							PlusMinus (&iEXEShadowHP, 139, 82, 0, 255, -10, 0);
							PlusMinus (&iEXEShadowHP, 154, 82, 0, 255, -1, 0);
							PlusMinus (&iEXEShadowHP, 224, 82, 0, 255, +1, 0);
							PlusMinus (&iEXEShadowHP, 239, 82, 0, 255, +10, 0);
						}

						/*** Chomper delay. ***/
						if (((iHomeComputer == 1) &&
							(ulChomperDelayA[iDiskImageA] != 0x00)) ||
							((iHomeComputer == 2) &&
							(ulChomperDelayB[iDiskImageB] != 0x00)) ||
							((iHomeComputer == 3) &&
							(ulChomperDelayC[iDiskImageC] != 0x00)))
						{
							PlusMinus (&iEXEChomperDelay, 139, 106, 3, 255, -10, 0);
							PlusMinus (&iEXEChomperDelay, 154, 106, 3, 255, -1, 0);
							PlusMinus (&iEXEChomperDelay, 224, 106, 3, 255, +1, 0);
							PlusMinus (&iEXEChomperDelay, 239, 106, 3, 255, +10, 0);
						}

						/*** Mouse delay. ***/
						if (((iHomeComputer == 1) &&
							(ulMouseDelayA[iDiskImageA] != 0x00)) ||
							((iHomeComputer == 2) &&
							(ulMouseDelayB[iDiskImageB] != 0x00)) ||
							((iHomeComputer == 3) &&
							(ulMouseDelayC[iDiskImageC] != 0x00)))
						{
							PlusMinus (&iEXEMouseDelay, 139, 130, 0, 255, -10, 0);
							PlusMinus (&iEXEMouseDelay, 154, 130, 0, 255, -1, 0);
							PlusMinus (&iEXEMouseDelay, 224, 130, 0, 255, +1, 0);
							PlusMinus (&iEXEMouseDelay, 239, 130, 0, 255, +10, 0);
						}

						/******************/
						/* guard settings */
						/******************/

						/*** strike prob. ***/
						if ((InArea (281, 188, 281 + 94, 188 + 19) == 1) && (iEXETab != 1))
							{ iEXETab = 1; PlaySound ("wav/extras.wav"); }
						/*** re-strike prob. ***/
						if ((InArea (383, 188, 383 + 94, 188 + 19) == 1) && (iEXETab != 2))
							{ iEXETab = 2; PlaySound ("wav/extras.wav"); }
						/*** block prob. ***/
						if ((InArea (485, 188, 485 + 94, 188 + 19) == 1) && (iEXETab != 3))
							{ iEXETab = 3; PlaySound ("wav/extras.wav"); }
						/*** imp. block prob. ***/
						if ((InArea (281, 215, 281 + 94, 215 + 19) == 1) && (iEXETab != 4))
							{ iEXETab = 4; PlaySound ("wav/extras.wav"); }
						/*** advance prob. ***/
						if ((InArea (383, 215, 383 + 94, 215 + 19) == 1) && (iEXETab != 5))
							{ iEXETab = 5; PlaySound ("wav/extras.wav"); }
						/*** refractory timer ***/
						if ((InArea (485, 215, 485 + 94, 215 + 19) == 1) && (iEXETab != 6))
							{ iEXETab = 6; PlaySound ("wav/extras.wav"); }
						/*** special color ***/
						if ((InArea (281, 242, 281 + 94, 242 + 19) == 1) && (iEXETab != 7))
							{ iEXETab = 7; PlaySound ("wav/extras.wav"); }
						/*** extra strength ***/
						if ((InArea (383, 242, 383 + 94, 242 + 19) == 1) && (iEXETab != 8))
							{ iEXETab = 8; PlaySound ("wav/extras.wav"); }

						if (((iHomeComputer == 1) &&
							(ulGuardA[iDiskImageA][iEXETab - 1] != 0x00)) ||
							((iHomeComputer == 2) &&
							(ulGuardB[iDiskImageB][iEXETab - 1] != 0x00)) ||
							((iHomeComputer == 3) &&
							(ulGuardC[iDiskImageC][iEXETab - 1] != 0x00)))
						{
							for (iRow = 1; iRow <= 6; iRow++)
							{
								iUseL = (iRow * 2) - 1;
								iUseR = iUseL + 1;
								switch (iEXETab)
								{
									case 6: iTabMax = 20; break;
									case 7: iTabMax = 1; break;
									case 8: iTabMax = 1; break;
									default: iTabMax = 255; break;
								}

								PlusMinus (&iEXEGuard[iEXETab][iUseL], 310,
									10 + (iRow * 24), 0, iTabMax, -10, 0);
								PlusMinus (&iEXEGuard[iEXETab][iUseL], 325,
									10 + (iRow * 24), 0, iTabMax, -1, 0);
								PlusMinus (&iEXEGuard[iEXETab][iUseL], 395,
									10 + (iRow * 24), 0, iTabMax, +1, 0);
								PlusMinus (&iEXEGuard[iEXETab][iUseL], 410,
									10 + (iRow * 24), 0, iTabMax, +10, 0);

								PlusMinus (&iEXEGuard[iEXETab][iUseR], 466,
									10 + (iRow * 24), 0, iTabMax, -10, 0);
								PlusMinus (&iEXEGuard[iEXETab][iUseR], 481,
									10 + (iRow * 24), 0, iTabMax, -1, 0);
								PlusMinus (&iEXEGuard[iEXETab][iUseR], 551,
									10 + (iRow * 24), 0, iTabMax, +1, 0);
								PlusMinus (&iEXEGuard[iEXETab][iUseR], 566,
									10 + (iRow * 24), 0, iTabMax, +10, 0);
							}
						}

						/**********************/
						/* per level settings */
						/**********************/

						/*** demo ***/
						if ((InArea (267, 299, 267 + 56, 299 + 19) == 1) && (iEXETabS
							!= 0)) { iEXETabS = 0; PlaySound ("wav/extras.wav"); }
						/*** level 1 ***/
						if ((InArea (331, 299, 331 + 56, 299 + 19) == 1) && (iEXETabS
							!= 1)) { iEXETabS = 1; PlaySound ("wav/extras.wav"); }
						/*** level 2 ***/
						if ((InArea (395, 299, 395 + 56, 299 + 19) == 1) && (iEXETabS
							!= 2)) { iEXETabS = 2; PlaySound ("wav/extras.wav"); }
						/*** level 3 ***/
						if ((InArea (459, 299, 459 + 56, 299 + 19) == 1) && (iEXETabS
							!= 3)) { iEXETabS = 3; PlaySound ("wav/extras.wav"); }
						/*** level 4 ***/
						if ((InArea (523, 299, 523 + 56, 299 + 19) == 1) && (iEXETabS
							!= 4)) { iEXETabS = 4; PlaySound ("wav/extras.wav"); }
						/*** level 5 ***/
						if ((InArea (267, 326, 267 + 56, 326 + 19) == 1) && (iEXETabS
							!= 5)) { iEXETabS = 5; PlaySound ("wav/extras.wav"); }
						/*** level 6 ***/
						if ((InArea (331, 326, 331 + 56, 326 + 19) == 1) && (iEXETabS
							!= 6)) { iEXETabS = 6; PlaySound ("wav/extras.wav"); }
						/*** level 7 ***/
						if ((InArea (395, 326, 395 + 56, 326 + 19) == 1) && (iEXETabS
							!= 7)) { iEXETabS = 7; PlaySound ("wav/extras.wav"); }
						/*** level 8 ***/
						if ((InArea (459, 326, 459 + 56, 326 + 19) == 1) && (iEXETabS
							!= 8)) { iEXETabS = 8; PlaySound ("wav/extras.wav"); }
						/*** level 9 ***/
						if ((InArea (523, 326, 523 + 56, 326 + 19) == 1) && (iEXETabS
							!= 9)) { iEXETabS = 9; PlaySound ("wav/extras.wav"); }
						/*** level 10 ***/
						if ((InArea (267, 353, 267 + 56, 353 + 19) == 1) && (iEXETabS
							!= 10)) { iEXETabS = 10; PlaySound ("wav/extras.wav"); }
						/*** level 11 ***/
						if ((InArea (331, 353, 331 + 56, 353 + 19) == 1) && (iEXETabS
							!= 11)) { iEXETabS = 11; PlaySound ("wav/extras.wav"); }
						/*** level 12 ***/
						if ((InArea (395, 353, 395 + 56, 353 + 19) == 1) && (iEXETabS
							!= 12)) { iEXETabS = 12; PlaySound ("wav/extras.wav"); }
						/*** level 13 ***/
						if ((InArea (459, 353, 459 + 56, 353 + 19) == 1) && (iEXETabS
							!= 13)) { iEXETabS = 13; PlaySound ("wav/extras.wav"); }
						/*** level 14 ***/
						if ((InArea (523, 353, 523 + 56, 353 + 19) == 1) && (iEXETabS
							!= 14)) { iEXETabS = 14; PlaySound ("wav/extras.wav"); }

						if (iEXETabS != 14) /*** Level 14 has no guards. ***/
						{
							/*** Guard HP. ***/
							if (((iHomeComputer == 1) &&
								(ulGuardHPA[iDiskImageA] != 0x00)) ||
								((iHomeComputer == 2) &&
								(ulGuardHPB[iDiskImageB] != 0x00)) ||
								((iHomeComputer == 3) &&
								(ulGuardHPC[iDiskImageC] != 0x00)))
							{
								PlusMinus (&iEXEGuardHP[iEXETabS], 139, 236, 0, 255, -10, 0);
								PlusMinus (&iEXEGuardHP[iEXETabS], 154, 236, 0, 255, -1, 0);
								PlusMinus (&iEXEGuardHP[iEXETabS], 224, 236, 0, 255, 1, 0);
								PlusMinus (&iEXEGuardHP[iEXETabS], 239, 236, 0, 255, 10, 0);
							}

							/*** Guard uniform. ***/
							if (((iHomeComputer == 1) &&
								(ulGuardUniformA[iDiskImageA] != 0x00)) ||
								((iHomeComputer == 2) &&
								(ulGuardUniformB[iDiskImageB] != 0x00)) ||
								((iHomeComputer == 3) &&
								(ulGuardUniformC[iDiskImageC] != 0x00)))
							{
								PlusMinus (&iEXEGuardU[iEXETabS], 139, 260, 0, 1, -10, 0);
								PlusMinus (&iEXEGuardU[iEXETabS], 154, 260, 0, 1, -1, 0);
								PlusMinus (&iEXEGuardU[iEXETabS], 224, 260, 0, 1, 1, 0);
								PlusMinus (&iEXEGuardU[iEXETabS], 239, 260, 0, 1, 10, 0);
							}

							/*** Guard sprite. ***/
							if (((iHomeComputer == 1) &&
								(ulGuardSpriteA[iDiskImageA] != 0x00)) ||
								((iHomeComputer == 2) &&
								(ulGuardSpriteB[iDiskImageB] != 0x00)) ||
								((iHomeComputer == 3) &&
								(ulGuardSpriteC[iDiskImageC] != 0x00)))
							{
								if (iEXETabS > 2) /*** Levels 0, 1 and 2 must use 0x00. ***/
								{
									PlusMinus (&iEXEGuardS[iEXETabS], 139, 284, 1, 5, -10, 0);
									PlusMinus (&iEXEGuardS[iEXETabS], 154, 284, 1, 5, -1, 0);
									PlusMinus (&iEXEGuardS[iEXETabS], 224, 284, 1, 5, 1, 0);
									PlusMinus (&iEXEGuardS[iEXETabS], 239, 284, 1, 5, 10, 0);
								}
							}
						}

						if (iEXETabS > 2) /*** Levels 0, 1 and 2 must use 0x00. ***/
						{
							if (iEXETabS == 3) { iMin = 0; } else { iMin = 1; }

							if (((iHomeComputer == 1) && (ulEnv1A[iDiskImageA] != 0x00)) ||
								((iHomeComputer == 2) && (ulEnv1B[iDiskImageB] != 0x00)) ||
								((iHomeComputer == 3) && (ulEnv1C[iDiskImageC] != 0x00)))
							{
								PlusMinus (&iEXEEnv1[iEXETabS], 139, 308, iMin, 2, -10, 0);
								PlusMinus (&iEXEEnv1[iEXETabS], 154, 308, iMin, 2, -1, 0);
								PlusMinus (&iEXEEnv1[iEXETabS], 224, 308, iMin, 2, 1, 0);
								PlusMinus (&iEXEEnv1[iEXETabS], 239, 308, iMin, 2, 10, 0);
							}

							if (((iHomeComputer == 1) && (ulEnv2A[iDiskImageA] != 0x00)) ||
								((iHomeComputer == 2) && (ulEnv2B[iDiskImageB] != 0x00)) ||
								((iHomeComputer == 3) && (ulEnv2C[iDiskImageC] != 0x00)))
							{
								PlusMinus (&iEXEEnv2[iEXETabS], 139, 332, iMin, 2, -10, 0);
								PlusMinus (&iEXEEnv2[iEXETabS], 154, 332, iMin, 2, -1, 0);
								PlusMinus (&iEXEEnv2[iEXETabS], 224, 332, iMin, 2, 1, 0);
								PlusMinus (&iEXEEnv2[iEXETabS], 239, 332, iMin, 2, 10, 0);
							}
						}
					}
					ShowEXE();
					break;
				case SDL_WINDOWEVENT:
					if (event.window.event == SDL_WINDOWEVENT_EXPOSED)
						{ ShowEXE(); } break;
				case SDL_QUIT:
					Quit(); break;
			}
		}

		/*** prevent CPU eating ***/
		gamespeed = REFRESH_PROG;
		while ((SDL_GetTicks() - looptime) < gamespeed)
		{
			SDL_Delay (10);
		}
		looptime = SDL_GetTicks();
	}
	PlaySound ("wav/popup_close.wav");
	ShowScreen();
}
/*****************************************************************************/
void ShowEXE (void)
/*****************************************************************************/
{
	SDL_Color clr;
	SDL_Color clrs[12 + 2];
	int iTabX, iTabY;
	int iLeft, iRight;

	/*** Used for looping. ***/
	int iSkillLoop;

	/*** background ***/
	ShowImage (imgexe, 0, 0, "imgexe");

	/*** save button ***/
	switch (iEXESave)
	{
		case 0: /*** off ***/
			ShowImage (imgsave[1], 510, 411, "imgsave[1]"); break;
		case 1: /*** on ***/
			ShowImage (imgsave[2], 510, 411, "imgsave[2]"); break;
	}

	/*** Prince HP. ***/
	if (((iHomeComputer == 1) && (ulPrinceHPA[iDiskImageA] != 0x00)) ||
		((iHomeComputer == 2) && (ulPrinceHPB[iDiskImageB] != 0x00)) ||
		((iHomeComputer == 3) && (ulPrinceHPC[iDiskImageC] != 0x00)))
	{
		if (iEXEPrinceHP == 0x03) { clr = color_bl; } else { clr = color_blue; }
		CenterNumber (iEXEPrinceHP, 167, 58, clr, 0);
	}

	/*** Shadow HP. ***/
	if (((iHomeComputer == 1) && (ulShadowHPA[iDiskImageA] != 0x00)) ||
		((iHomeComputer == 2) && (ulShadowHPB[iDiskImageB] != 0x00)) ||
		((iHomeComputer == 3) && (ulShadowHPC[iDiskImageC] != 0x00)))
	{
		if (iEXEShadowHP == 0x04) { clr = color_bl; } else { clr = color_blue; }
		CenterNumber (iEXEShadowHP, 167, 82, clr, 0);
	}

	/*** Chomper delay. ***/
	if (((iHomeComputer == 1) && (ulChomperDelayA[iDiskImageA] != 0x00)) ||
		((iHomeComputer == 2) && (ulChomperDelayB[iDiskImageB] != 0x00)) ||
		((iHomeComputer == 3) && (ulChomperDelayC[iDiskImageC] != 0x00)))
	{
		if (iEXEChomperDelay == 0x10) { clr = color_bl; }
			else { clr = color_blue; }
		CenterNumber (iEXEChomperDelay, 167, 106, clr, 0);
	}

	/*** Mouse delay. ***/
	if (((iHomeComputer == 1) && (ulMouseDelayA[iDiskImageA] != 0x00)) ||
		((iHomeComputer == 2) && (ulMouseDelayB[iDiskImageB] != 0x00)) ||
		((iHomeComputer == 3) && (ulMouseDelayC[iDiskImageC] != 0x00)))
	{
		if (iEXEMouseDelay == 0x96) { clr = color_bl; } else { clr = color_blue; }
		CenterNumber (iEXEMouseDelay, 167, 130, clr, 0);
	}

	/*** Guard tab. ***/
	switch (iEXETab)
	{
		case 1: iTabX = 281; iTabY = 188; break; /*** strike prob. ***/
		case 2: iTabX = 383; iTabY = 188; break; /*** re-strike prob. ***/
		case 3: iTabX = 485; iTabY = 188; break; /*** block prob. ***/
		case 4: iTabX = 281; iTabY = 215; break; /*** imp. block prob. ***/
		case 5: iTabX = 383; iTabY = 215; break; /*** advance prob. ***/
		case 6: iTabX = 485; iTabY = 215; break; /*** refractory timer ***/
		case 7: iTabX = 281; iTabY = 242; break; /*** special color ***/
		case 8: iTabX = 383; iTabY = 242; break; /*** extra strength ***/
		default:
			printf ("[ WARN ] Unknown guard tab: %i!\n", iEXETab);
			iEXETab = 1; iTabX = 281; iTabY = 188; break;
	}
	ShowImage (imgexetab, iTabX, iTabY, "imgexetab");

	/*** Per level tab. ***/
	switch (iEXETabS)
	{
		case 0: iTabX = 267; iTabY = 299; break; /*** demo ***/
		case 1: iTabX = 331; iTabY = 299; break; /*** level 1 ***/
		case 2: iTabX = 395; iTabY = 299; break; /*** level 2 ***/
		case 3: iTabX = 459; iTabY = 299; break; /*** level 3 ***/
		case 4: iTabX = 523; iTabY = 299; break; /*** level 4 ***/
		case 5: iTabX = 267; iTabY = 326; break; /*** level 5 ***/
		case 6: iTabX = 331; iTabY = 326; break; /*** level 6 ***/
		case 7: iTabX = 395; iTabY = 326; break; /*** level 7 ***/
		case 8: iTabX = 459; iTabY = 326; break; /*** level 8 ***/
		case 9: iTabX = 523; iTabY = 326; break; /*** level 9 ***/
		case 10: iTabX = 267; iTabY = 353; break; /*** level 10 ***/
		case 11: iTabX = 331; iTabY = 353; break; /*** level 11 ***/
		case 12: iTabX = 395; iTabY = 353; break; /*** level 12 ***/
		case 13: iTabX = 459; iTabY = 353; break; /*** level 13 ***/
		case 14: iTabX = 523; iTabY = 353; break; /*** level 14 ***/
		default:
			printf ("[ WARN ] Unknown per level tab: %i!\n", iEXETabS);
			iEXETabS = 0; iTabX = 267; iTabY = 299; break;
	}
	ShowImage (imgexetabs, iTabX, iTabY, "imgexetabs");

	if (((iHomeComputer == 1) && (ulGuardA[iDiskImageA][iEXETab - 1] != 0x00)) ||
		((iHomeComputer == 2) && (ulGuardB[iDiskImageB][iEXETab - 1] != 0x00)) ||
		((iHomeComputer == 3) && (ulGuardC[iDiskImageC][iEXETab - 1] != 0x00)))
	{
		/* Use black.
		 * Override with blue if the setting does not match the default.
		 */
		for (iSkillLoop = 1; iSkillLoop <= 12; iSkillLoop++)
		{
			clrs[iSkillLoop] = color_bl;
			if (iEXEGuard[iEXETab][iSkillLoop] !=
				arDefaultGuard[iEXETab - 1][iSkillLoop - 1])
				{ clrs[iSkillLoop] = color_blue; }
		}
		iLeft = 338; iRight = 494;
		CenterNumber (iEXEGuard[iEXETab][1], iLeft, 10 + (1 * 24), clrs[1], 0);
		CenterNumber (iEXEGuard[iEXETab][2], iRight, 10 + (1 * 24), clrs[2], 0);
		CenterNumber (iEXEGuard[iEXETab][3], iLeft, 10 + (2 * 24), clrs[3], 0);
		CenterNumber (iEXEGuard[iEXETab][4], iRight, 10 + (2 * 24), clrs[4], 0);
		CenterNumber (iEXEGuard[iEXETab][5], iLeft, 10 + (3 * 24), clrs[5], 0);
		CenterNumber (iEXEGuard[iEXETab][6], iRight, 10 + (3 * 24), clrs[6], 0);
		CenterNumber (iEXEGuard[iEXETab][7], iLeft, 10 + (4 * 24), clrs[7], 0);
		CenterNumber (iEXEGuard[iEXETab][8], iRight, 10 + (4 * 24), clrs[8], 0);
		CenterNumber (iEXEGuard[iEXETab][9], iLeft, 10 + (5 * 24), clrs[9], 0);
		CenterNumber (iEXEGuard[iEXETab][10], iRight, 10 + (5 * 24), clrs[10], 0);
		CenterNumber (iEXEGuard[iEXETab][11], iLeft, 10 + (6 * 24), clrs[11], 0);
		CenterNumber (iEXEGuard[iEXETab][12], iRight, 10 + (6 * 24), clrs[12], 0);
	}

	/*** Guard HP. ***/
	if (((iHomeComputer == 1) && (ulGuardHPA[iDiskImageA] != 0x00)) ||
		((iHomeComputer == 2) && (ulGuardHPB[iDiskImageB] != 0x00)) ||
		((iHomeComputer == 3) && (ulGuardHPC[iDiskImageC] != 0x00)))
	{
		if (iEXETabS != 14)
		{
			clr = color_bl;
			if (iEXEGuardHP[iEXETabS] != arDefaultGuardHP[iEXETabS])
				{ clr = color_blue; }
			CenterNumber (iEXEGuardHP[iEXETabS], 167, 236, clr, 0);
		}
	}

	/*** Guard uniform. ***/
	if (((iHomeComputer == 1) && (ulGuardUniformA[iDiskImageA] != 0x00)) ||
		((iHomeComputer == 2) && (ulGuardUniformB[iDiskImageB] != 0x00)) ||
		((iHomeComputer == 3) && (ulGuardUniformC[iDiskImageC] != 0x00)))
	{
		if (iEXETabS != 14)
		{
			clr = color_bl;
			if (iEXEGuardU[iEXETabS] != arDefaultGuardU[iEXETabS])
				{ clr = color_blue; }
			CenterNumber (iEXEGuardU[iEXETabS], 167, 260, clr, 0);
		}
	}

	/*** Guard sprite. ***/
	if (((iHomeComputer == 1) && (ulGuardSpriteA[iDiskImageA] != 0x00)) ||
		((iHomeComputer == 2) && (ulGuardSpriteB[iDiskImageB] != 0x00)) ||
		((iHomeComputer == 3) && (ulGuardSpriteC[iDiskImageC] != 0x00)))
	{
		if (iEXETabS != 14)
		{
			if (iEXETabS > 2) { clr = color_bl; } else { clr = color_gray; }
			if (iEXEGuardS[iEXETabS] != arDefaultGuardS[iEXETabS])
				{ clr = color_blue; }
			CenterNumber (iEXEGuardS[iEXETabS], 167, 284, clr, 0);
		}
	}

	/*** Env. 1 ***/
	if (((iHomeComputer == 1) && (ulEnv1A[iDiskImageA] != 0x00)) ||
		((iHomeComputer == 2) && (ulEnv1B[iDiskImageB] != 0x00)) ||
		((iHomeComputer == 3) && (ulEnv1C[iDiskImageC] != 0x00)))
	{
		if (iEXETabS > 2) { clr = color_bl; } else { clr = color_gray; }
		if (iEXEEnv1[iEXETabS] != arDefaultEnv1[iEXETabS])
			{ clr = color_blue; }
		CenterNumber (iEXEEnv1[iEXETabS], 167, 308, clr, 0);
	}

	/*** Env. 2 ***/
	if (((iHomeComputer == 1) && (ulEnv2A[iDiskImageA] != 0x00)) ||
		((iHomeComputer == 2) && (ulEnv2B[iDiskImageB] != 0x00)) ||
		((iHomeComputer == 3) && (ulEnv2C[iDiskImageC] != 0x00)))
	{
		if (iEXETabS > 2) { clr = color_bl; } else { clr = color_gray; }
		if (iEXEEnv2[iEXETabS] != arDefaultEnv2[iEXETabS])
			{ clr = color_blue; }
		CenterNumber (iEXEEnv2[iEXETabS], 167, 332, clr, 0);
	}

	/*** Environment image. ***/
	if (((iHomeComputer == 1) && (ulEnv1A[iDiskImageA] != 0x00) &&
		(ulEnv2A[iDiskImageA] != 0x00)) ||
		((iHomeComputer == 2) && (ulEnv1B[iDiskImageB] != 0x00) &&
		(ulEnv2B[iDiskImageB] != 0x00)) ||
		((iHomeComputer == 3) && (ulEnv1C[iDiskImageC] != 0x00) &&
		(ulEnv2C[iDiskImageC] != 0x00)))
	{
		if (iEXEEnv1[iEXETabS] == iEXEEnv2[iEXETabS])
		{
			ShowImage (imgexeenvok, 53, 321, "imgexeenvok");
		} else {
			ShowImage (imgexeenvwarn, 53, 321, "imgexeenvwarn");
		}
	}

	/*** refresh screen ***/
	SDL_RenderPresent (ascreen);
}
/*****************************************************************************/
void InitScreenAction (char *sAction)
/*****************************************************************************/
{
	int iEventTile;
	int iEventRoom;

	if (strcmp (sAction, "left") == 0)
	{
		switch (iScreen)
		{
			case 1:
				iSelected--;
				switch (iSelected)
				{
					case 0: iSelected = 10; break;
					case 10: iSelected = 20; break;
					case 20: iSelected = 30; break;
				}
				break;
			case 2:
				if (arBrokenRoomLinks[iCurLevel] == 0)
				{
					if (iMovingRoom != 0)
					{
						if (iMovingNewX != 1) { iMovingNewX--; }
							else { iMovingNewX = 25; }
						PlaySound ("wav/cross.wav");
					}
				} else {
					if (iChangingBrokenSide != 1)
					{
						iChangingBrokenSide = 1;
					} else {
						switch (iChangingBrokenRoom)
						{
							case 1: iChangingBrokenRoom = 4; break;
							case 5: iChangingBrokenRoom = 8; break;
							case 9: iChangingBrokenRoom = 12; break;
							case 13: iChangingBrokenRoom = 16; break;
							case 17: iChangingBrokenRoom = 20; break;
							case 21: iChangingBrokenRoom = 24; break;
							default: iChangingBrokenRoom--; break;
						}
					}
				}
				break;
			case 3:
				iEventTile = arEventsTile[iCurLevel][iChangeEvent];
				switch (iEventTile)
				{
					case 1: iEventTile = 10; break;
					case 11: iEventTile = 20; break;
					case 21: iEventTile = 30; break;
					default: iEventTile--; break;
				}
				arEventsTile[iCurLevel][iChangeEvent] = iEventTile;
				PlaySound ("wav/check_box.wav");
				iChanged++;
				break;
		}
	}

	if (strcmp (sAction, "right") == 0)
	{
		switch (iScreen)
		{
			case 1:
				iSelected++;
				switch (iSelected)
				{
					case 11: iSelected = 1; break;
					case 21: iSelected = 11; break;
					case 31: iSelected = 21; break;
				}
				break;
			case 2:
				if (arBrokenRoomLinks[iCurLevel] == 0)
				{
					if (iMovingRoom != 0)
					{
						if (iMovingNewX != 25) { iMovingNewX++; }
							else { iMovingNewX = 1; }
						PlaySound ("wav/cross.wav");
					}
				} else {
					if (iChangingBrokenSide != 2)
					{
						iChangingBrokenSide = 2;
					} else {
						switch (iChangingBrokenRoom)
						{
							case 4: iChangingBrokenRoom = 1; break;
							case 8: iChangingBrokenRoom = 5; break;
							case 12: iChangingBrokenRoom = 9; break;
							case 16: iChangingBrokenRoom = 13; break;
							case 20: iChangingBrokenRoom = 17; break;
							case 24: iChangingBrokenRoom = 21; break;
							default: iChangingBrokenRoom++; break;
						}
					}
				}
				break;
			case 3:
				iEventTile = arEventsTile[iCurLevel][iChangeEvent];
				switch (iEventTile)
				{
					case 10: iEventTile = 1; break;
					case 20: iEventTile = 11; break;
					case 30: iEventTile = 21; break;
					default: iEventTile++; break;
				}
				arEventsTile[iCurLevel][iChangeEvent] = iEventTile;
				PlaySound ("wav/check_box.wav");
				iChanged++;
				break;
		}
	}

	if (strcmp (sAction, "up") == 0)
	{
		switch (iScreen)
		{
			case 1:
				if (iSelected > 10) { iSelected-=10; }
					else { iSelected+=20; }
				break;
			case 2:
				if (arBrokenRoomLinks[iCurLevel] == 0)
				{
					if (iMovingRoom != 0)
					{
						if (iMovingNewY != 1) { iMovingNewY--; }
							else { iMovingNewY = 24; }
						PlaySound ("wav/cross.wav");
					}
				} else {
					if (iChangingBrokenSide != 3)
					{
						iChangingBrokenSide = 3;
					} else {
						switch (iChangingBrokenRoom)
						{
							case 1: iChangingBrokenRoom = 21; break;
							case 2: iChangingBrokenRoom = 22; break;
							case 3: iChangingBrokenRoom = 23; break;
							case 4: iChangingBrokenRoom = 24; break;
							default: iChangingBrokenRoom -= 4; break;
						}
					}
				}
				break;
			case 3:
				iEventTile = arEventsTile[iCurLevel][iChangeEvent];
				if (iEventTile > 10) { iEventTile-=10; }
					else { iEventTile+=20; }
				arEventsTile[iCurLevel][iChangeEvent] = iEventTile;
				PlaySound ("wav/check_box.wav");
				iChanged++;
				break;
		}
	}

	if (strcmp (sAction, "down") == 0)
	{
		switch (iScreen)
		{
			case 1:
				if (iSelected <= 20) { iSelected+=10; }
					else { iSelected-=20; }
				break;
			case 2:
				if (arBrokenRoomLinks[iCurLevel] == 0)
				{
					if (iMovingRoom != 0)
					{
						if (iMovingNewY != 24) { iMovingNewY++; }
							else { iMovingNewY = 1; }
						PlaySound ("wav/cross.wav");
					}
				} else {
					if (iChangingBrokenSide != 4)
					{
						iChangingBrokenSide = 4;
					} else {
						switch (iChangingBrokenRoom)
						{
							case 21: iChangingBrokenRoom = 1; break;
							case 22: iChangingBrokenRoom = 2; break;
							case 23: iChangingBrokenRoom = 3; break;
							case 24: iChangingBrokenRoom = 4; break;
							default: iChangingBrokenRoom += 4; break;
						}
					}
				}
				break;
			case 3:
				iEventTile = arEventsTile[iCurLevel][iChangeEvent];
				if (iEventTile <= 20) { iEventTile+=10; }
					else { iEventTile-=20; }
				arEventsTile[iCurLevel][iChangeEvent] = iEventTile;
				PlaySound ("wav/check_box.wav");
				iChanged++;
				break;
		}
	}

	if (strcmp (sAction, "left bracket") == 0)
	{
		switch (iScreen)
		{
			case 2:
				if (arBrokenRoomLinks[iCurLevel] == 0)
				{
					iMovingNewBusy = 0;
					switch (iMovingRoom)
					{
						case 0: iMovingRoom = ROOMS; break; /*** If disabled. ***/
						case 1: iMovingRoom = ROOMS; break;
						default: iMovingRoom--; break;
					}
				}
				break;
			case 3:
				iEventRoom = arEventsRoom[iCurLevel][iChangeEvent];
				if ((iEventRoom >= 2) && (iEventRoom <= 24))
				{
					iEventRoom--;
				} else {
					iEventRoom = 24;
				}
				arEventsRoom[iCurLevel][iChangeEvent] = iEventRoom;
				PlaySound ("wav/check_box.wav");
				iChanged++;
				break;
		}
	}

	if (strcmp (sAction, "right bracket") == 0)
	{
		switch (iScreen)
		{
			case 2:
				if (arBrokenRoomLinks[iCurLevel] == 0)
				{
					iMovingNewBusy = 0;
					switch (iMovingRoom)
					{
						case 0: iMovingRoom = 1; break; /*** If disabled. ***/
						case 24: iMovingRoom = 1; break;
						default: iMovingRoom++; break;
					}
				}
				break;
			case 3:
				iEventRoom = arEventsRoom[iCurLevel][iChangeEvent];
				if ((iEventRoom >= 1) & (iEventRoom <= 23))
				{
					iEventRoom++;
				} else {
					iEventRoom = 1;
				}
				arEventsRoom[iCurLevel][iChangeEvent] = iEventRoom;
				PlaySound ("wav/check_box.wav");
				iChanged++;
				break;
		}
	}

	if (strcmp (sAction, "enter") == 0)
	{
		switch (iScreen)
		{
			case 1:
				ChangePos();
				break;
			case 2:
				if (arBrokenRoomLinks[iCurLevel] == 0)
				{
					if (iMovingRoom != 0)
					{
						if (arMovingRooms[iMovingNewX][iMovingNewY] == 0)
						{
							RemoveOldRoom();
							AddNewRoom (iMovingNewX, iMovingNewY, iMovingRoom);
							iChanged++;
						}
						iMovingRoom = 0; iMovingNewBusy = 0;
					}
				} else {
					LinkPlus();
				}
				break;
		}
	}

	if (strcmp (sAction, "env") == 0)
	{
		if (((iHomeComputer == 1) && (ulEnv1A[iDiskImageA] != 0x00)) ||
			((iHomeComputer == 2) && (ulEnv1B[iDiskImageB] != 0x00)) ||
			((iHomeComputer == 3) && (ulEnv1C[iDiskImageC] != 0x00)))
		{
			if ((iCurLevel >= 3) && (iCurLevel <= 14))
			{
				EXELoad();
				switch (cCurType)
				{
					case 'd':
						cCurType = 'p';
						iEXEEnv1[iCurLevel] = 0x01;
						iEXEEnv2[iCurLevel] = 0x01;
						break;
					case 'p':
						cCurType = 'd';
						iEXEEnv1[iCurLevel] = 0x02;
						iEXEEnv2[iCurLevel] = 0x02;
						break;
				}
				EXESave();
				PlaySound ("wav/extras.wav");
			} else {
				printf ("[ INFO ] The environment of levels 1 and 2 (and 0) cannot be"
					" changed.\n");
			}
		} else {
			printf ("[ INFO ] Unknown environment offset.\n");
		}
	}
}
/*****************************************************************************/
void RunLevel (int iLevel)
/*****************************************************************************/
{
	SDL_Thread *princethread;

	if (iDebug == 1)
	{
		printf ("[  OK  ] Starting the game in level %i.\n", iLevel);
	}

	PlaytestStart (iLevel);

	princethread = SDL_CreateThread (StartGame, "StartGame", NULL);
	if (princethread == NULL)
	{
		printf ("[FAILED] Could not create thread!\n");
		exit (EXIT_ERROR);
	}
}
/*****************************************************************************/
int StartGame (void *unused)
/*****************************************************************************/
{
	char sSystem[200 + 2];
	char sWine[200 + 2];

	if (unused != NULL) { } /*** To prevent warnings. ***/

	PlaySound ("wav/emulator.wav");

	switch (iHomeComputer)
	{
		case 1: /*** AppleWin ***/
#if defined WIN32 || _WIN32 || WIN64 || _WIN64
snprintf (sWine, 200, "%s", "");
#else
snprintf (sWine, 200, "%s", "wine ");
#endif

			snprintf (sSystem, 200, "%sAppleWin%sAppleWin.exe -model %s -h1 %s > %s",
				sWine, SLASH, "apple2ee", sPathFile, DEVNULL);
			break;
		case 2: /*** B-em ***/
			snprintf (sSystem, 200, "b-em -m10 -fasttape %s > %s",
				sPathFile, DEVNULL);
			break;
		case 3: /*** VICE ***/
			snprintf (sSystem, 200, "x64sc %s > %s",
				sPathFile, DEVNULL);
			break;
	}
	if (system (sSystem) == -1)
		{ printf ("[ WARN ] Could not execute emulator!\n"); }
	if (iModified == 1) { PlaytestStop(); }

	return (EXIT_NORMAL);
}
/*****************************************************************************/
void ClearRoom (void)
/*****************************************************************************/
{
	int iTileLoop;

	/*** Remove tiles. ***/
	for (iTileLoop = 1; iTileLoop <= 30; iTileLoop++)
	{
		SetLocation (iCurRoom, iTileLoop, 0x00, 0x00);
	}

	/*** Remove guard. ***/
	arGuardTile[iCurLevel][iCurRoom] = TILES + 1;

	PlaySound ("wav/ok_close.wav");
	iChanged++;
}
/*****************************************************************************/
void UseTile (int iTile, int iLocation, int iRoom)
/*****************************************************************************/
{
	/*** Do not use iSelected in this function. ***/

	int iDir;
	int iGetTile, iGetMod;

	/*** Random tile. ***/
	if (iTile == -1)
	{
		do {
			iTile = 1 + (int) (78.0 * rand() / (RAND_MAX + 1.0));
		} while (Unused (iTile) == 1);
	}

	/*** Custom tile. ***/
	if (iTile == -2)
	{
		SetLocation (iRoom, iLocation,
			(32 * iCustomX) + iCustomTile, iCustomMod);
	}

	/*** Make sure the disabled living can't be used. ***/
	if ((iCurGuard != 0x01) && ((iTile == 83) || (iTile == 84))) { return; }
	if ((iCurGuard != 0x03) && ((iTile == 85) || (iTile == 86))) { return; }
	if ((iCurGuard != 0x04) && ((iTile == 87) || (iTile == 88))) { return; }
	if ((iCurGuard != 0x05) && ((iTile == 89) || (iTile == 90))) { return; }
	if ((((iCurGuard != 0x00) && (iCurGuard != 0x02)) || (iCurLevel == 14)) &&
		((iTile == 81) || (iTile == 82))) { return; }

	if ((iTile >= 1) && (iTile <= 78))
	{
		GetTileModChange (iTile, &iGetTile, &iGetMod);
		SetLocation (iRoom, iLocation, iGetTile, iGetMod);
	}

	switch (iTile)
	{
		/*** prince ***/
		case 79: /*** turned right ***/
			if ((arStartLocation[iCurLevel][1] != iCurRoom) ||
				(arStartLocation[iCurLevel][2] != iLocation) ||
				(arStartLocation[iCurLevel][3] != 0x00))
			{
				arStartLocation[iCurLevel][1] = iCurRoom;
				arStartLocation[iCurLevel][2] = iLocation;
				arStartLocation[iCurLevel][3] = 0x00;
				PlaySound ("wav/hum_adj.wav");
			}
			break;
		case 80: /*** turned left ***/
			if ((arStartLocation[iCurLevel][1] != iCurRoom) ||
				(arStartLocation[iCurLevel][2] != iLocation) ||
				(arStartLocation[iCurLevel][3] != 0xFF))
			{
				arStartLocation[iCurLevel][1] = iCurRoom;
				arStartLocation[iCurLevel][2] = iLocation;
				arStartLocation[iCurLevel][3] = 0xFF;
				PlaySound ("wav/hum_adj.wav");
			}
			break;

		/*** guards ***/
		case 81: case 82: /*** guard ***/
		case 83: case 84: /*** skel ***/
		case 85: case 86: /*** fat ***/
		case 87: case 88: /*** shadow ***/
		case 89: case 90: /*** Jaffar ***/
			switch (iTile)
			{
				case 81: case 83: case 85: case 87: case 89:
					iDir = 0x00; break;
				case 82: case 84: case 86: case 88: case 90:
				default: /*** To prevent warnings. ***/
					iDir = 0xFF; break;
			}
			if ((arGuardTile[iCurLevel][iCurRoom] == iLocation) &&
				(arGuardDir[iCurLevel][iCurRoom] == iDir))
			{
				arGuardTile[iCurLevel][iCurRoom] = TILES + 1;
			} else {
				arGuardTile[iCurLevel][iCurRoom] = iLocation;
				arGuardDir[iCurLevel][iCurRoom] = iDir;
				arGuardSkill[iCurLevel][iCurRoom] = iGuardType;
				/*** arGuardC[iCurLevel][iCurRoom] = ; ***/
				PlaySound ("wav/hum_adj.wav");
			}
			break;
	}
}
/*****************************************************************************/
void Zoom (int iToggleFull)
/*****************************************************************************/
{
	if (iToggleFull == 1)
	{
		if (iFullscreen == 0)
		{ iFullscreen = SDL_WINDOW_FULLSCREEN_DESKTOP; }
			else { iFullscreen = 0; }
	} else {
		if (iFullscreen == SDL_WINDOW_FULLSCREEN_DESKTOP)
		{
			iFullscreen = 0;
			iScale = 1;
		} else if (iScale == 1) {
			iScale = 2;
		} else if (iScale == 2) {
			iFullscreen = SDL_WINDOW_FULLSCREEN_DESKTOP;
		} else {
			printf ("[ WARN ] Unknown window state!\n");
		}
	}

	SDL_SetWindowFullscreen (window, iFullscreen);
	SDL_SetWindowSize (window, (WINDOW_WIDTH) * iScale,
		(WINDOW_HEIGHT) * iScale);
	SDL_RenderSetLogicalSize (ascreen, (WINDOW_WIDTH) * iScale,
		(WINDOW_HEIGHT) * iScale);
	SDL_SetWindowPosition (window, SDL_WINDOWPOS_CENTERED,
		SDL_WINDOWPOS_CENTERED);
	TTF_CloseFont (font1);
	TTF_CloseFont (font2);
	TTF_CloseFont (font3);
	LoadFonts();
}
/*****************************************************************************/
void LinkMinus (void)
/*****************************************************************************/
{
	int iCurrent, iNew;

	iCurrent = arRoomLinks[iCurLevel][iChangingBrokenRoom][iChangingBrokenSide];
	if (iCurrent == 0) {
		iNew = ROOMS;
	} else {
		iNew = iCurrent - 1;
	}
	arRoomLinks[iCurLevel][iChangingBrokenRoom][iChangingBrokenSide] = iNew;
	iChanged++;
	arBrokenRoomLinks[iCurLevel] = BrokenRoomLinks (0);
	PlaySound ("wav/hum_adj.wav");
}
/*****************************************************************************/
int BrokenRoomLinks (int iPrint)
/*****************************************************************************/
{
	int iBroken;
	int iRoomLoop;

	for (iRoomLoop = 1; iRoomLoop <= ROOMS; iRoomLoop++)
	{
		arDone[iRoomLoop] = 0;
		arRoomConnectionsBroken[iCurLevel][iRoomLoop][1] = 0;
		arRoomConnectionsBroken[iCurLevel][iRoomLoop][2] = 0;
		arRoomConnectionsBroken[iCurLevel][iRoomLoop][3] = 0;
		arRoomConnectionsBroken[iCurLevel][iRoomLoop][4] = 0;
	}
	CheckSides (arStartLocation[iCurLevel][1], 0, 0);
	iBroken = 0;

	for (iRoomLoop = 1; iRoomLoop <= ROOMS; iRoomLoop++)
	{
		/*** If the room is in use... ***/
		if (arDone[iRoomLoop] == 1)
		{
			/*** check left ***/
			if (arRoomLinks[iCurLevel][iRoomLoop][1] != 0)
			{
				if ((arRoomLinks[iCurLevel][iRoomLoop][1] == iRoomLoop) ||
					(arRoomLinks[iCurLevel][arRoomLinks[iCurLevel][iRoomLoop][1]][2]
					!= iRoomLoop) ||
					(arRoomLinks[iCurLevel][iRoomLoop][1] > ROOMS))
				{
					arRoomConnectionsBroken[iCurLevel][iRoomLoop][1] = 1;
					iBroken = 1;
					if ((iDebug == 1) && (iPrint == 1))
					{
						printf ("[ INFO ] The left of room %i has a broken link.\n",
							iRoomLoop);
					}
				}
			}
			/*** check right ***/
			if (arRoomLinks[iCurLevel][iRoomLoop][2] != 0)
			{
				if ((arRoomLinks[iCurLevel][iRoomLoop][2] == iRoomLoop) ||
					(arRoomLinks[iCurLevel][arRoomLinks[iCurLevel][iRoomLoop][2]][1]
					!= iRoomLoop) ||
					(arRoomLinks[iCurLevel][iRoomLoop][2] > ROOMS))
				{
					arRoomConnectionsBroken[iCurLevel][iRoomLoop][2] = 1;
					iBroken = 1;
					if ((iDebug == 1) && (iPrint == 1))
					{
						printf ("[ INFO ] The right of room %i has a broken link.\n",
							iRoomLoop);
					}
				}
			}
			/*** check up ***/
			if (arRoomLinks[iCurLevel][iRoomLoop][3] != 0)
			{
				if ((arRoomLinks[iCurLevel][iRoomLoop][3] == iRoomLoop) ||
					(arRoomLinks[iCurLevel][arRoomLinks[iCurLevel][iRoomLoop][3]][4]
					!= iRoomLoop) ||
					(arRoomLinks[iCurLevel][iRoomLoop][3] > ROOMS))
				{
					arRoomConnectionsBroken[iCurLevel][iRoomLoop][3] = 1;
					iBroken = 1;
					if ((iDebug == 1) && (iPrint == 1))
					{
						printf ("[ INFO ] The top of room %i has a broken link.\n",
							iRoomLoop);
					}
				}
			}
			/*** check down ***/
			if (arRoomLinks[iCurLevel][iRoomLoop][4] != 0)
			{
				if ((arRoomLinks[iCurLevel][iRoomLoop][4] == iRoomLoop) ||
					(arRoomLinks[iCurLevel][arRoomLinks[iCurLevel][iRoomLoop][4]][3]
					!= iRoomLoop) ||
					(arRoomLinks[iCurLevel][iRoomLoop][4] > ROOMS))
				{
					arRoomConnectionsBroken[iCurLevel][iRoomLoop][4] = 1;
					iBroken = 1;
					if ((iDebug == 1) && (iPrint == 1))
					{
						printf ("[ INFO ] The bottom of room %i has a broken link.\n",
							iRoomLoop);
					}
				}
			}
		}
	}

	return (iBroken);
}
/*****************************************************************************/
void ChangeEvent (int iAmount, int iChangePos)
/*****************************************************************************/
{
	int iTile;

	if (((iAmount > 0) && (iChangeEvent < EVENTS)) ||
		((iAmount < 0) && (iChangeEvent > 1)))
	{
		/*** Modify the event number. ***/
		iChangeEvent+=iAmount;
		if (iChangeEvent < 1) { iChangeEvent = 1; }
		if (iChangeEvent > EVENTS) { iChangeEvent = EVENTS; }

		/*** If necessary, apply the event number to the selected tile. ***/
		if (iChangePos == 1)
		{
			iTile = arRoomTiles[iCurLevel][iCurRoom][iSelected];
			if ((iTile == 0x06) || (iTile == 0x0F)) /*** drop or raise ***/
			{
				arRoomMod[iCurLevel][iCurRoom][iSelected] = iChangeEvent - 1;
				iChanged++;
			}
		}

		PlaySound ("wav/plus_minus.wav");
	}
}
/*****************************************************************************/
void ChangeCustom (int iAmount, int iType)
/*****************************************************************************/
{
	if (iType == 1)
	{
		/*** Modify the custom tile. ***/
		if (((iAmount > 0) && (iCustomTile < 0x1F)) ||
			((iAmount < 0) && (iCustomTile > 0x00)))
		{
			iCustomTile+=iAmount;
			if (iCustomTile < 0x00) { iCustomTile = 0x00; }
			if (iCustomTile > 0x1F) { iCustomTile = 0x1F; }

			PlaySound ("wav/plus_minus.wav");
		}
	} else {
		/*** Modify the custom mod. ***/
		if (((iAmount > 0) && (iCustomMod < 0xFF)) ||
			((iAmount < 0) && (iCustomMod > 0x00)))
		{
			iCustomMod+=iAmount;
			if (iCustomMod < 0x00) { iCustomMod = 0x00; }
			if (iCustomMod > 0xFF) { iCustomMod = 0xFF; }

			PlaySound ("wav/plus_minus.wav");
		}
	}
}
/*****************************************************************************/
void Prev (void)
/*****************************************************************************/
{
	if (iCurLevel != 1)
	{
		iCurLevel--;
		LoadLevels (iCurLevel); iChanged = 0; /*** Discard changes. ***/
		iCurRoom = arStartLocation[iCurLevel][1];
		PlaySound ("wav/level_change.wav");
	}
}
/*****************************************************************************/
void Next (void)
/*****************************************************************************/
{
	if (iCurLevel != LEVELS)
	{
		iCurLevel++;
		LoadLevels (iCurLevel); iChanged = 0; /*** Discard changes. ***/
		iCurRoom = arStartLocation[iCurLevel][1];
		PlaySound ("wav/level_change.wav");
	}
}
/*****************************************************************************/
void CallSave (void)
/*****************************************************************************/
{
	CreateBAK();
	SaveLevels();
}
/*****************************************************************************/
void Sprinkle (void)
/*****************************************************************************/
{
	int iRandom;
	int iTile, iMod;

	/*** Used for looping. ***/
	int iRoomLoop;
	int iTileLoop;

	for (iRoomLoop = 1; iRoomLoop <= ROOMS; iRoomLoop++)
	{
		for (iTileLoop = 1; iTileLoop <= TILES; iTileLoop++)
		{
			iTile = arRoomTiles[iCurLevel][iRoomLoop][iTileLoop];
			iMod = arRoomMod[iCurLevel][iRoomLoop][iTileLoop];

			/*** d: space? add wall shadow ***/
			/*** p: wall pattern? add variant pattern ***/
			if ((iTile == 0x00) && ((((iMod == 0x00) || (iMod == 0xFF)) &&
				(cCurType == 'd')) || ((iMod == 0x01) && (cCurType == 'p'))))
			{
				/*** 1-4 ***/
				iRandom = 1 + (int) (4.0 * rand() / (RAND_MAX + 1.0));
				switch (iRandom)
				{
					case 1:
						/*** arRoomTiles unchanged. ***/
						if (cCurType == 'd')
						{
							arRoomMod[iCurLevel][iRoomLoop][iTileLoop] = 0x01;
						}
						break;
					case 2:
						/*** arRoomTiles unchanged. ***/
						arRoomMod[iCurLevel][iRoomLoop][iTileLoop] = 0x02;
						break;
				}
			}

			/*** d: empty floor? add wall shadow (2x), rubble, torch, skeleton ***/
			/*** p: empty floor? add variant pattern, rubble, torch ***/
			if ((iTile == 0x01) && ((iMod == 0x00) || (iMod == 0xFF)))
			{
				/*** 1-10 ***/
				iRandom = 1 + (int) (10.0 * rand() / (RAND_MAX + 1.0));
				switch (iRandom)
				{
					case 1: /*** wall shadow ***/
						/*** arRoomTiles unchanged. ***/
						if (cCurType == 'd')
						{
							arRoomMod[iCurLevel][iRoomLoop][iTileLoop] = 0x01;
						}
						break;
					case 2: /*** wall shadow / variant pattern ***/
						/*** arRoomTiles unchanged. ***/
						arRoomMod[iCurLevel][iRoomLoop][iTileLoop] = 0x02;
						break;
					case 3: /*** rubble ***/
						arRoomTiles[iCurLevel][iRoomLoop][iTileLoop] = 0x0E;
						arRoomMod[iCurLevel][iRoomLoop][iTileLoop] = 0x00;
						break;
					case 4: /*** torch ***/
						arRoomTiles[iCurLevel][iRoomLoop][iTileLoop] = 0x13;
						arRoomMod[iCurLevel][iRoomLoop][iTileLoop] = 0x00;
						break;
					case 5: /*** skeleton ***/
						if (cCurType == 'd')
						{
							arRoomTiles[iCurLevel][iRoomLoop][iTileLoop] = 0x15;
							arRoomMod[iCurLevel][iRoomLoop][iTileLoop] = 0x00;
						}
						break;
				}
			}
		}
	}
}
/*****************************************************************************/
void SetLocation (int iRoom, int iLocation, int iTile, int iMod)
/*****************************************************************************/
{
	if (iTile >= 32)
	{
		iTile-=32;
		arRoomX[iCurLevel][iRoom][iLocation] = 1;
		iLastX = 1;
	} else {
		arRoomX[iCurLevel][iRoom][iLocation] = 0;
		iLastX = 0;
	}
	arRoomTiles[iCurLevel][iRoom][iLocation] = iTile;
	arRoomMod[iCurLevel][iRoom][iLocation] = iMod;
	iLastTile = iTile;
	iLastMod = iMod;
}
/*****************************************************************************/
void FlipRoom (int iAxis)
/*****************************************************************************/
{
	int arRoomXTemp[TILES + 2];
	unsigned char arRoomTilesTemp[TILES + 2];
	unsigned char arRoomModTemp[TILES + 2];
	int iTileUse;
	int iTile;

	/*** Used for looping. ***/
	int iTileLoop;

	/*** Storing tiles for later use. ***/
	for (iTileLoop = 1; iTileLoop <= TILES; iTileLoop++)
	{
		arRoomXTemp[iTileLoop] = arRoomX[iCurLevel][iCurRoom][iTileLoop];
		arRoomTilesTemp[iTileLoop] = arRoomTiles[iCurLevel][iCurRoom][iTileLoop];
		arRoomModTemp[iTileLoop] = arRoomMod[iCurLevel][iCurRoom][iTileLoop];
	}

	if (iAxis == 1) /*** horizontal ***/
	{
		/*** tiles ***/
		for (iTileLoop = 1; iTileLoop <= TILES; iTileLoop++)
		{
			iTileUse = 0; /*** To prevent warnings. ***/
			if ((iTileLoop >= 1) && (iTileLoop <= 10))
				{ iTileUse = 11 - iTileLoop; }
			if ((iTileLoop >= 11) && (iTileLoop <= 20))
				{ iTileUse = 31 - iTileLoop; }
			if ((iTileLoop >= 21) && (iTileLoop <= 30))
				{ iTileUse = 51 - iTileLoop; }
			arRoomX[iCurLevel][iCurRoom][iTileLoop] = arRoomXTemp[iTileUse];
			arRoomTiles[iCurLevel][iCurRoom][iTileLoop] = arRoomTilesTemp[iTileUse];
			arRoomMod[iCurLevel][iCurRoom][iTileLoop] = arRoomModTemp[iTileUse];
		}

		/*** prince ***/
		if (arStartLocation[iCurLevel][1] == iCurRoom)
		{
			/*** direction ***/
			if (arStartLocation[iCurLevel][3] == 0x00)
				{ arStartLocation[iCurLevel][3] = 0xFF; }
					else { arStartLocation[iCurLevel][3] = 0x00; }
			/*** tile ***/
			iTile = arStartLocation[iCurLevel][2];
			if ((iTile >= 1) && (iTile <= 10))
				{ arStartLocation[iCurLevel][2] = 11 - iTile; }
			if ((iTile >= 11) && (iTile <= 20))
				{ arStartLocation[iCurLevel][2] = 31 - iTile; }
			if ((iTile >= 21) && (iTile <= 30))
				{ arStartLocation[iCurLevel][2] = 51 - iTile; }
		}

		/*** guard ***/
		if (arGuardTile[iCurLevel][iCurRoom] <= TILES + 1)
		{
			/*** direction ***/
			if (arGuardDir[iCurLevel][iCurRoom] == 0x00)
				{ arGuardDir[iCurLevel][iCurRoom] = 0xFF; }
					else { arGuardDir[iCurLevel][iCurRoom] = 0x00; }
			/*** tile ***/
			iTile = arGuardTile[iCurLevel][iCurRoom];
			if ((iTile >= 1) && (iTile <= 10))
				{ arGuardTile[iCurLevel][iCurRoom] = 11 - iTile; }
			if ((iTile >= 11) && (iTile <= 20))
				{ arGuardTile[iCurLevel][iCurRoom] = 31 - iTile; }
			if ((iTile >= 21) && (iTile <= 30))
				{ arGuardTile[iCurLevel][iCurRoom] = 51 - iTile; }
		}
	} else { /*** vertical ***/
		/*** tiles ***/
		for (iTileLoop = 1; iTileLoop <= TILES; iTileLoop++)
		{
			iTileUse = 0; /*** To prevent warnings. ***/
			if ((iTileLoop >= 1) && (iTileLoop <= 10))
				{ iTileUse = iTileLoop + 20; }
			if ((iTileLoop >= 11) && (iTileLoop <= 20))
				{ iTileUse = iTileLoop; }
			if ((iTileLoop >= 21) && (iTileLoop <= 30))
				{ iTileUse = iTileLoop - 20; }
			arRoomX[iCurLevel][iCurRoom][iTileLoop] = arRoomXTemp[iTileUse];
			arRoomTiles[iCurLevel][iCurRoom][iTileLoop] =
				arRoomTilesTemp[iTileUse];
			arRoomMod[iCurLevel][iCurRoom][iTileLoop] = arRoomModTemp[iTileUse];
		}

		/*** prince ***/
		if (arStartLocation[iCurLevel][1] == iCurRoom)
		{
			/*** tile ***/
			iTile = arStartLocation[iCurLevel][2];
			if ((iTile >= 1) && (iTile <= 10))
				{ arStartLocation[iCurLevel][2] = iTile + 20; }
			if ((iTile >= 21) && (iTile <= 30))
				{ arStartLocation[iCurLevel][2] = iTile - 20; }
		}

		/*** guard ***/
		if (arGuardTile[iCurLevel][iCurRoom] <= TILES + 1)
		{
			/*** tile ***/
			iTile = arGuardTile[iCurLevel][iCurRoom];
			if ((iTile >= 1) && (iTile <= 10))
				{ arGuardTile[iCurLevel][iCurRoom] = iTile + 20; }
			if ((iTile >= 21) && (iTile <= 30))
				{ arGuardTile[iCurLevel][iCurRoom] = iTile - 20; }
		}
	}
}
/*****************************************************************************/
void CopyPaste (int iAction)
/*****************************************************************************/
{
	/*** Used for looping. ***/
	int iTileLoop;

	if (iAction == 1) /*** copy ***/
	{
		for (iTileLoop = 1; iTileLoop <= TILES; iTileLoop++)
		{
			arCopyPasteX[iTileLoop] = arRoomX[iCurLevel][iCurRoom][iTileLoop];
			arCopyPasteTile[iTileLoop] = arRoomTiles[iCurLevel][iCurRoom][iTileLoop];
			arCopyPasteMod[iTileLoop] = arRoomMod[iCurLevel][iCurRoom][iTileLoop];
		}
		cCopyPasteGuardTile = arGuardTile[iCurLevel][iCurRoom];
		cCopyPasteGuardDir = arGuardDir[iCurLevel][iCurRoom];
		cCopyPasteGuardSkill = arGuardSkill[iCurLevel][iCurRoom];
		cCopyPasteGuardC = arGuardC[iCurLevel][iCurRoom];
		iCopied = 1;
	} else { /*** paste ***/
		if (iCopied == 1)
		{
			for (iTileLoop = 1; iTileLoop <= TILES; iTileLoop++)
			{
				arRoomX[iCurLevel][iCurRoom][iTileLoop] = arCopyPasteX[iTileLoop];
				arRoomTiles[iCurLevel][iCurRoom][iTileLoop] =
					arCopyPasteTile[iTileLoop];
				arRoomMod[iCurLevel][iCurRoom][iTileLoop] = arCopyPasteMod[iTileLoop];
			}
			arGuardTile[iCurLevel][iCurRoom] = cCopyPasteGuardTile;
			arGuardDir[iCurLevel][iCurRoom] = cCopyPasteGuardDir;
			arGuardSkill[iCurLevel][iCurRoom] = cCopyPasteGuardSkill;
			arGuardC[iCurLevel][iCurRoom] = cCopyPasteGuardC;
		} else {
			for (iTileLoop = 1; iTileLoop <= TILES; iTileLoop++)
			{
				arRoomX[iCurLevel][iCurRoom][iTileLoop] = 0;
				arRoomTiles[iCurLevel][iCurRoom][iTileLoop] = 0x00;
				arRoomMod[iCurLevel][iCurRoom][iTileLoop] = 0x00;
			}
			arGuardTile[iCurLevel][iCurRoom] = TILES + 1;
		}
	}
}
/*****************************************************************************/
int InArea (int iUpperLeftX, int iUpperLeftY,
	int iLowerRightX, int iLowerRightY)
/*****************************************************************************/
{
	if ((iUpperLeftX * iScale <= iXPos) &&
		(iLowerRightX * iScale >= iXPos) &&
		(iUpperLeftY * iScale <= iYPos) &&
		(iLowerRightY * iScale >= iYPos))
	{
		return (1);
	} else {
		return (0);
	}
}
/*****************************************************************************/
int MouseSelectAdj (void)
/*****************************************************************************/
{
	/* On the broken room links screen, if you click one of the adjacent
	 * rooms, this function sets iOnAdj to 1, and also sets both
	 * iChangingBrokenRoom and iChangingBrokenSide.
	 */

	int iOnAdj;
	int iRoomLoop;
	int iAdjBaseX;
	int iAdjBaseY;

	iOnAdj = 0;
	for (iRoomLoop = 1; iRoomLoop <= ROOMS; iRoomLoop++)
	{
		switch (iRoomLoop)
		{
			case 1: iAdjBaseX = ADJ_BASE_X + (63 * 0);
				iAdjBaseY = ADJ_BASE_Y + (63 * 0); break;
			case 2: iAdjBaseX = ADJ_BASE_X + (63 * 1);
				iAdjBaseY = ADJ_BASE_Y + (63 * 0); break;
			case 3: iAdjBaseX = ADJ_BASE_X + (63 * 2);
				iAdjBaseY = ADJ_BASE_Y + (63 * 0); break;
			case 4: iAdjBaseX = ADJ_BASE_X + (63 * 3);
				iAdjBaseY = ADJ_BASE_Y + (63 * 0); break;
			case 5: iAdjBaseX = ADJ_BASE_X + (63 * 0);
				iAdjBaseY = ADJ_BASE_Y + (63 * 1); break;
			case 6: iAdjBaseX = ADJ_BASE_X + (63 * 1);
				iAdjBaseY = ADJ_BASE_Y + (63 * 1); break;
			case 7: iAdjBaseX = ADJ_BASE_X + (63 * 2);
				iAdjBaseY = ADJ_BASE_Y + (63 * 1); break;
			case 8: iAdjBaseX = ADJ_BASE_X + (63 * 3);
				iAdjBaseY = ADJ_BASE_Y + (63 * 1); break;
			case 9: iAdjBaseX = ADJ_BASE_X + (63 * 0);
				iAdjBaseY = ADJ_BASE_Y + (63 * 2); break;
			case 10: iAdjBaseX = ADJ_BASE_X + (63 * 1);
				iAdjBaseY = ADJ_BASE_Y + (63 * 2); break;
			case 11: iAdjBaseX = ADJ_BASE_X + (63 * 2);
				iAdjBaseY = ADJ_BASE_Y + (63 * 2); break;
			case 12: iAdjBaseX = ADJ_BASE_X + (63 * 3);
				iAdjBaseY = ADJ_BASE_Y + (63 * 2); break;
			case 13: iAdjBaseX = ADJ_BASE_X + (63 * 0);
				iAdjBaseY = ADJ_BASE_Y + (63 * 3); break;
			case 14: iAdjBaseX = ADJ_BASE_X + (63 * 1);
				iAdjBaseY = ADJ_BASE_Y + (63 * 3); break;
			case 15: iAdjBaseX = ADJ_BASE_X + (63 * 2);
				iAdjBaseY = ADJ_BASE_Y + (63 * 3); break;
			case 16: iAdjBaseX = ADJ_BASE_X + (63 * 3);
				iAdjBaseY = ADJ_BASE_Y + (63 * 3); break;
			case 17: iAdjBaseX = ADJ_BASE_X + (63 * 0);
				iAdjBaseY = ADJ_BASE_Y + (63 * 4); break;
			case 18: iAdjBaseX = ADJ_BASE_X + (63 * 1);
				iAdjBaseY = ADJ_BASE_Y + (63 * 4); break;
			case 19: iAdjBaseX = ADJ_BASE_X + (63 * 2);
				iAdjBaseY = ADJ_BASE_Y + (63 * 4); break;
			case 20: iAdjBaseX = ADJ_BASE_X + (63 * 3);
				iAdjBaseY = ADJ_BASE_Y + (63 * 4); break;
			case 21: iAdjBaseX = ADJ_BASE_X + (63 * 0);
				iAdjBaseY = ADJ_BASE_Y + (63 * 5); break;
			case 22: iAdjBaseX = ADJ_BASE_X + (63 * 1);
				iAdjBaseY = ADJ_BASE_Y + (63 * 5); break;
			case 23: iAdjBaseX = ADJ_BASE_X + (63 * 2);
				iAdjBaseY = ADJ_BASE_Y + (63 * 5); break;
			case 24: iAdjBaseX = ADJ_BASE_X + (63 * 3);
				iAdjBaseY = ADJ_BASE_Y + (63 * 5); break;
			default:
				printf ("[FAILED] iRoomLoop is not in the 1-24 range!\n");
				exit (EXIT_ERROR);
		}
		if (InArea (iAdjBaseX + 1, iAdjBaseY + 16,
			iAdjBaseX + 15, iAdjBaseY + 30) == 1)
		{
			iChangingBrokenRoom = iRoomLoop;
			iChangingBrokenSide = 1; /*** left ***/
			iOnAdj = 1;
		}
		if (InArea (iAdjBaseX + 31, iAdjBaseY + 16,
			iAdjBaseX + 45, iAdjBaseY + 30) == 1)
		{
			iChangingBrokenRoom = iRoomLoop;
			iChangingBrokenSide = 2; /*** right ***/
			iOnAdj = 1;
		}
		if (InArea (iAdjBaseX + 16, iAdjBaseY + 1,
			iAdjBaseX + 30, iAdjBaseY + 14) == 1)
		{
			iChangingBrokenRoom = iRoomLoop;
			iChangingBrokenSide = 3; /*** up ***/
			iOnAdj = 1;
		}
		if (InArea (iAdjBaseX + 16, iAdjBaseY + 31,
			iAdjBaseX + 30, iAdjBaseY + 45) == 1)
		{
			iChangingBrokenRoom = iRoomLoop;
			iChangingBrokenSide = 4; /*** down ***/
			iOnAdj = 1;
		}
	}

	return (iOnAdj);
}
/*****************************************************************************/
int OnLevelBar (void)
/*****************************************************************************/
{
	if (InArea (28, 3, 522, 22) == 1) { return (1); } else { return (0); }
}
/*****************************************************************************/
void ChangePos (void)
/*****************************************************************************/
{
	int iChanging;
	SDL_Event event;
	int iOldXPos, iOldYPos;
	int iUseTile;
	int iNowOn;
	int iSkillChange;

	/*** Used for looping. ***/
	int iRoomLoop;
	int iTileLoop;

	iChanging = 1;
	iCustomHover = 0;
	iGuardType = 0;
	ontile = SDL_GetTicks();

	ShowChange();
	while (iChanging == 1)
	{
		/*** This is for the large preview. ***/
		newticks = SDL_GetTicks();
		if (newticks > oldticks + REFRESH_PROG)
		{
			ShowChange();
			oldticks = newticks;
		}

		while (SDL_PollEvent (&event))
		{
			switch (event.type)
			{
				case SDL_CONTROLLERBUTTONDOWN:
					/*** Nothing for now. ***/
					break;
				case SDL_CONTROLLERBUTTONUP:
					switch (event.cbutton.button)
					{
						case SDL_CONTROLLER_BUTTON_A:
							if (iOnTile != 0)
							{
								UseTile (iOnTile, iSelected, iCurRoom);
								if (iOnTile <= 78) { iChanging = 0; }
								iChanged++;
							}
							break;
						case SDL_CONTROLLER_BUTTON_B:
							iChanging = 0; break;
						case SDL_CONTROLLER_BUTTON_X:
							if (iGuardType < 11) { iGuardType++; }
								else { iGuardType = 0; }
							ApplySkillIfNecessary (iSelected);
							PlaySound ("wav/check_box.wav");
							break;
						case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
							ChangePosAction ("left"); break;
						case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
							ChangePosAction ("right"); break;
						case SDL_CONTROLLER_BUTTON_DPAD_UP:
							ChangePosAction ("up"); break;
						case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
							ChangePosAction ("down"); break;
					}
					ShowChange();
					break;
				case SDL_CONTROLLERAXISMOTION: /*** triggers and analog sticks ***/
					iXJoy1 = SDL_JoystickGetAxis (joystick, 0);
					iYJoy1 = SDL_JoystickGetAxis (joystick, 1);
					iXJoy2 = SDL_JoystickGetAxis (joystick, 3);
					iYJoy2 = SDL_JoystickGetAxis (joystick, 4);
					if ((iXJoy1 < -30000) || (iXJoy2 < -30000)) /*** left ***/
					{
						if ((SDL_GetTicks() - joyleft) > 300)
						{
							ChangeEvent (-1, 1);
							joyleft = SDL_GetTicks();
						}
					}
					if ((iXJoy1 > 30000) || (iXJoy2 > 30000)) /*** right ***/
					{
						if ((SDL_GetTicks() - joyright) > 300)
						{
							ChangeEvent (1, 1);
							joyright = SDL_GetTicks();
						}
					}
					if ((iYJoy1 < -30000) || (iYJoy2 < -30000)) /*** up ***/
					{
						if ((SDL_GetTicks() - joyup) > 300)
						{
							ChangeEvent (10, 1);
							joyup = SDL_GetTicks();
						}
					}
					if ((iYJoy1 > 30000) || (iYJoy2 > 30000)) /*** down ***/
					{
						if ((SDL_GetTicks() - joydown) > 300)
						{
							ChangeEvent (-10, 1);
							joydown = SDL_GetTicks();
						}
					}
					ShowChange();
					break;
				case SDL_KEYDOWN:
					iSkillChange = 0;
					switch (event.key.keysym.sym)
					{
						case SDLK_KP_ENTER:
						case SDLK_RETURN:
						case SDLK_SPACE:
							if (event.key.keysym.mod & KMOD_CTRL)
							{
								if ((iOnTile >= 1) && (iOnTile <= 78))
								{
									for (iRoomLoop = 1; iRoomLoop <= ROOMS; iRoomLoop++)
									{
										for (iTileLoop = 1; iTileLoop <= TILES; iTileLoop++)
											{ UseTile (iOnTile, iTileLoop, iRoomLoop); }
									}
									iChanging = 0;
									iChanged++;
								}
							} else if (event.key.keysym.mod & KMOD_SHIFT) {
								if ((iOnTile >= 1) && (iOnTile <= 78))
								{
									for (iTileLoop = 1; iTileLoop <= TILES; iTileLoop++)
										{ UseTile (iOnTile, iTileLoop, iCurRoom); }
									iChanging = 0;
									iChanged++;
								}
							} else if (iOnTile != 0) {
								UseTile (iOnTile, iSelected, iCurRoom);
								if (iOnTile <= 78) { iChanging = 0; }
								iChanged++;
							}
							break;
						case SDLK_ESCAPE:
						case SDLK_q:
						case SDLK_c:
							iChanging = 0; break;
						case SDLK_0:
						case SDLK_KP_0:
							if (iGuardType != 0) { iGuardType = 0; iSkillChange = 1; }
							break;
						case SDLK_1:
						case SDLK_KP_1:
							if (iGuardType != 1) { iGuardType = 1; iSkillChange = 1; }
							break;
						case SDLK_2:
						case SDLK_KP_2:
							if (iGuardType != 2) { iGuardType = 2; iSkillChange = 1; }
							break;
						case SDLK_3:
						case SDLK_KP_3:
							if (iGuardType != 3) { iGuardType = 3; iSkillChange = 1; }
							break;
						case SDLK_4:
						case SDLK_KP_4:
							if (iGuardType != 4) { iGuardType = 4; iSkillChange = 1; }
							break;
						case SDLK_5:
						case SDLK_KP_5:
							if (iGuardType != 5) { iGuardType = 5; iSkillChange = 1; }
							break;
						case SDLK_6:
						case SDLK_KP_6:
							if (iGuardType != 6) { iGuardType = 6; iSkillChange = 1; }
							break;
						case SDLK_7:
						case SDLK_KP_7:
							if (iGuardType != 7) { iGuardType = 7; iSkillChange = 1; }
							break;
						case SDLK_8:
						case SDLK_KP_8:
							if (iGuardType != 8) { iGuardType = 8; iSkillChange = 1; }
							break;
						case SDLK_9:
						case SDLK_KP_9:
							if (iGuardType != 9) { iGuardType = 9; iSkillChange = 1; }
							break;
						case SDLK_a:
							if (iGuardType != 10) { iGuardType = 10; iSkillChange = 1; }
							break;
						case SDLK_b:
							if (iGuardType != 11) { iGuardType = 11; iSkillChange = 1; }
							break;
						case SDLK_LEFT:
							if (event.key.keysym.mod & KMOD_CTRL)
							{
								ChangeEvent (-10, 1);
							} else if (event.key.keysym.mod & KMOD_SHIFT) {
								ChangeEvent (-1, 1);
							} else {
								ChangePosAction ("left");
							}
							break;
						case SDLK_RIGHT:
							if (event.key.keysym.mod & KMOD_CTRL)
							{
								ChangeEvent (10, 1);
							} else if (event.key.keysym.mod & KMOD_SHIFT) {
								ChangeEvent (1, 1);
							} else {
								ChangePosAction ("right");
							}
							break;
						case SDLK_UP: ChangePosAction ("up"); break;
						case SDLK_DOWN: ChangePosAction ("down"); break;
						default: break;
					}
					if (iSkillChange == 1)
					{
						ApplySkillIfNecessary (iSelected);
						PlaySound ("wav/check_box.wav");
					}
					ShowChange();
					break;
				case SDL_MOUSEMOTION:
					iOldXPos = iXPos;
					iOldYPos = iYPos;
					iXPos = event.motion.x;
					iYPos = event.motion.y;
					if ((iOldXPos == iXPos) && (iOldYPos == iYPos)) { break; }

					/*** custom hover ***/
					iCustomHoverOld = iCustomHover;
					if (InArea (3, 383, 3 + 304, 383 + 43) == 1)
						{ iCustomHover = 1; } else { iCustomHover = 0; }
					if (iCustomHover != iCustomHoverOld) { ShowChange(); }

					if (InArea (311, 427, 311 + 128, 427 + 29) == 1)
					{
						if (iGuardTooltip == 0) { iGuardTooltip = 1; ShowChange(); }
					} else {
						if (iGuardTooltip == 1) { iGuardTooltip = 0; ShowChange(); }
					}

					if (InArea (443, 427, 443 + 128, 427 + 29) == 1)
					{
						if (iEventHover == 0) { iEventHover = 1; ShowChange(); }
					} else {
						if (iEventHover == 1) { iEventHover = 0; ShowChange(); }
					}

					iNowOn = OnTile();
					if ((iOnTile != iNowOn) && (iNowOn != 0))
					{
						if (IsDisabled (iNowOn) == 0)
						{
							iOnTile = iNowOn;
							ShowChange();
						}
					}
					break;
				case SDL_MOUSEBUTTONDOWN:
					if (event.button.button == 1) /*** left mouse button ***/
					{
						if (InArea (576, 0, 576 + 36, 0 + 461) == 1) /*** close ***/
						{
							iCloseOn = 1;
							ShowChange();
						}
					}
					break;
				case SDL_MOUSEBUTTONUP:
					iCloseOn = 0;

					/*** On tile or living. ***/
					iUseTile = 0;
					if (InArea (0, 2, 0 + 574, 2 + 380) == 1) { iUseTile = 1; }

					/*** On the custom tile area. ***/
					if (InArea (3, 383, 3 + 304, 383 + 43) == 1)
					{
						iOnTile = -2;
						iUseTile = 1;
					}

					if (event.button.button == 1) /*** left mouse button ***/
					{
						iSkillChange = 0;
						if (InArea (315, 427, 315 + 14, 427 + 14) == 1)
							if (iGuardType != 0) { iGuardType = 0; iSkillChange = 1; }
						if (InArea (330, 427, 330 + 14, 427 + 14) == 1)
							if (iGuardType != 1) { iGuardType = 1; iSkillChange = 1; }
						if (InArea (345, 427, 345 + 14, 427 + 14) == 1)
							if (iGuardType != 2) { iGuardType = 2; iSkillChange = 1; }
						if (InArea (360, 427, 360 + 14, 427 + 14) == 1)
							if (iGuardType != 3) { iGuardType = 3; iSkillChange = 1; }
						if (InArea (375, 427, 375 + 14, 427 + 14) == 1)
							if (iGuardType != 4) { iGuardType = 4; iSkillChange = 1; }
						if (InArea (390, 427, 390 + 14, 427 + 14) == 1)
							if (iGuardType != 5) { iGuardType = 5; iSkillChange = 1; }
						if (InArea (405, 427, 405 + 14, 427 + 14) == 1)
							if (iGuardType != 6) { iGuardType = 6; iSkillChange = 1; }
						if (InArea (420, 427, 420 + 14, 427 + 14) == 1)
							if (iGuardType != 7) { iGuardType = 7; iSkillChange = 1; }
						if (InArea (345, 442, 345 + 14, 442 + 14) == 1)
							if (iGuardType != 8) { iGuardType = 8; iSkillChange = 1; }
						if (InArea (360, 442, 360 + 14, 442 + 14) == 1)
							if (iGuardType != 9) { iGuardType = 9; iSkillChange = 1; }
						if (InArea (375, 442, 375 + 14, 442 + 14) == 1)
							if (iGuardType != 10) { iGuardType = 10; iSkillChange = 1; }
						if (InArea (390, 442, 390 + 14, 442 + 14) == 1)
							if (iGuardType != 11) { iGuardType = 11; iSkillChange = 1; }
						if (iSkillChange == 1)
						{
							ApplySkillIfNecessary (iSelected);
							PlaySound ("wav/check_box.wav");
						}

						/*** Changing the custom x. ***/
						if (InArea (11, 434, 11 + 14, 434 + 14) == 1)
						{
							if (iCustomX == 1) { iCustomX = 0; } else { iCustomX = 1; }
							PlaySound ("wav/check_box.wav");
						}

						/*** Changing the custom tile. ***/
						if (InArea (57, 431, 57 + 13, 431 + 20) == 1)
							{ ChangeCustom (-16, 1); }
						if (InArea (72, 431, 72 + 13, 431 + 20) == 1)
							{ ChangeCustom (-1, 1); }
						if (InArea (142, 431, 142 + 13, 431 + 20) == 1)
							{ ChangeCustom (1, 1); }
						if (InArea (157, 431, 157 + 13, 431 + 20) == 1)
							{ ChangeCustom (16, 1); }

						/*** Changing the custom mod. ***/
						if (InArea (186, 431, 186 + 13, 431 + 20) == 1)
							{ ChangeCustom (-16, 2); }
						if (InArea (201, 431, 201 + 13, 431 + 20) == 1)
							{ ChangeCustom (-1, 2); }
						if (InArea (271, 431, 271 + 13, 431 + 20) == 1)
							{ ChangeCustom (1, 2); }
						if (InArea (286, 431, 286 + 13, 431 + 20) == 1)
							{ ChangeCustom (16, 2); }

						/*** Changing the event number. ***/
						if (InArea (450, 431, 450 + 13, 431 + 20) == 1)
							{ ChangeEvent (-10, 1); }
						if (InArea (465, 431, 465 + 13, 431 + 20) == 1)
							{ ChangeEvent (-1, 1); }
						if (InArea (535, 431, 535 + 13, 431 + 20) == 1)
							{ ChangeEvent (1, 1); }
						if (InArea (550, 431, 550 + 13, 431 + 20) == 1)
							{ ChangeEvent (10, 1); }

						/*** On close. ***/
						if (InArea (576, 0, 576 + 36, 0 + 461) == 1) { iChanging = 0; }

						if (iUseTile == 1)
						{
							UseTile (iOnTile, iSelected, iCurRoom);
							if (iOnTile <= 78) { iChanging = 0; }
							iChanged++;
						}
					}

					if (event.button.button == 2)
					{
						if ((iUseTile == 1) && (iOnTile != 0) && (iOnTile <= 78))
						{
							for (iTileLoop = 1; iTileLoop <= TILES; iTileLoop++)
							{
								UseTile (iOnTile, iTileLoop, iCurRoom);
							}
							iChanging = 0;
							iChanged++;
						}
					}

					if (event.button.button == 3)
					{
						if ((iUseTile == 1) && (iOnTile != 0) && (iOnTile <= 78))
						{
							for (iRoomLoop = 1; iRoomLoop <= ROOMS; iRoomLoop++)
							{
								for (iTileLoop = 1; iTileLoop <= TILES; iTileLoop++)
								{
									UseTile (iOnTile, iTileLoop, iRoomLoop);
								}
							}
							iChanging = 0;
							iChanged++;
						}
					}

					ShowChange();
					break;
				case SDL_WINDOWEVENT:
					if (event.window.event == SDL_WINDOWEVENT_EXPOSED)
						{ ShowChange(); } break;
				case SDL_QUIT:
					Quit(); break;
			}
		}

		/*** prevent CPU eating ***/
		gamespeed = REFRESH_PROG;
		while ((SDL_GetTicks() - looptime) < gamespeed)
		{
			SDL_Delay (10);
		}
		looptime = SDL_GetTicks();
	}
	PlaySound ("wav/ok_close.wav");
}
/*****************************************************************************/
void RemoveOldRoom (void)
/*****************************************************************************/
{
	arMovingRooms[iMovingOldX][iMovingOldY] = 0;

	/* Change the links of the rooms around
	 * the removed room.
	 */

	/*** left of removed ***/
	if ((iMovingOldX >= 2) && (iMovingOldX <= 24))
	{
		if (arMovingRooms[iMovingOldX - 1][iMovingOldY] != 0)
		{
			arRoomLinks[iCurLevel][arMovingRooms[iMovingOldX - 1]
				[iMovingOldY]][2] = 0; /*** remove right ***/
		}
	}

	/*** right of removed ***/
	if ((iMovingOldX >= 1) && (iMovingOldX <= 23))
	{
		if (arMovingRooms[iMovingOldX + 1][iMovingOldY] != 0)
		{
			arRoomLinks[iCurLevel][arMovingRooms[iMovingOldX + 1]
				[iMovingOldY]][1] = 0; /*** remove left ***/
		}
	}

	/*** above removed ***/
	if ((iMovingOldY >= 2) && (iMovingOldY <= 24))
	{
		if (arMovingRooms[iMovingOldX][iMovingOldY - 1] != 0)
		{
			arRoomLinks[iCurLevel][arMovingRooms[iMovingOldX]
				[iMovingOldY - 1]][4] = 0; /*** remove below ***/
		}
	}

	/*** below removed ***/
	if ((iMovingOldY >= 1) && (iMovingOldY <= 23))
	{
		if (arMovingRooms[iMovingOldX][iMovingOldY + 1] != 0)
		{
			arRoomLinks[iCurLevel][arMovingRooms[iMovingOldX]
				[iMovingOldY + 1]][3] = 0; /*** remove above ***/
		}
	}
}
/*****************************************************************************/
void AddNewRoom (int iX, int iY, int iRoom)
/*****************************************************************************/
{
	arMovingRooms[iX][iY] = iRoom;

	/* Change the links of the rooms around
	 * the new room and the room itself.
	 */

	arRoomLinks[iCurLevel][iRoom][1] = 0;
	arRoomLinks[iCurLevel][iRoom][2] = 0;
	arRoomLinks[iCurLevel][iRoom][3] = 0;
	arRoomLinks[iCurLevel][iRoom][4] = 0;

	if ((iX >= 2) && (iX <= 24)) /*** left of added ***/
	{
		if (arMovingRooms[iX - 1][iY] != 0)
		{
			arRoomLinks[iCurLevel][arMovingRooms[iX - 1]
				[iY]][2] = iRoom; /*** add room right ***/
			arRoomLinks[iCurLevel][iRoom][1] = arMovingRooms[iX - 1][iY];
		}
	}

	if ((iX >= 1) && (iX <= 23)) /*** right of added ***/
	{
		if (arMovingRooms[iX + 1][iY] != 0)
		{
			arRoomLinks[iCurLevel][arMovingRooms[iX + 1]
				[iY]][1] = iRoom; /*** add room left ***/
			arRoomLinks[iCurLevel][iRoom][2] = arMovingRooms[iX + 1][iY];
		}
	}

	if ((iY >= 2) && (iY <= 24)) /*** above added ***/
	{
		if (arMovingRooms[iX][iY - 1] != 0)
		{
			arRoomLinks[iCurLevel][arMovingRooms[iX]
				[iY - 1]][4] = iRoom; /*** add room below ***/
			arRoomLinks[iCurLevel][iRoom][3] = arMovingRooms[iX][iY - 1];
		}
	}

	if ((iY >= 1) && (iY <= 23)) /*** below added ***/
	{
		if (arMovingRooms[iX][iY + 1] != 0)
		{
			arRoomLinks[iCurLevel][arMovingRooms[iX]
				[iY + 1]][3] = iRoom; /*** add room above ***/
			arRoomLinks[iCurLevel][iRoom][4] = arMovingRooms[iX][iY + 1];
		}
	}

	PlaySound ("wav/move_room.wav");
}
/*****************************************************************************/
void LinkPlus (void)
/*****************************************************************************/
{
	int iCurrent, iNew;

	iCurrent = arRoomLinks[iCurLevel][iChangingBrokenRoom][iChangingBrokenSide];
	if (iCurrent == ROOMS) {
		iNew = 0;
	} else {
		iNew = iCurrent + 1;
	}
	arRoomLinks[iCurLevel][iChangingBrokenRoom][iChangingBrokenSide] = iNew;
	iChanged++;
	arBrokenRoomLinks[iCurLevel] = BrokenRoomLinks (0);
	PlaySound ("wav/hum_adj.wav");
}
/*****************************************************************************/
void ShowImage (SDL_Texture *img, int iX, int iY, char *sImageInfo)
/*****************************************************************************/
{
	SDL_Rect dest;
	SDL_Rect loc;
	int iWidth, iHeight;
	int iInfoC;
	char arText[9 + 2][MAX_TEXT + 2];
	char cXValue;
	int iTileValue;
	int iModValue;
	int iGreen;
	char sValueTile[MAX_OPTION + 2];
	char sValueMod[MAX_OPTION + 2];
	int iTileImg;

	iInfoC = 0;

	iGreen = 1;
	if ((strncmp (sImageInfo, "tile=", 5) == 0) ||
		(strncmp (sImageInfo, "high=", 5) == 0))
	{
		if (strncmp (sImageInfo, "tile=", 5) != 0) { iGreen = 2; }
		snprintf (sValueTile, MAX_OPTION, "%c%c", sImageInfo[5], sImageInfo[6]);
		iTileValue = (int)strtol (sValueTile, NULL, 16);
		snprintf (sValueMod, MAX_OPTION, "%c%c", sImageInfo[8], sImageInfo[9]);
		iModValue = (int)strtol (sValueMod, NULL, 16);
		if (iTileValue >= 32)
		{
			iTileValue-=32;
			cXValue = 'Y';
		} else {
			cXValue = 'N';
		}
		iTileImg = iTileValue;
		if (iTileImg == 0x0B)
		{
			if (cXValue == 'Y') { iTileImg+=32; }
		}
		if (imgd[iTileImg][iModValue][1] == NULL)
		{
			img = imgunk[iGreen];
		} else {
			switch (cCurType)
			{
				case 'd': img = imgd[iTileImg][iModValue][iGreen]; break;
				case 'p': img = imgp[iTileImg][iModValue][iGreen]; break;
			}
		}
	} else {
		iTileValue = -1;
		iModValue = -1;
		iTileImg = -1;
		cXValue = 'N';
	}

	if (iInfo == 1)
	{
		snprintf (arText[0], MAX_TEXT, "%c%02X/%02X",
			cXValue, iTileValue, iModValue);
	}

	switch (iTileValue)
	{
		case 0x0F: /*** raise ***/
			switch (cCurType)
			{
				case 'd': img = imgd[0x0F][0x00][iGreen]; break;
				case 'p': img = imgp[0x0F][0x00][iGreen]; break;
			}
			if (iInfo != 1)
				{ snprintf (arText[0], MAX_TEXT, "E:%i", iModValue + 1); }
			iInfoC = 1;
			break;
		case 0x06: /*** drop ***/
			switch (cCurType)
			{
				case 'd': img = imgd[0x06][0x00][iGreen]; break;
				case 'p': img = imgp[0x06][0x00][iGreen]; break;
			}
			if (iInfo != 1)
				{ snprintf (arText[0], MAX_TEXT, "E:%i", iModValue + 1); }
			iInfoC = 1;
			break;
	}

	/*** Custom tile. ***/
	if ((iInfoC == 0) && (imgd[iTileImg][iModValue][1] == NULL))
	{
		snprintf (arText[0], MAX_TEXT, "%c%02X/%02X",
			cXValue, iTileValue, iModValue);
		iInfoC = 1;
	}

	if ((iNoAnim == 0) && (iGreen == 1) &&
		(iTileValue == 0x13) && (iModValue == 0x00))
	{
		switch (cCurType)
		{
			case 'd': img = imgspriteflamed; break;
			case 'p': img = imgspriteflamep; break;
		}
	}
	SDL_QueryTexture (img, NULL, NULL, &iWidth, &iHeight);
	loc.x = 0;
	loc.y = 0;
	loc.w = iWidth;
	loc.h = iHeight;
	dest.x = iX;
	dest.y = iY;
	dest.w = iWidth;
	dest.h = iHeight;
	/*** This is for the game animation. ***/
	if (iNoAnim == 0)
	{
		if (newticks > oldticks + REFRESH_GAME)
		{
			/*** 1-9 ***/
			iFlameFrame = 1 + (int) (9.0 * rand() / (RAND_MAX + 1.0));
		}
		if (iGreen == 1)
		{
			if ((iTileValue == 0x13) && (iModValue == 0x00))
			{
				loc.x = (iFlameFrame - 1) * 117;
				loc.w = loc.w / 9;
				dest.w = dest.w / 9;
			}
		}
	}
	CustomRenderCopy (img, &loc, &dest, sImageInfo);

	/*** Info ("i"). ***/
	if ((iTileValue != -1) && ((iInfo == 1) || (iInfoC == 1)))
	{
		DisplayText (dest.x, dest.y + 136 - FONT_SIZE_11,
			FONT_SIZE_11, arText, 1, font2);
	}
}
/*****************************************************************************/
void CustomRenderCopy (SDL_Texture* src, SDL_Rect* srcrect,
	SDL_Rect *dstrect, char *sImageInfo)
/*****************************************************************************/
{
	SDL_Rect stuff;

	stuff.x = dstrect->x * iScale;
	stuff.y = dstrect->y * iScale;
	if (srcrect != NULL) /*** image ***/
	{
		stuff.w = dstrect->w * iScale;
		stuff.h = dstrect->h * iScale;
	} else { /*** font ***/
		stuff.w = dstrect->w;
		stuff.h = dstrect->h;
	}
	if (SDL_RenderCopy (ascreen, src, srcrect, &stuff) != 0)
	{
		printf ("[ WARN ] SDL_RenderCopy (%s): %s!\n",
			sImageInfo, SDL_GetError());
	}
}
/*****************************************************************************/
void CreateBAK (void)
/*****************************************************************************/
{
	FILE *fDAT;
	FILE *fBAK;
	int iData;

	fDAT = fopen (sPathFile, "rb");
	if (fDAT == NULL)
		{ printf ("[FAILED] Could not open %s: %s!\n",
			sPathFile, strerror (errno)); }

	switch (iHomeComputer)
	{
		case 1: fBAK = fopen (BACKUP_A, "wb"); break;
		case 2: fBAK = fopen (BACKUP_B, "wb"); break;
		case 3: fBAK = fopen (BACKUP_C, "wb"); break;
		default: printf ("[FAILED] iHomeComputer!\n"); exit (EXIT_ERROR); break;
	}
	if (fBAK == NULL)
		{ printf ("[FAILED] Could not open backup: %s!\n", strerror (errno)); }

	while (1)
	{
		iData = fgetc (fDAT);
		if (iData == EOF) { break; }
			else { putc (iData, fBAK); }
	}

	fclose (fDAT);
	fclose (fBAK);
}
/*****************************************************************************/
void DisplayText (int iStartX, int iStartY, int iFontSize,
	char arText[9 + 2][MAX_TEXT + 2], int iLines, TTF_Font *font)
/*****************************************************************************/
{
	int iTemp;

	for (iTemp = 0; iTemp <= (iLines - 1); iTemp++)
	{
		if (strcmp (arText[iTemp], "") != 0)
		{
			message = TTF_RenderText_Shaded (font,
				arText[iTemp], color_bl, color_wh);
			messaget = SDL_CreateTextureFromSurface (ascreen, message);
			if ((strcmp (arText[iTemp], "single tile (change or select)") == 0) ||
				(strcmp (arText[iTemp], "entire room (clear or fill)") == 0) ||
				(strcmp (arText[iTemp], "entire level (randomize or fill)") == 0))
			{
				offset.x = iStartX + 20;
			} else {
				offset.x = iStartX;
			}
			offset.y = iStartY + (iTemp * (iFontSize + 4));
			offset.w = message->w; offset.h = message->h;
			CustomRenderCopy (messaget, NULL, &offset, "message");
			SDL_DestroyTexture (messaget); SDL_FreeSurface (message);
		}
	}
}
/*****************************************************************************/
void InitRooms (void)
/*****************************************************************************/
{
	int iRoomLoop;
	int iRoomLoop2;

	for (iRoomLoop = 1; iRoomLoop <= ROOMS + 1; iRoomLoop++) /*** x ***/
	{
		for (iRoomLoop2 = 1; iRoomLoop2 <= ROOMS; iRoomLoop2++) /*** y ***/
		{
			arMovingRooms[iRoomLoop][iRoomLoop2] = 0;
		}
	}
}
/*****************************************************************************/
void WhereToStart (void)
/*****************************************************************************/
{
	int iRoomLoop;

	iMinX = 0;
	iMaxX = 0;
	iMinY = 0;
	iMaxY = 0;

	for (iRoomLoop = 1; iRoomLoop <= ROOMS; iRoomLoop++)
	{
		arDone[iRoomLoop] = 0;
	}
	CheckSides (arStartLocation[iCurLevel][1], 0, 0);

	iStartRoomsX = round (12 - (((float)iMinX + (float)iMaxX) / 2));
	iStartRoomsY = round (12 - (((float)iMinY + (float)iMaxY) / 2));
}
/*****************************************************************************/
void CheckSides (int iRoom, int iX, int iY)
/*****************************************************************************/
{
	if (iX < iMinX) { iMinX = iX; }
	if (iY < iMinY) { iMinY = iY; }
	if (iX > iMaxX) { iMaxX = iX; }
	if (iY > iMaxY) { iMaxY = iY; }

	arDone[iRoom] = 1;

	if ((arRoomLinks[iCurLevel][iRoom][1] != 0) &&
		(arDone[arRoomLinks[iCurLevel][iRoom][1]] != 1))
		{ CheckSides (arRoomLinks[iCurLevel][iRoom][1], iX - 1, iY); }

	if ((arRoomLinks[iCurLevel][iRoom][2] != 0) &&
		(arDone[arRoomLinks[iCurLevel][iRoom][2]] != 1))
		{ CheckSides (arRoomLinks[iCurLevel][iRoom][2], iX + 1, iY); }

	if ((arRoomLinks[iCurLevel][iRoom][3] != 0) &&
		(arDone[arRoomLinks[iCurLevel][iRoom][3]] != 1))
		{ CheckSides (arRoomLinks[iCurLevel][iRoom][3], iX, iY - 1); }

	if ((arRoomLinks[iCurLevel][iRoom][4] != 0) &&
		(arDone[arRoomLinks[iCurLevel][iRoom][4]] != 1))
		{ CheckSides (arRoomLinks[iCurLevel][iRoom][4], iX, iY + 1); }
}
/*****************************************************************************/
void ShowRooms (int iRoom, int iX, int iY, int iNext)
/*****************************************************************************/
{
	int iShowX, iShowY;

	if (iX == 25) /*** side pane ***/
	{
		iShowX = 189;
	} else {
		iShowX = 199 + (iX * 15); /*** grid, 24x24 ***/
	}
	iShowY = 49 + (iY * 15); /*** grid, 24x24 & pane ***/

	if (iRoom != -1)
	{
		ShowImage (imgroom[iRoom], iShowX, iShowY, "imgroom[...]");
		arMovingRooms[iX][iY] = iRoom; /*** save room location ***/
		if (iCurRoom == iRoom)
		{
			ShowImage (imgsrc, iShowX, iShowY, "imgsrc"); /*** green stripes ***/
		}
		if (arStartLocation[iCurLevel][1] == iRoom)
		{
			ShowImage (imgsrs, iShowX, iShowY, "imgsrs"); /*** blue border ***/
		}
		if (iMovingRoom == iRoom)
		{
			ShowImage (imgsrm, iShowX, iShowY, "imgsrm"); /*** red stripes ***/
		}
	} else {
		ShowImage (imgsrp, iShowX, iShowY, "imgsrp"); /*** white cross ***/
	}
	if (iRoom == iMovingRoom)
	{
		iMovingOldX = iX;
		iMovingOldY = iY;
		if (iMovingNewBusy == 0)
		{
			iMovingNewX = iMovingOldX;
			iMovingNewY = iMovingOldY;
			iMovingNewBusy = 1;
		}
	}

	arDone[iRoom] = 1;

	if (iNext == 1)
	{
		if ((arRoomLinks[iCurLevel][iRoom][1] != 0) &&
			(arDone[arRoomLinks[iCurLevel][iRoom][1]] != 1))
			{ ShowRooms (arRoomLinks[iCurLevel][iRoom][1], iX - 1, iY, 1); }

		if ((arRoomLinks[iCurLevel][iRoom][2] != 0) &&
			(arDone[arRoomLinks[iCurLevel][iRoom][2]] != 1))
			{ ShowRooms (arRoomLinks[iCurLevel][iRoom][2], iX + 1, iY, 1); }

		if ((arRoomLinks[iCurLevel][iRoom][3] != 0) &&
			(arDone[arRoomLinks[iCurLevel][iRoom][3]] != 1))
			{ ShowRooms (arRoomLinks[iCurLevel][iRoom][3], iX, iY - 1, 1); }

		if ((arRoomLinks[iCurLevel][iRoom][4] != 0) &&
			(arDone[arRoomLinks[iCurLevel][iRoom][4]] != 1))
			{ ShowRooms (arRoomLinks[iCurLevel][iRoom][4], iX, iY + 1, 1); }
	}
}
/*****************************************************************************/
void BrokenRoomChange (int iRoom, int iSide, int *iX, int *iY)
/*****************************************************************************/
{
	switch (iSide)
	{
		case 0:
			*iX = BROKEN_ROOM_X;
			*iY = BROKEN_ROOM_Y;
			break;
		case 1:
			*iX = BROKEN_LEFT_X;
			*iY = BROKEN_LEFT_Y;
			break;
		case 2:
			*iX = BROKEN_RIGHT_X;
			*iY = BROKEN_RIGHT_Y;
			break;
		case 3:
			*iX = BROKEN_UP_X;
			*iY = BROKEN_UP_Y;
			break;
		case 4:
			*iX = BROKEN_DOWN_X;
			*iY = BROKEN_DOWN_Y;
			break;
	}

	switch (iRoom)
	{
		case 1: *iX += (63 * 0); *iY += (63 * 0); break;
		case 2: *iX += (63 * 1); *iY += (63 * 0); break;
		case 3: *iX += (63 * 2); *iY += (63 * 0); break;
		case 4: *iX += (63 * 3); *iY += (63 * 0); break;
		case 5: *iX += (63 * 0); *iY += (63 * 1); break;
		case 6: *iX += (63 * 1); *iY += (63 * 1); break;
		case 7: *iX += (63 * 2); *iY += (63 * 1); break;
		case 8: *iX += (63 * 3); *iY += (63 * 1); break;
		case 9: *iX += (63 * 0); *iY += (63 * 2); break;
		case 10: *iX += (63 * 1); *iY += (63 * 2); break;
		case 11: *iX += (63 * 2); *iY += (63 * 2); break;
		case 12: *iX += (63 * 3); *iY += (63 * 2); break;
		case 13: *iX += (63 * 0); *iY += (63 * 3); break;
		case 14: *iX += (63 * 1); *iY += (63 * 3); break;
		case 15: *iX += (63 * 2); *iY += (63 * 3); break;
		case 16: *iX += (63 * 3); *iY += (63 * 3); break;
		case 17: *iX += (63 * 0); *iY += (63 * 4); break;
		case 18: *iX += (63 * 1); *iY += (63 * 4); break;
		case 19: *iX += (63 * 2); *iY += (63 * 4); break;
		case 20: *iX += (63 * 3); *iY += (63 * 4); break;
		case 21: *iX += (63 * 0); *iY += (63 * 5); break;
		case 22: *iX += (63 * 1); *iY += (63 * 5); break;
		case 23: *iX += (63 * 2); *iY += (63 * 5); break;
		case 24: *iX += (63 * 3); *iY += (63 * 5); break;
	}
}
/*****************************************************************************/
void ShowChange (void)
/*****************************************************************************/
{
	int iX, iY;
	int iOldTile, iOldM;
	int iGetTile, iGetMod;

	/*** background ***/
	switch (cCurType)
	{
		case 'd': ShowImage (imgdungeon, 0, 0, "imgdungeon"); break;
		case 'p': ShowImage (imgpalace, 0, 0, "imgpalace"); break;
	}

	/*** close button ***/
	switch (iCloseOn)
	{
		case 0: /*** off ***/
			ShowImage (imgclosebig_0, 576, 0, "imgclosebig_0"); break;
		case 1: /*** on ***/
			ShowImage (imgclosebig_1, 576, 0, "imgclosebig_1"); break;
	}

	DisableSome();

	/*** old tile ***/
	iOldTile = arRoomTiles[iCurLevel][iCurRoom][iSelected];
	if (iOldTile == 0x0B)
	{
		if (arRoomX[iCurLevel][iCurRoom][iSelected] == 1)
			{ iOldTile+=32; }
	}
	iOldM = arRoomMod[iCurLevel][iCurRoom][iSelected];
	if ((iOldTile == 0x06) || (iOldTile == 0x0F)) /*** drop or raise ***/
		{ iChangeEvent = iOldM + 1; }
	iX = -1; iY = -1;

	/*** Row 1. ***/
	if ((iOldTile == 0x00) && (iOldM == 0x00)) { iY = TILESY1; iX = TILESX1; }
	if ((iOldTile == 0x00) && (iOldM == 0x01)) { iY = TILESY1; iX = TILESX2; }
	if ((iOldTile == 0x00) && (iOldM == 0x02)) { iY = TILESY1; iX = TILESX3; }
	if ((iOldTile == 0x00) && (iOldM == 0x03)) { iY = TILESY1; iX = TILESX4; }
	if ((iOldTile == 0x00) && (iOldM == 0xFF)) { iY = TILESY1; iX = TILESX5; }
	if ((iOldTile == 0x01) && (iOldM == 0x00)) { iY = TILESY1; iX = TILESX6; }
	if ((iOldTile == 0x01) && (iOldM == 0x01)) { iY = TILESY1; iX = TILESX7; }
	if ((iOldTile == 0x01) && (iOldM == 0x02)) { iY = TILESY1; iX = TILESX8; }
	if ((iOldTile == 0x01) && (iOldM == 0x03)) { iY = TILESY1; iX = TILESX9; }
	if ((iOldTile == 0x01) && (iOldM == 0xFF)) { iY = TILESY1; iX = TILESX10; }
	if ((iOldTile == 0x02) && (iOldM == 0x00)) { iY = TILESY1; iX = TILESX11; }
	if ((iOldTile == 0x02) && (iOldM == 0x01)) { iY = TILESY1; iX = TILESX12; }
	if ((iOldTile == 0x02) && (iOldM == 0x02)) { iY = TILESY1; iX = TILESX13; }

	/*** Row 2. ***/
	if ((iOldTile == 0x02) && (iOldM == 0x03)) { iY = TILESY2; iX = TILESX1; }
	if ((iOldTile == 0x02) && (iOldM == 0x04)) { iY = TILESY2; iX = TILESX2; }
	if ((iOldTile == 0x02) && (iOldM == 0x05)) { iY = TILESY2; iX = TILESX3; }
	if ((iOldTile == 0x02) && (iOldM == 0x06)) { iY = TILESY2; iX = TILESX4; }
	if ((iOldTile == 0x02) && (iOldM == 0x07)) { iY = TILESY2; iX = TILESX5; }
	if ((iOldTile == 0x02) && (iOldM == 0x08)) { iY = TILESY2; iX = TILESX6; }
	if ((iOldTile == 0x02) && (iOldM == 0x09)) { iY = TILESY2; iX = TILESX7; }
	if ((iOldTile == 0x03) && (iOldM == 0x00)) { iY = TILESY2; iX = TILESX8; }
	if ((iOldTile == 0x04) && (iOldM == 0x02)) { iY = TILESY2; iX = TILESX9; }
	if ((iOldTile == 0x04) && (iOldM == 0x01)) { iY = TILESY2; iX = TILESX10; }
	if ((iOldTile == 0x05) && (iOldM == 0x00)) { iY = TILESY2; iX = TILESX11; }
	if (iOldTile == 0x06) { iY = TILESY2; iX = TILESX12; }
	if ((iOldTile == 0x07) && (iOldM == 0x00)) { iY = TILESY2; iX = TILESX13; }

	/*** Row 3. ***/
	if ((iOldTile == 0x07) && (iOldM == 0x01)) { iY = TILESY3; iX = TILESX1; }
	if ((iOldTile == 0x07) && (iOldM == 0x02)) { iY = TILESY3; iX = TILESX2; }
	if ((iOldTile == 0x07) && (iOldM == 0x03)) { iY = TILESY3; iX = TILESX3; }
	if ((iOldTile == 0x08) && (iOldM == 0x00)) { iY = TILESY3; iX = TILESX4; }
	if ((iOldTile == 0x09) && (iOldM == 0x00)) { iY = TILESY3; iX = TILESX5; }
	if ((iOldTile == 0x0A) && (iOldM == 0x00)) { iY = TILESY3; iX = TILESX6; }
	if ((iOldTile == 0x0A) && (iOldM == 0x01)) { iY = TILESY3; iX = TILESX7; }
	if ((iOldTile == 0x0A) && (iOldM == 0x02)) { iY = TILESY3; iX = TILESX8; }
	if ((iOldTile == 0x0A) && (iOldM == 0x03)) { iY = TILESY3; iX = TILESX9; }
	if ((iOldTile == 0x0A) && (iOldM == 0x04)) { iY = TILESY3; iX = TILESX10; }
	if ((iOldTile == 0x0A) && (iOldM == 0x05)) { iY = TILESY3; iX = TILESX11; }
	if ((iOldTile == 0x0A) && (iOldM == 0x06)) { iY = TILESY3; iX = TILESX12; }
	if ((iOldTile == 0x0B) && (iOldM == 0x00)) { iY = TILESY3; iX = TILESX13; }

	/*** Row 4. ***/
	if ((iOldTile == 0x0C) && (iOldM == 0x00)) { iY = TILESY4; iX = TILESX1; }
	if ((iOldTile == 0x0C) && (iOldM == 0x01)) { iY = TILESY4; iX = TILESX2; }
	if ((iOldTile == 0x0C) && (iOldM == 0x02)) { iY = TILESY4; iX = TILESX3; }
	if ((iOldTile == 0x0C) && (iOldM == 0x03)) { iY = TILESY4; iX = TILESX4; }
	if ((iOldTile == 0x0C) && (iOldM == 0x04)) { iY = TILESY4; iX = TILESX5; }
	if ((iOldTile == 0x0C) && (iOldM == 0x05)) { iY = TILESY4; iX = TILESX6; }
	if ((iOldTile == 0x0C) && (iOldM == 0x06)) { iY = TILESY4; iX = TILESX7; }
	if ((iOldTile == 0x0C) && (iOldM == 0x07)) { iY = TILESY4; iX = TILESX8; }
	if ((iOldTile == 0x0D) && (iOldM == 0x00)) { iY = TILESY4; iX = TILESX9; }
	if ((iOldTile == 0x0E) && (iOldM == 0x00)) { iY = TILESY4; iX = TILESX10; }
	if (iOldTile == 0x0F) { iY = TILESY4; iX = TILESX11; }
	if ((iOldTile == 0x10) && (iOldM == 0x00)) { iY = TILESY4; iX = TILESX12; }
	if ((iOldTile == 0x11) && (iOldM == 0x00)) { iY = TILESY4; iX = TILESX13; }

	/*** Row 5. ***/
	if ((iOldTile == 0x12) && (iOldM == 0x00)) { iY = TILESY5; iX = TILESX1; }
	if ((iOldTile == 0x12) && (iOldM == 0x01)) { iY = TILESY5; iX = TILESX2; }
	if ((iOldTile == 0x12) && (iOldM == 0x02)) { iY = TILESY5; iX = TILESX3; }
	if ((iOldTile == 0x12) && (iOldM == 0x03)) { iY = TILESY5; iX = TILESX4; }
	if ((iOldTile == 0x12) && (iOldM == 0x04)) { iY = TILESY5; iX = TILESX5; }
	if ((iOldTile == 0x12) && (iOldM == 0x05)) { iY = TILESY5; iX = TILESX6; }
	if ((iOldTile == 0x13) && (iOldM == 0x00)) { iY = TILESY5; iX = TILESX7; }
	if ((iOldTile == 0x14) && (iOldM == 0x00)) { iY = TILESY5; iX = TILESX8; }
	if ((iOldTile == 0x14) && (iOldM == 0x01)) { iY = TILESY5; iX = TILESX9; }
	if ((iOldTile == 0x15) && (iOldM == 0x00)) { iY = TILESY5; iX = TILESX10; }
	if ((iOldTile == 0x16) && (iOldM == 0x00)) { iY = TILESY5; iX = TILESX11; }
	if ((iOldTile == 0x17) && (iOldM == 0x00)) { iY = TILESY5; iX = TILESX12; }
	if ((iOldTile == 0x18) && (iOldM == 0x00)) { iY = TILESY5; iX = TILESX13; }

	/*** Row 6. ***/
	if ((iOldTile == 0x19) && (iOldM == 0x00)) { iY = TILESY6; iX = TILESX1; }
	if ((iOldTile == 0x1A) && (iOldM == 0x00)) { iY = TILESY6; iX = TILESX2; }
	if ((iOldTile == 0x1B) && (iOldM == 0x00)) { iY = TILESY6; iX = TILESX3; }
	if ((iOldTile == 0x1C) && (iOldM == 0x00)) { iY = TILESY6; iX = TILESX4; }
	if ((iOldTile == 0x1D) && (iOldM == 0x00)) { iY = TILESY6; iX = TILESX5; }
	if ((iOldTile == 0x1E) && (iOldM == 0x00)) { iY = TILESY6; iX = TILESX6; }
	if ((iOldTile == 0x2B) && (iOldM == 0x00)) { iY = TILESY6; iX = TILESX7; }
	if ((iOldTile == 0x12) && (iOldM == 0x80)) { iY = TILESY6; iX = TILESX8; }
	if ((iOldTile == 0x12) && (iOldM == 0x81)) { iY = TILESY6; iX = TILESX9; }
	if ((iOldTile == 0x12) && (iOldM == 0x82)) { iY = TILESY6; iX = TILESX10; }
	if ((iOldTile == 0x12) && (iOldM == 0x83)) { iY = TILESY6; iX = TILESX11; }
	if ((iOldTile == 0x12) && (iOldM == 0x84)) { iY = TILESY6; iX = TILESX12; }
	if ((iOldTile == 0x12) && (iOldM == 0x85)) { iY = TILESY6; iX = TILESX13; }

	if ((iX != -1) && (iY != -1))
	{
		ShowImage (imgborderbl, iX, iY, "imgborderbl");
	}

	/*** prince ***/
	if ((iCurRoom == arStartLocation[iCurLevel][1]) &&
		(iSelected == arStartLocation[iCurLevel][2]))
	{
		switch (arStartLocation[iCurLevel][3])
		{
			case 0x00: /*** r ***/
				ShowImage (imgbordersl, 0, 326, "imgbordersl"); break;
			case 0xFF: /*** l ***/
				ShowImage (imgbordersl, 22, 326, "imgbordersl"); break;
		}
	}

	/*** guard ***/
	if (iSelected == arGuardTile[iCurLevel][iCurRoom])
	{
		iGuardType = arGuardSkill[iCurLevel][iCurRoom];
		iY = 326;
		switch (arGuardDir[iCurLevel][iCurRoom])
		{
			case 0x00: /*** r ***/
				switch (iCurGuard)
				{
					case 0x01: iX = 88; break; /*** skeleton ***/
					case 0x03: iX = 132; break; /*** fat ***/
					case 0x04: iX = 176; break; /*** shadow ***/
					case 0x05: iX = 220; break; /*** Jaffar ***/
					default: iX = 44; break; /*** guard; 0x00 and 0x02 ***/
				}
				break;
			case 0xFF: /*** l ***/
				switch (iCurGuard)
				{
					case 0x01: iX = 110; break; /*** skeleton ***/
					case 0x03: iX = 154; break; /*** fat ***/
					case 0x04: iX = 198; break; /*** shadow ***/
					case 0x05: iX = 242; break; /*** Jaffar ***/
					default: iX = 66; break; /*** guard; 0x00 and 0x02 ***/
				}
				break;
		}
		ShowImage (imgbordersl, iX, iY, "imgbordersl");
	}

	/*** selected (new) tile ***/
	if (iOnTile != iOnTileOld)
	{
		ontile = SDL_GetTicks();
		iOnTileOld = iOnTile;
	}
	if ((iOnTile != 0) && (IsDisabled (iOnTile) == 0))
	{
		if (iOnTile <= 78) /*** (large) tiles ***/
		{
			iX = (((iOnTile - 1) % 13) * (TILEWIDTH + 2));
			iY = 2 + (((int)((iOnTile - 1) / 13)) * (TILEHEIGHT + 2));
			ShowImage (imgborderb, iX, iY, "imgborderb");

			/*** large preview ***/
			if ((SDL_GetTicks() > ontile + 1000) && (iEventHover == 0))
			{
				GetTileModChange (iOnTile, &iGetTile, &iGetMod);
				snprintf (sInfo, MAX_INFO, "tile=%02x_%02x", iGetTile, iGetMod);
				if (((iOnTile >= 1) && (iOnTile <= 7)) ||
					((iOnTile >= 14) && (iOnTile <= 20)) ||
					((iOnTile >= 27) && (iOnTile <= 33)) ||
					((iOnTile >= 40) && (iOnTile <= 46)) ||
					((iOnTile >= 53) && (iOnTile <= 59)) ||
					((iOnTile >= 66) && (iOnTile <= 72)))
				{ /*** show right ***/
					ShowImage (imgpreviewb, 356, 76, "imgpreviewb");
					ShowImage (NULL, 371, 91, sInfo);
				} else { /*** show left ***/
					ShowImage (imgpreviewb, 71, 76, "imgpreviewb");
					ShowImage (NULL, 86, 91, sInfo);
				}
			}
		} else { /*** living ***/
			iX = 0 + (22 * (iOnTile - 79));
			iY = 326;
			ShowImage (imgborders, iX, iY, "imgborders");
		}
	}

	/*** guard skill ***/
	switch (iGuardType)
	{
		case 0: iX = 315; iY = 427; break;
		case 1: iX = 330; iY = 427; break;
		case 2: iX = 345; iY = 427; break;
		case 3: iX = 360; iY = 427; break;
		case 4: iX = 375; iY = 427; break;
		case 5: iX = 390; iY = 427; break;
		case 6: iX = 405; iY = 427; break;
		case 7: iX = 420; iY = 427; break;
		case 8: iX = 345; iY = 442; break;
		case 9: iX = 360; iY = 442; break;
		case 10: iX = 375; iY = 442; break;
		case 11: iX = 390; iY = 442; break;
	}
	ShowImage (imgsell, iX, iY, "imgsell");

	/*** custom tile ***/
	if (iCustomX == 1) { ShowImage (imgsell, 11, 434, "imgsell"); }
	CenterNumber (iCustomTile, 85, 431, color_bl, 1);
	CenterNumber (iCustomMod, 214, 431, color_bl, 1);

	/*** event number ***/
	CenterNumber (iChangeEvent, 478, 431, color_bl, 0);

	if (iCustomHover == 1)
		{ ShowImage (imgchover, 3, 383, "imgchover"); }

	if (iGuardTooltip == 1)
		{ ShowImage (imgtooltipg, 311, 383, "imgtooltipg"); }

	if (iEventHover == 1)
		{ ShowImage (imgeventh, 440, 56, "imgeventh"); }

	/*** refresh screen ***/
	SDL_RenderPresent (ascreen);
}
/*****************************************************************************/
int OnTile (void)
/*****************************************************************************/
{
	int iTempX;
	int iTempY;
	int iTempOn;

	/*** (large) tiles ***/
	for (iTempY = 0; iTempY < 6; iTempY++)
	{
		for (iTempX = 0; iTempX < 13; iTempX++)
		{
			if (InArea (2 + (iTempX * (TILEWIDTH + 2)),
				4 + (iTempY * (TILEHEIGHT + 2)),
				2 + (iTempX * (TILEWIDTH + 2)) + TILEWIDTH,
				4 + (iTempY * (TILEHEIGHT + 2)) + TILEHEIGHT) == 1)
			{
				iTempOn = (iTempY * 13) + iTempX + 1;
				if ((iTempOn >= 1) && (iTempOn <= 78)) { return (iTempOn); }
			}
		}
	}

	/*** living ***/
	for (iTempX = 0; iTempX < 26; iTempX++)
	{
		if (InArea (2 + (22 * iTempX), 328,
			2 + (22 * iTempX) + 20, 328 + 52) == 1)
		{ return (79 + iTempX); }
	}

	return (0);
}
/*****************************************************************************/
void ChangePosAction (char *sAction)
/*****************************************************************************/
{
	if (strcmp (sAction, "left") == 0)
	{
		do {
			switch (iOnTile)
			{
				case 1: iOnTile = 13; break;
				case 14: iOnTile = 26; break;
				case 27: iOnTile = 39; break;
				case 40: iOnTile = 52; break;
				case 53: iOnTile = 65; break;
				case 66: iOnTile = 78; break;
				case 79: iOnTile = 104; break;
				default: iOnTile--; break;
			}
		} while (IsDisabled (iOnTile) == 1);
	}

	if (strcmp (sAction, "right") == 0)
	{
		do {
			switch (iOnTile)
			{
				case 13: iOnTile = 1; break;
				case 26: iOnTile = 14; break;
				case 39: iOnTile = 27; break;
				case 52: iOnTile = 40; break;
				case 65: iOnTile = 53; break;
				case 78: iOnTile = 66; break;
				case 104: iOnTile = 79; break;
				default: iOnTile++; break;
			}
		} while (IsDisabled (iOnTile) == 1);
	}

	if (strcmp (sAction, "up") == 0)
	{
		do {
			switch (iOnTile)
			{
				case 1: iOnTile = 79; break;
				case 2: iOnTile = 81; break;
				case 3: iOnTile = 83; break;
				case 4: iOnTile = 85; break;
				case 5: iOnTile = 87; break;
				case 6: iOnTile = 89; break;
				case 7: iOnTile = 91; break;
				case 8: iOnTile = 93; break;
				case 9: iOnTile = 95; break;
				case 10: iOnTile = 97; break;
				case 11: iOnTile = 99; break;
				case 12: iOnTile = 101; break;
				case 13: iOnTile = 103; break;
				case 79: case 80: iOnTile = 66; break;
				case 81: case 82: iOnTile = 67; break;
				case 83: case 84: iOnTile = 68; break;
				case 85: case 86: iOnTile = 69; break;
				case 87: case 88: iOnTile = 70; break;
				case 89: case 90: iOnTile = 71; break;
				case 91: case 92: iOnTile = 72; break;
				case 93: case 94: iOnTile = 73; break;
				case 95: case 96: iOnTile = 74; break;
				case 97: case 98: iOnTile = 75; break;
				case 99: case 100: iOnTile = 76; break;
				case 101: case 102: iOnTile = 77; break;
				case 103: case 104: iOnTile = 78; break;
				default: iOnTile-=13; break;
			}
		} while (IsDisabled (iOnTile) == 1);
	}

	if (strcmp (sAction, "down") == 0)
	{
		do {
			switch (iOnTile)
			{
				case 66: iOnTile = 79; break;
				case 67: iOnTile = 81; break;
				case 68: iOnTile = 83; break;
				case 69: iOnTile = 85; break;
				case 70: iOnTile = 87; break;
				case 71: iOnTile = 89; break;
				case 72: iOnTile = 91; break;
				case 73: iOnTile = 93; break;
				case 74: iOnTile = 95; break;
				case 75: iOnTile = 97; break;
				case 76: iOnTile = 99; break;
				case 77: iOnTile = 101; break;
				case 78: iOnTile = 103; break;
				case 79: case 80: iOnTile = 1; break;
				case 81: case 82: iOnTile = 2; break;
				case 83: case 84: iOnTile = 3; break;
				case 85: case 86: iOnTile = 4; break;
				case 87: case 88: iOnTile = 5; break;
				case 89: case 90: iOnTile = 6; break;
				case 91: case 92: iOnTile = 7; break;
				case 93: case 94: iOnTile = 8; break;
				case 95: case 96: iOnTile = 9; break;
				case 97: case 98: iOnTile = 10; break;
				case 99: case 100: iOnTile = 11; break;
				case 101: case 102: iOnTile = 12; break;
				case 103: case 104: iOnTile = 13; break;
				default: iOnTile+=13; break;
			}
		} while (IsDisabled (iOnTile) == 1);
	}
}
/*****************************************************************************/
void DisableSome (void)
/*****************************************************************************/
{
	if (iCurGuard != 0x01)
	{
		/*** disable skeleton ***/
		ShowImage (imgdisabled, 88, 326, "imgdisabled");
	}

	if (iCurGuard != 0x03)
	{
		/*** disable fat ***/
		ShowImage (imgdisabled, 132, 326, "imgdisabled");
	}

	if (iCurGuard != 0x04)
	{
		/*** disable shadow ***/
		ShowImage (imgdisabled, 176, 326, "imgdisabled");
	}

	if (iCurGuard != 0x05)
	{
		/*** disable Jaffar ***/
		ShowImage (imgdisabled, 220, 326, "imgdisabled");
	}

	if (((iCurGuard != 0x00) && (iCurGuard != 0x02)) || (iCurLevel == 14))
	{
		/*** disable guard ***/
		ShowImage (imgdisabled, 44, 326, "imgdisabled");
	}
}
/*****************************************************************************/
int IsDisabled (int iTile)
/*****************************************************************************/
{
	/*** skeleton ***/
	if ((iCurGuard != 0x01) && ((iTile == 83) || (iTile == 84)))
		{ return (1); }

	/*** fat ***/
	if ((iCurGuard != 0x03) && ((iTile == 85) || (iTile == 86)))
		{ return (1); }

	/*** shadow ***/
	if ((iCurGuard != 0x04) && ((iTile == 87) || (iTile == 88)))
		{ return (1); }

	/*** Jaffar ***/
	if ((iCurGuard != 0x05) && ((iTile == 89) || (iTile == 90)))
		{ return (1); }

	/*** guard ***/
	if ((((iCurGuard != 0x00) && (iCurGuard != 0x02)) || (iCurLevel == 14)) &&
		((iTile == 81) || (iTile == 82))) { return (1); }

	if (Unused (iTile) == 1) { return (1); }

	return (0);
}
/*****************************************************************************/
void CenterNumber (int iNumber, int iX, int iY,
	SDL_Color fore, int iHex)
/*****************************************************************************/
{
	char sText[MAX_TEXT + 2];

	if (iHex == 0)
	{
		snprintf (sText, MAX_TEXT, "%i", iNumber);
	} else {
		snprintf (sText, MAX_TEXT, "%02x", iNumber);
	}
	/* The 100000 is a workaround for 0 being broken. SDL devs have fixed that
	 * see e.g. https://hg.libsdl.org/SDL_ttf/rev/72b8861dbc01 but
	 * Ubuntu et al. still ship an sdl-ttf that is >10 years(!) old.
	 */
	message = TTF_RenderText_Blended_Wrapped (font3, sText, fore, 100000);
	messaget = SDL_CreateTextureFromSurface (ascreen, message);
	if (iHex == 0)
	{
		if ((iNumber >= -9) && (iNumber <= -1))
		{
			offset.x = iX + 16;
		} else if ((iNumber >= 0) && (iNumber <= 9)) {
			offset.x = iX + 21;
		} else if ((iNumber >= 10) && (iNumber <= 99)) {
			offset.x = iX + 14;
		} else {
			offset.x = iX + 7;
		}
	} else {
		offset.x = iX + 14;
	}
	offset.y = iY - 1;
	offset.w = message->w; offset.h = message->h;
	CustomRenderCopy (messaget, NULL, &offset, "message");
	SDL_DestroyTexture (messaget); SDL_FreeSurface (message);
}
/*****************************************************************************/
int Unused (int iTile)
/*****************************************************************************/
{
	if ((iTile >= 91) && (iTile <= 104)) { return (1); }

	return (0);
}
/*****************************************************************************/
void OpenURL (char *sURL)
/*****************************************************************************/
{
#if defined WIN32 || _WIN32 || WIN64 || _WIN64
ShellExecute (NULL, "open", sURL, NULL, NULL, SW_SHOWNORMAL);
#else
pid_t pid;
pid = fork();
if (pid == 0)
{
	execl ("/usr/bin/xdg-open", "xdg-open", sURL, (char *)NULL);
	exit (EXIT_NORMAL);
}
#endif
}
/*****************************************************************************/
void EXELoad (void)
/*****************************************************************************/
{
	int iFdEXE;
	unsigned char sRead[2 + 2];
	int iTab;
	int iHCSwitch;
	unsigned long ulOffset;

	/*** Used for looping. ***/
	int iSkillLoop;

	switch (iHomeComputer)
	{
		case 1: iHCSwitch = 1; break;
		case 2: iHCSwitch = 2; break;
		case 3: iHCSwitch = 3; break;
		default:
			printf ("[FAILED] Strange iHomeComputer: %i!\n", iHomeComputer);
			exit (EXIT_ERROR); break;
	}

	iFdEXE = open (sPathFile, O_RDONLY|O_BINARY);
	if (iFdEXE == -1)
	{
		printf ("[FAILED] Error opening %s: %s!\n",
			sPathFile, strerror (errno));
		exit (EXIT_ERROR);
	}

	/*** Prince HP. ***/
	switch (iHCSwitch)
	{
		case 1: ulOffset = ulPrinceHPA[iDiskImageA]; break;
		case 2: ulOffset = ulPrinceHPB[iDiskImageB]; break;
		case 3: ulOffset = ulPrinceHPC[iDiskImageC]; break;
	}
	if (ulOffset != 0x00)
	{
		lseek (iFdEXE, ulOffset, SEEK_SET);
		read (iFdEXE, sRead, 1);
		iEXEPrinceHP = sRead[0];
	}

	/*** Shadow HP. ***/
	switch (iHCSwitch)
	{
		case 1: ulOffset = ulShadowHPA[iDiskImageA]; break;
		case 2: ulOffset = ulShadowHPB[iDiskImageB]; break;
		case 3: ulOffset = ulShadowHPC[iDiskImageC]; break;
	}
	if (ulOffset != 0x00)
	{
		lseek (iFdEXE, ulOffset, SEEK_SET);
		read (iFdEXE, sRead, 1);
		iEXEShadowHP = sRead[0];
	}

	/*** Chomper delay. ***/
	switch (iHCSwitch)
	{
		case 1: ulOffset = ulChomperDelayA[iDiskImageA]; break;
		case 2: ulOffset = ulChomperDelayB[iDiskImageB]; break;
		case 3: ulOffset = ulChomperDelayC[iDiskImageC]; break;
	}
	if (ulOffset != 0x00)
	{
		lseek (iFdEXE, ulOffset, SEEK_SET);
		read (iFdEXE, sRead, 1);
		iEXEChomperDelay = sRead[0];
	}

	/*** Mouse delay. ***/
	switch (iHCSwitch)
	{
		case 1: ulOffset = ulMouseDelayA[iDiskImageA]; break;
		case 2: ulOffset = ulMouseDelayB[iDiskImageB]; break;
		case 3: ulOffset = ulMouseDelayC[iDiskImageC]; break;
	}
	if (ulOffset != 0x00)
	{
		lseek (iFdEXE, ulOffset, SEEK_SET);
		read (iFdEXE, sRead, 1);
		iEXEMouseDelay = sRead[0];
	}

	/*** Guard settings. ***/
	for (iTab = 1; iTab <= TABS_GUARD; iTab++)
	{
		switch (iHCSwitch)
		{
			case 1: ulOffset = ulGuardA[iDiskImageA][iTab - 1]; break;
			case 2: ulOffset = ulGuardB[iDiskImageB][iTab - 1]; break;
			case 3: ulOffset = ulGuardC[iDiskImageC][iTab - 1]; break;
		}
		if (ulOffset != 0x00)
		{
			lseek (iFdEXE, ulOffset, SEEK_SET);
			for (iSkillLoop = 1; iSkillLoop <= 12; iSkillLoop++)
			{
				read (iFdEXE, sRead, 1);
				iEXEGuard[iTab][iSkillLoop] = sRead[0];
			}
		}
	}

	/*** Guard HP. ***/
	switch (iHCSwitch)
	{
		case 1: ulOffset = ulGuardHPA[iDiskImageA]; break;
		case 2: ulOffset = ulGuardHPB[iDiskImageB]; break;
		case 3: ulOffset = ulGuardHPC[iDiskImageC]; break;
	}
	if (ulOffset != 0x00)
	{
		lseek (iFdEXE, ulOffset, SEEK_SET);
		for (iTab = 1; iTab <= TABS_LEVEL; iTab++)
		{
			if (iTab != TABS_LEVEL) /*** Level 14 has no guards. ***/
			{
				read (iFdEXE, sRead, 1);
				iEXEGuardHP[iTab - 1] = sRead[0];
			}
		}
	}

	/*** Guard uniform. ***/
	switch (iHCSwitch)
	{
		case 1: ulOffset = ulGuardUniformA[iDiskImageA]; break;
		case 2: ulOffset = ulGuardUniformB[iDiskImageB]; break;
		case 3: ulOffset = ulGuardUniformC[iDiskImageC]; break;
	}
	if (ulOffset != 0x00)
	{
		lseek (iFdEXE, ulOffset, SEEK_SET);
		for (iTab = 1; iTab <= TABS_LEVEL; iTab++)
		{
			if (iTab != TABS_LEVEL) /*** Level 14 has no guards. ***/
			{
				read (iFdEXE, sRead, 1);
				iEXEGuardU[iTab - 1] = sRead[0];
			}
		}
	}

	/*** Guard sprite. ***/
	switch (iHCSwitch)
	{
		case 1: ulOffset = ulGuardSpriteA[iDiskImageA]; break;
		case 2: ulOffset = ulGuardSpriteB[iDiskImageB]; break;
		case 3: ulOffset = ulGuardSpriteC[iDiskImageC]; break;
	}
	if (ulOffset != 0x00)
	{
		lseek (iFdEXE, ulOffset, SEEK_SET);
		for (iTab = 1; iTab <= TABS_LEVEL; iTab++)
		{
			if (iTab != TABS_LEVEL) /*** Level 14 has no guards. ***/
			{
				read (iFdEXE, sRead, 1);
				iEXEGuardS[iTab - 1] = sRead[0];
			}
		}
	} else {
		for (iTab = 1; iTab <= TABS_LEVEL; iTab++)
		{
			if (iTab != TABS_LEVEL) /*** Level 14 has no guards. ***/
			{
				iEXEGuardS[iTab - 1] = arDefaultGuardS[iTab - 1];
			}
		}
	}

	/*** Env. 1 ***/
	switch (iHCSwitch)
	{
		case 1: ulOffset = ulEnv1A[iDiskImageA]; break;
		case 2: ulOffset = ulEnv1B[iDiskImageB]; break;
		case 3: ulOffset = ulEnv1C[iDiskImageC]; break;
	}
	if (ulOffset != 0x00)
	{
		lseek (iFdEXE, ulOffset, SEEK_SET);
		for (iTab = 1; iTab <= TABS_LEVEL; iTab++)
		{
			read (iFdEXE, sRead, 1);
			iEXEEnv1[iTab - 1] = sRead[0];
		}
	} else {
		for (iTab = 1; iTab <= TABS_LEVEL; iTab++)
		{
			iEXEEnv1[iTab - 1] = arDefaultEnv1[iTab - 1];
		}
	}

	/*** Env. 2 ***/
	switch (iHCSwitch)
	{
		case 1: ulOffset = ulEnv2A[iDiskImageA]; break;
		case 2: ulOffset = ulEnv2B[iDiskImageB]; break;
		case 3: ulOffset = ulEnv2C[iDiskImageC]; break;
	}
	if (ulOffset != 0x00)
	{
		lseek (iFdEXE, ulOffset, SEEK_SET);
		for (iTab = 1; iTab <= TABS_LEVEL; iTab++)
		{
			read (iFdEXE, sRead, 1);
			iEXEEnv2[iTab - 1] = sRead[0];
		}
	} else {
		for (iTab = 1; iTab <= TABS_LEVEL; iTab++)
		{
			iEXEEnv2[iTab - 1] = arDefaultEnv2[iTab - 1];
		}
	}

	close (iFdEXE);
}
/*****************************************************************************/
void EXESave (void)
/*****************************************************************************/
{
	int iFdEXE;
	char sToWrite[MAX_TOWRITE + 2];
	int iTab;
	int iHCSwitch;
	unsigned long ulOffset;

	/*** Used for looping. ***/
	int iSkillLoop;

	switch (iHomeComputer)
	{
		case 1: iHCSwitch = 1; break;
		case 2: iHCSwitch = 2; break;
		case 3: iHCSwitch = 3; break;
		default:
			printf ("[FAILED] Strange iHomeComputer: %i!\n", iHomeComputer);
			exit (EXIT_ERROR); break;
	}

	iFdEXE = open (sPathFile, O_RDWR|O_BINARY);
	if (iFdEXE == -1)
	{
		printf ("[FAILED] Error opening %s: %s!\n",
			sPathFile, strerror (errno));
		exit (EXIT_ERROR);
	}

	/*** Prince HP. ***/
	switch (iHCSwitch)
	{
		case 1: ulOffset = ulPrinceHPA[iDiskImageA]; break;
		case 2: ulOffset = ulPrinceHPB[iDiskImageB]; break;
		case 3: ulOffset = ulPrinceHPC[iDiskImageC]; break;
	}
	if (ulOffset != 0x00)
	{
		lseek (iFdEXE, ulOffset, SEEK_SET);
		snprintf (sToWrite, MAX_TOWRITE, "%c", iEXEPrinceHP);
		write (iFdEXE, sToWrite, 1);
	}

	/*** Shadow HP. ***/
	switch (iHCSwitch)
	{
		case 1: ulOffset = ulShadowHPA[iDiskImageA]; break;
		case 2: ulOffset = ulShadowHPB[iDiskImageB]; break;
		case 3: ulOffset = ulShadowHPC[iDiskImageC]; break;
	}
	if (ulOffset != 0x00)
	{
		lseek (iFdEXE, ulOffset, SEEK_SET);
		snprintf (sToWrite, MAX_TOWRITE, "%c", iEXEShadowHP);
		write (iFdEXE, sToWrite, 1);
	}

	/*** Chomper delay. ***/
	switch (iHCSwitch)
	{
		case 1: ulOffset = ulChomperDelayA[iDiskImageA]; break;
		case 2: ulOffset = ulChomperDelayB[iDiskImageB]; break;
		case 3: ulOffset = ulChomperDelayC[iDiskImageC]; break;
	}
	if (ulOffset != 0x00)
	{
		lseek (iFdEXE, ulOffset, SEEK_SET);
		snprintf (sToWrite, MAX_TOWRITE, "%c", iEXEChomperDelay);
		write (iFdEXE, sToWrite, 1);
	}

	/*** Mouse delay. ***/
	switch (iHCSwitch)
	{
		case 1: ulOffset = ulMouseDelayA[iDiskImageA]; break;
		case 2: ulOffset = ulMouseDelayB[iDiskImageB]; break;
		case 3: ulOffset = ulMouseDelayC[iDiskImageC]; break;
	}
	if (ulOffset != 0x00)
	{
		lseek (iFdEXE, ulOffset, SEEK_SET);
		snprintf (sToWrite, MAX_TOWRITE, "%c", iEXEMouseDelay);
		write (iFdEXE, sToWrite, 1);
	}

	/*** Guard settings. ***/
	for (iTab = 1; iTab <= TABS_GUARD; iTab++)
	{
		switch (iHCSwitch)
		{
			case 1: ulOffset = ulGuardA[iDiskImageA][iTab - 1]; break;
			case 2: ulOffset = ulGuardB[iDiskImageB][iTab - 1]; break;
			case 3: ulOffset = ulGuardC[iDiskImageC][iTab - 1]; break;
		}
		if (ulOffset != 0x00)
		{
			lseek (iFdEXE, ulOffset, SEEK_SET);
			for (iSkillLoop = 1; iSkillLoop <= 12; iSkillLoop++)
			{
				snprintf (sToWrite, MAX_TOWRITE, "%c", iEXEGuard[iTab][iSkillLoop]);
				write (iFdEXE, sToWrite, 1);
			}
		}
	}

	/*** Guard HP. ***/
	switch (iHCSwitch)
	{
		case 1: ulOffset = ulGuardHPA[iDiskImageA]; break;
		case 2: ulOffset = ulGuardHPB[iDiskImageB]; break;
		case 3: ulOffset = ulGuardHPC[iDiskImageC]; break;
	}
	if (ulOffset != 0x00)
	{
		lseek (iFdEXE, ulOffset, SEEK_SET);
		for (iTab = 1; iTab <= TABS_LEVEL; iTab++)
		{
			if (iTab != TABS_LEVEL) /*** Level 14 has no guards. ***/
			{
				snprintf (sToWrite, MAX_TOWRITE, "%c", iEXEGuardHP[iTab - 1]);
				write (iFdEXE, sToWrite, 1);
			}
		}
	}

	/*** Guard uniform. ***/
	switch (iHCSwitch)
	{
		case 1: ulOffset = ulGuardUniformA[iDiskImageA]; break;
		case 2: ulOffset = ulGuardUniformB[iDiskImageB]; break;
		case 3: ulOffset = ulGuardUniformC[iDiskImageC]; break;
	}
	if (ulOffset != 0x00)
	{
		lseek (iFdEXE, ulOffset, SEEK_SET);
		for (iTab = 1; iTab <= TABS_LEVEL; iTab++)
		{
			if (iTab != TABS_LEVEL) /*** Level 14 has no guards. ***/
			{
				snprintf (sToWrite, MAX_TOWRITE, "%c", iEXEGuardU[iTab - 1]);
				write (iFdEXE, sToWrite, 1);
			}
		}
	}

	/*** Guard sprite. ***/
	switch (iHCSwitch)
	{
		case 1: ulOffset = ulGuardSpriteA[iDiskImageA]; break;
		case 2: ulOffset = ulGuardSpriteB[iDiskImageB]; break;
		case 3: ulOffset = ulGuardSpriteC[iDiskImageC]; break;
	}
	if (ulOffset != 0x00)
	{
		lseek (iFdEXE, ulOffset, SEEK_SET);
		for (iTab = 1; iTab <= TABS_LEVEL; iTab++)
		{
			if (iTab != TABS_LEVEL) /*** Level 14 has no guards. ***/
			{
				snprintf (sToWrite, MAX_TOWRITE, "%c", iEXEGuardS[iTab - 1]);
				write (iFdEXE, sToWrite, 1);
			}
		}
	}

	/*** Update iCurGuard if necessary. ***/
	if (iCurGuard != iEXEGuardS[iCurLevel])
		{ iCurGuard = iEXEGuardS[iCurLevel]; }

	/*** Env. 1 ***/
	switch (iHCSwitch)
	{
		case 1: ulOffset = ulEnv1A[iDiskImageA]; break;
		case 2: ulOffset = ulEnv1B[iDiskImageB]; break;
		case 3: ulOffset = ulEnv1C[iDiskImageC]; break;
	}
	if (ulOffset != 0x00)
	{
		lseek (iFdEXE, ulOffset, SEEK_SET);
		for (iTab = 1; iTab <= TABS_LEVEL; iTab++)
		{
			snprintf (sToWrite, MAX_TOWRITE, "%c", iEXEEnv1[iTab - 1]);
			write (iFdEXE, sToWrite, 1);
		}
	}

	/*** Env. 2 ***/
	switch (iHCSwitch)
	{
		case 1: ulOffset = ulEnv2A[iDiskImageA]; break;
		case 2: ulOffset = ulEnv2B[iDiskImageB]; break;
		case 3: ulOffset = ulEnv2C[iDiskImageC]; break;
	}
	if (ulOffset != 0x00)
	{
		lseek (iFdEXE, ulOffset, SEEK_SET);
		for (iTab = 1; iTab <= TABS_LEVEL; iTab++)
		{
			snprintf (sToWrite, MAX_TOWRITE, "%c", iEXEEnv2[iTab - 1]);
			write (iFdEXE, sToWrite, 1);
		}
	}

	/*** Update cCurType if necessary. ***/
	if ((iEXEEnv1[iCurLevel] == 0x01) && (iEXEEnv2[iCurLevel] == 0x01) &&
		(cCurType == 'd')) { cCurType = 'p'; }
	if ((iEXEEnv1[iCurLevel] == 0x02) && (iEXEEnv2[iCurLevel] == 0x02) &&
		(cCurType == 'p')) { cCurType = 'd'; }

	close (iFdEXE);

	PlaySound ("wav/save.wav");
}
/*****************************************************************************/
int PlusMinus (int *iWhat, int iX, int iY,
	int iMin, int iMax, int iChange, int iAddChanged)
/*****************************************************************************/
{
	if ((InArea (iX, iY, iX + 13, iY + 20) == 1) &&
		(((iChange < 0) && (*iWhat > iMin)) ||
		((iChange > 0) && (*iWhat < iMax))))
	{
		*iWhat = *iWhat + iChange;
		if ((iChange < 0) && (*iWhat < iMin)) { *iWhat = iMin; }
		if ((iChange > 0) && (*iWhat > iMax)) { *iWhat = iMax; }
		if (iAddChanged == 1) { iChanged++; }
		PlaySound ("wav/plus_minus.wav");
		return (1);
	} else { return (0); }
}
/*****************************************************************************/
void GetOptionValue (char *sArgv, char *sValue)
/*****************************************************************************/
{
	int iTemp;
	char sTemp[MAX_OPTION + 2];

	iTemp = strlen (sArgv) - 1;
	snprintf (sValue, MAX_OPTION, "%s", "");
	while (sArgv[iTemp] != '=')
	{
		snprintf (sTemp, MAX_OPTION, "%c%s", sArgv[iTemp], sValue);
		snprintf (sValue, MAX_OPTION, "%s", sTemp);
		iTemp--;
	}
}
/*****************************************************************************/
int BitsToInt (char *sString)
/*****************************************************************************/
{
	/*** Converts binary to decimal. ***/
	/*** Example: 11111111 to 255 ***/

	int iTemp = 0;

	for (; *sString; iTemp = (iTemp << 1) | (*sString++ - '0'));
	return (iTemp);
}
/*****************************************************************************/
void IntToBits (int iInt, char *sOutput, int iBits)
/*****************************************************************************/
{
	/*** Converts decimal to exactly iBits bits. ***/
	/*** Example: 255 to 11111111 ***/

	unsigned long luScale;
	unsigned long luFinal;
	int iTemp;
	int iDigit;
	char sOutputTemp[MAX_TOWRITE + 2];

	iTemp = iInt;
	luScale = 1;
	luFinal = 0;

	while (iTemp > 0)
	{
		iDigit = iTemp % 2;
		luFinal+=iDigit * luScale;
		iTemp = iTemp / 2;
		luScale = luScale * 10;
	}

	snprintf (sOutput, MAX_TOWRITE, "%lu", luFinal);
	if ((int)strlen (sOutput) != iBits)
	{
		do {
			snprintf (sOutputTemp, MAX_TOWRITE, "%s", sOutput);
			snprintf (sOutput, MAX_TOWRITE, "0%s", sOutputTemp);
		} while ((int)strlen (sOutput) != iBits);
	}
}
/*****************************************************************************/
void GetAsEightBits (unsigned char cChar, char *sBinary)
/*****************************************************************************/
{
	int i = CHAR_BIT;
	int iTemp;

	iTemp = 0;
	while (i > 0)
	{
		i--;
		if (cChar&(1 << i))
		{
			sBinary[iTemp] = '1';
		} else {
			sBinary[iTemp] = '0';
		}
		iTemp++;
	}
	sBinary[iTemp] = '\0';
}
/*****************************************************************************/
int ChecksumOrWrite (int iFd, int iLevel)
/*****************************************************************************/
{
	long lSum;
	char sToWrite[MAX_TOWRITE + 2];
	int iSC;
	char sBitsRoom[8 + 2];
	char sBitsTile[8 + 2];
	char sBinaryFDoors[9 + 2]; /*** 8 chars, plus \0 ***/
	char sBinarySDoors[9 + 2]; /*** 8 chars, plus \0 ***/
	char cNext;
	unsigned char cStartDir;

	/*** Used for looping. ***/
	int iRoomLoop;
	int iTileLoop;
	int iSideLoop;

	lSum = 0;

	/*** Tiles. ***/
	for (iRoomLoop = 1; iRoomLoop <= ROOMS; iRoomLoop++)
	{
		for (iTileLoop = 1; iTileLoop <= TILES; iTileLoop++)
		{
			snprintf (sToWrite, MAX_TOWRITE, "%c",
				arRoomTiles[iLevel][iRoomLoop][iTileLoop]);
			if (arRoomX[iLevel][iRoomLoop][iTileLoop] == 1)
				{ sToWrite[0]+=32; }
			if (iFd == -1)
				{ lSum+=sToWrite[0]; }
					else { write (iFd, sToWrite, 1); }
		}
	}

	/*** Mods. ***/
	for (iRoomLoop = 1; iRoomLoop <= ROOMS; iRoomLoop++)
	{
		for (iTileLoop = 1; iTileLoop <= TILES; iTileLoop++)
		{
			snprintf (sToWrite, MAX_TOWRITE, "%c",
				arRoomMod[iLevel][iRoomLoop][iTileLoop]);
			if (iFd == -1)
				{ lSum+=sToWrite[0]; }
					else { write (iFd, sToWrite, 1); }
		}
	}

	/*** Events. ***/
	for (iSC = 1; iSC <= 256; iSC++)
	{
		IntToBits (arEventsRoom[iLevel][iSC], sBitsRoom, 5);
		IntToBits (arEventsTile[iLevel][iSC] - 1, sBitsTile, 5);
		cNext = '0'; /*** Fallback. ***/
		switch (arEventsNext[iLevel][iSC])
		{
			case 0: cNext = '1'; break;
			case 1: cNext = '0'; break;
		}
		sBinaryFDoors[0] = cNext;
		sBinaryFDoors[1] = sBitsRoom[3];
		sBinaryFDoors[2] = sBitsRoom[4];
		sBinaryFDoors[3] = sBitsTile[0];
		sBinaryFDoors[4] = sBitsTile[1];
		sBinaryFDoors[5] = sBitsTile[2];
		sBinaryFDoors[6] = sBitsTile[3];
		sBinaryFDoors[7] = sBitsTile[4];
		sBinaryFDoors[8] = '\0';

		snprintf (sToWrite, MAX_TOWRITE, "%c", BitsToInt (sBinaryFDoors));
		if (iFd == -1)
			{ lSum+=sToWrite[0]; }
				else { write (iFd, sToWrite, 1); }
	}
	for (iSC = 1; iSC <= 256; iSC++)
	{
		IntToBits (arEventsRoom[iLevel][iSC], sBitsRoom, 5);
		IntToBits (arEventsTile[iLevel][iSC] - 1, sBitsTile, 5);
		sBinarySDoors[0] = sBitsRoom[0];
		sBinarySDoors[1] = sBitsRoom[1];
		sBinarySDoors[2] = sBitsRoom[2];
		/*** Not using arEventsTimer, for now. ***/
		sBinarySDoors[3] = '0';
		sBinarySDoors[4] = '0';
		sBinarySDoors[5] = '0';
		sBinarySDoors[6] = '0';
		sBinarySDoors[7] = '0';
		sBinarySDoors[8] = '\0';

		snprintf (sToWrite, MAX_TOWRITE, "%c", BitsToInt (sBinarySDoors));
		if (iFd == -1)
			{ lSum+=sToWrite[0]; }
				else { write (iFd, sToWrite, 1); }
	}

	/*** Room links. ***/
	for (iRoomLoop = 1; iRoomLoop <= ROOMS; iRoomLoop++)
	{
		for (iSideLoop = 1; iSideLoop <= 4; iSideLoop++)
		{
			snprintf (sToWrite, MAX_TOWRITE, "%c",
				arRoomLinks[iLevel][iRoomLoop][iSideLoop]);
			if (iFd == -1)
				{ lSum+=sToWrite[0]; }
					else { write (iFd, sToWrite, 1); }
		}
	}

	/*** Unknown (64). ***/
	for (iSC = 0; iSC < 64; iSC++)
	{
		snprintf (sToWrite, MAX_TOWRITE, "%c", arBytes64[iLevel][iSC]);
		if (iFd == -1)
			{ lSum+=sToWrite[0]; }
				else { write (iFd, sToWrite, 1); }
	}

	/*** Start location. ***/
	snprintf (sToWrite, MAX_TOWRITE, "%c", arStartLocation[iLevel][1]);
	if (iFd == -1) { lSum+=sToWrite[0]; } else { write (iFd, sToWrite, 1); }
	snprintf (sToWrite, MAX_TOWRITE, "%c", arStartLocation[iLevel][2] - 1);
	if (iFd == -1) { lSum+=sToWrite[0]; } else { write (iFd, sToWrite, 1); }
	snprintf (sToWrite, MAX_TOWRITE, "%c", arStartLocation[iLevel][3]);
	/*** 2 of 2 ***/
	if ((iLevel == 1) || (iLevel == 13))
	{
		if (arStartLocation[iLevel][3] == 0x00)
			{ cStartDir = 0xFF; }
				else { cStartDir = 0x00; }
		snprintf (sToWrite, MAX_TOWRITE, "%c", cStartDir);
	}
	if (iFd == -1) { lSum+=sToWrite[0]; } else { write (iFd, sToWrite, 1); }

	/*** Unknown (4). ***/
	for (iSC = 0; iSC < 4; iSC++)
	{
		snprintf (sToWrite, MAX_TOWRITE, "%c", arBytes4[iLevel][iSC]);
		if (iFd == -1) { lSum+=sToWrite[0]; } else { write (iFd, sToWrite, 1); }
	}

	/*** Guards. ***/
	for (iSC = 1; iSC <= 24; iSC++)
	{
		snprintf (sToWrite, MAX_TOWRITE, "%c", arGuardTile[iLevel][iSC] - 1);
		if (iFd == -1) { lSum+=sToWrite[0]; } else { write (iFd, sToWrite, 1); }
	}
	for (iSC = 1; iSC <= 24; iSC++)
	{
		snprintf (sToWrite, MAX_TOWRITE, "%c", arGuardDir[iLevel][iSC]);
		if (iFd == -1) { lSum+=sToWrite[0]; } else { write (iFd, sToWrite, 1); }
	}
	for (iSC = 1; iSC <= 24; iSC++)
	{
		snprintf (sToWrite, MAX_TOWRITE, "%c", arGuardUnk1[iLevel][iSC]);
		if (iFd == -1) { lSum+=sToWrite[0]; } else { write (iFd, sToWrite, 1); }
	}
	for (iSC = 1; iSC <= 24; iSC++)
	{
		snprintf (sToWrite, MAX_TOWRITE, "%c", arGuardUnk2[iLevel][iSC]);
		if (iFd == -1) { lSum+=sToWrite[0]; } else { write (iFd, sToWrite, 1); }
	}
	for (iSC = 1; iSC <= 24; iSC++)
	{
		snprintf (sToWrite, MAX_TOWRITE, "%c", arGuardSkill[iLevel][iSC]);
		if (iFd == -1) { lSum+=sToWrite[0]; } else { write (iFd, sToWrite, 1); }
	}
	for (iSC = 1; iSC <= 24; iSC++)
	{
		snprintf (sToWrite, MAX_TOWRITE, "%c", arGuardUnk3[iLevel][iSC]);
		if (iFd == -1) { lSum+=sToWrite[0]; } else { write (iFd, sToWrite, 1); }
	}
	for (iSC = 1; iSC <= 24; iSC++)
	{
		snprintf (sToWrite, MAX_TOWRITE, "%c", arGuardC[iLevel][iSC]);
		if (iFd == -1) { lSum+=sToWrite[0]; } else { write (iFd, sToWrite, 1); }
	}

	/*** Unknown (16). ***/
	for (iSC = 0; iSC < 16; iSC++)
	{
		snprintf (sToWrite, MAX_TOWRITE, "%c", arBytes16[iLevel][iSC]);
		if (iFd == -1) { lSum+=sToWrite[0]; } else { write (iFd, sToWrite, 1); }
	}

	if (iFd != -1)
	{
		snprintf (sToWrite, MAX_TOWRITE, "%c", cChecksum);
		write (iFd, sToWrite, 1);
		if (iDebug == 1)
		{
			printf ("[ INFO ] Saved checksum: 0x%02x (%i)\n",
				cChecksum, cChecksum);
		}
	}

	return (255 - (lSum % 256));
}
/*****************************************************************************/
int Verify (int iFd, int iOffset, char *sText)
/*****************************************************************************/
{
	char sVerify[100 + 2];
	int iSize;

	iSize = strlen (sText);

	lseek (iFd, iOffset, SEEK_SET);
	read (iFd, sVerify, iSize);
	sVerify[iSize] = '\0';
	if (strcmp (sVerify, sText) != 0)
	{
		return (0);
	} else {
		return (1);
	}
}
/*****************************************************************************/
void GetTileMod (int iGetRoom, int iGetTile, int *iTile, int *iMod)
/*****************************************************************************/
{
	*iTile = arRoomTiles[iCurLevel][iGetRoom][iGetTile];
	if (arRoomX[iCurLevel][iGetRoom][iGetTile] == 1) { *iTile+=32; }
	*iMod = arRoomMod[iCurLevel][iGetRoom][iGetTile];
}
/*****************************************************************************/
void GetTileModChange (int iGetTile, int *iTile, int *iMod)
/*****************************************************************************/
{
	switch (iGetTile)
	{
		/*** Row 1. ***/
		case 1: *iTile = 0x00; *iMod = 0x00; break;
		case 2: *iTile = 0x00; *iMod = 0x01; break;
		case 3: *iTile = 0x00; *iMod = 0x02; break;
		case 4: *iTile = 0x00; *iMod = 0x03; break;
		case 5: *iTile = 0x00; *iMod = 0xFF; break;
		case 6: *iTile = 0x01; *iMod = 0x00; break;
		case 7: *iTile = 0x01; *iMod = 0x01; break;
		case 8: *iTile = 0x01; *iMod = 0x02; break;
		case 9: *iTile = 0x01; *iMod = 0x03; break;
		case 10: *iTile = 0x01; *iMod = 0xFF; break;
		case 11: *iTile = 0x02; *iMod = 0x00; break;
		case 12: *iTile = 0x02; *iMod = 0x01; break;
		case 13: *iTile = 0x02; *iMod = 0x02; break;

		/*** Row 2. ***/
		case 14: *iTile = 0x02; *iMod = 0x03; break;
		case 15: *iTile = 0x02; *iMod = 0x04; break;
		case 16: *iTile = 0x02; *iMod = 0x05; break;
		case 17: *iTile = 0x02; *iMod = 0x06; break;
		case 18: *iTile = 0x02; *iMod = 0x07; break;
		case 19: *iTile = 0x02; *iMod = 0x08; break;
		case 20: *iTile = 0x02; *iMod = 0x09; break;
		case 21: *iTile = 0x03; *iMod = 0x00; break;
		case 22: *iTile = 0x04; *iMod = 0x02; break;
		case 23: *iTile = 0x04; *iMod = 0x01; break;
		case 24: *iTile = 0x05; *iMod = 0x00; break;
		case 25: *iTile = 0x06; *iMod = iChangeEvent - 1; break;
		case 26: *iTile = 0x07; *iMod = 0x00; break;

		/*** Row 3. ***/
		case 27: *iTile = 0x07; *iMod = 0x01; break;
		case 28: *iTile = 0x07; *iMod = 0x02; break;
		case 29: *iTile = 0x07; *iMod = 0x03; break;
		case 30: *iTile = 0x08; *iMod = 0x00; break;
		case 31: *iTile = 0x09; *iMod = 0x00; break;
		case 32: *iTile = 0x0A; *iMod = 0x00; break;
		case 33: *iTile = 0x0A; *iMod = 0x01; break;
		case 34: *iTile = 0x0A; *iMod = 0x02; break;
		case 35: *iTile = 0x0A; *iMod = 0x03; break;
		case 36: *iTile = 0x0A; *iMod = 0x04; break;
		case 37: *iTile = 0x0A; *iMod = 0x05; break;
		case 38: *iTile = 0x0A; *iMod = 0x06; break;
		case 39: *iTile = 0x0B; *iMod = 0x00; break;

		/*** Row 4. ***/
		case 40: *iTile = 0x0C; *iMod = 0x00; break;
		case 41: *iTile = 0x0C; *iMod = 0x01; break;
		case 42: *iTile = 0x0C; *iMod = 0x02; break;
		case 43: *iTile = 0x0C; *iMod = 0x03; break;
		case 44: *iTile = 0x0C; *iMod = 0x04; break;
		case 45: *iTile = 0x0C; *iMod = 0x05; break;
		case 46: *iTile = 0x0C; *iMod = 0x06; break;
		case 47: *iTile = 0x0C; *iMod = 0x07; break;
		case 48: *iTile = 0x0D; *iMod = 0x00; break;
		case 49: *iTile = 0x0E; *iMod = 0x00; break;
		case 50: *iTile = 0x0F; *iMod = iChangeEvent - 1; break;
		case 51: *iTile = 0x10; *iMod = 0x00; break;
		case 52: *iTile = 0x11; *iMod = 0x00; break;

		/*** Row 5. ***/
		case 53: *iTile = 0x12; *iMod = 0x00; break;
		case 54: *iTile = 0x12; *iMod = 0x01; break;
		case 55: *iTile = 0x12; *iMod = 0x02; break;
		case 56: *iTile = 0x12; *iMod = 0x03; break;
		case 57: *iTile = 0x12; *iMod = 0x04; break;
		case 58: *iTile = 0x12; *iMod = 0x05; break;
		case 59: *iTile = 0x13; *iMod = 0x00; break;
		case 60: *iTile = 0x14; *iMod = 0x00; break;
		case 61: *iTile = 0x14; *iMod = 0x01; break;
		case 62: *iTile = 0x15; *iMod = 0x00; break;
		case 63: *iTile = 0x16; *iMod = 0x00; break;
		case 64: *iTile = 0x17; *iMod = 0x00; break;
		case 65: *iTile = 0x18; *iMod = 0x00; break;

		/*** Row 6. ***/
		case 66: *iTile = 0x19; *iMod = 0x00; break;
		case 67: *iTile = 0x1A; *iMod = 0x00; break;
		case 68: *iTile = 0x1B; *iMod = 0x00; break;
		case 69: *iTile = 0x1C; *iMod = 0x00; break;
		case 70: *iTile = 0x1D; *iMod = 0x00; break;
		case 71: *iTile = 0x1E; *iMod = 0x00; break;
		case 72: *iTile = 0x2B; *iMod = 0x00; break;
		case 73: *iTile = 0x12; *iMod = 0x80; break;
		case 74: *iTile = 0x12; *iMod = 0x81; break;
		case 75: *iTile = 0x12; *iMod = 0x82; break;
		case 76: *iTile = 0x12; *iMod = 0x83; break;
		case 77: *iTile = 0x12; *iMod = 0x84; break;
		case 78: *iTile = 0x12; *iMod = 0x85; break;
	}
}
/*****************************************************************************/
void ApplySkillIfNecessary (int iTile)
/*****************************************************************************/
{
	if (arGuardTile[iCurLevel][iCurRoom] == iTile)
	{
		arGuardSkill[iCurLevel][iCurRoom] = iGuardType;
		iChanged++;
	}
}
/*****************************************************************************/
void LoadingBar (int iBarHeight)
/*****************************************************************************/
{
	SDL_Rect bar;

	bar.x = (10 + 2) * iScale;
	bar.y = (441 + 10 - 2 - iBarHeight) * iScale;
	bar.w = (20 - 2 - 2) * iScale;
	bar.h = iBarHeight * iScale;
	SDL_SetRenderDrawColor (ascreen, 0x44, 0x44, 0x44, SDL_ALPHA_OPAQUE);
	SDL_RenderFillRect (ascreen, &bar);
	iCurrentBarHeight = iBarHeight;

	/*** refresh screen ***/
	SDL_RenderPresent (ascreen);
}
/*****************************************************************************/
void HomeComputerAction (char *sAction)
/*****************************************************************************/
{
	if (strcmp (sAction, "one") == 0)
	{
		if (iAppleII == 1)
		{
			iHomeComputer = 1;
			iHomeComputerActive = 0;
		}
	}

	if (strcmp (sAction, "two") == 0)
	{
		if (iBBCMaster == 1)
		{
			iHomeComputer = 2;
			iHomeComputerActive = 0;
		}
	}

	if (strcmp (sAction, "three") == 0)
	{
		if (iC64 == 1)
		{
			iHomeComputer = 3;
			iHomeComputerActive = 0;
		}
	}
}
/*****************************************************************************/
void HomeComputer (void)
/*****************************************************************************/
{
	SDL_Event event;
	int iOldXPos, iOldYPos;

	iHomeComputerActive = 1;

	ShowHomeComputer();
	while (iHomeComputerActive == 1)
	{
		while (SDL_PollEvent (&event))
		{
			switch (event.type)
			{
				case SDL_CONTROLLERBUTTONDOWN:
					/*** Nothing for now. ***/
					break;
				case SDL_CONTROLLERBUTTONUP:
					switch (event.cbutton.button)
					{
						case SDL_CONTROLLER_BUTTON_A:
						case SDL_CONTROLLER_BUTTON_START:
							if ((iAppleII == 1) && (iOnAppleII == 1))
								{ HomeComputerAction ("one"); }
							if ((iBBCMaster == 1) && (iOnBBCMaster == 1))
								{ HomeComputerAction ("two"); }
							if ((iC64 == 1) && (iOnC64 == 1))
								{ HomeComputerAction ("three"); }
							break;
						case SDL_CONTROLLER_BUTTON_B:
						case SDL_CONTROLLER_BUTTON_BACK:
							Quit(); break;
						case SDL_CONTROLLER_BUTTON_LEFTSHOULDER:
						case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
							if ((iOnC64 == 1) && (iBBCMaster == 1))
							{
								iOnBBCMaster = 1; iOnC64 = 0;
								ShowHomeComputer();
							} else if ((iOnBBCMaster == 1) && (iAppleII == 1)) {
								iOnAppleII = 1; iOnBBCMaster = 0;
								ShowHomeComputer();
							}
							break;
						case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER:
						case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
							if ((iOnAppleII == 1) && (iBBCMaster == 1))
							{
								iOnAppleII = 0; iOnBBCMaster = 1;
								ShowHomeComputer();
							} else if ((iOnBBCMaster == 1) && (iC64 == 1)) {
								iOnBBCMaster = 0; iOnC64 = 1;
								ShowHomeComputer();
							}
							break;
						case SDL_CONTROLLER_BUTTON_X:
							HomeComputerAction ("one");
							break;
						case SDL_CONTROLLER_BUTTON_Y:
							HomeComputerAction ("two");
							break;
						case SDL_CONTROLLER_BUTTON_GUIDE:
							HomeComputerAction ("three");
							break;
					}
					ShowHomeComputer();
					break;
				case SDL_CONTROLLERAXISMOTION: /*** triggers and analog sticks ***/
					iXJoy1 = SDL_JoystickGetAxis (joystick, 0);
					iXJoy2 = SDL_JoystickGetAxis (joystick, 3);
					if ((event.caxis.axis == SDL_CONTROLLER_AXIS_TRIGGERLEFT) ||
						(iXJoy1 < -30000) || (iXJoy2 < -30000)) /*** left ***/
					{
						if ((iOnC64 == 1) && (iBBCMaster == 1))
						{
							iOnBBCMaster = 1; iOnC64 = 0;
							ShowHomeComputer();
						} else if ((iOnBBCMaster == 1) && (iAppleII == 1)) {
							iOnAppleII = 1; iOnBBCMaster = 0;
							ShowHomeComputer();
						}
					}
					if ((event.caxis.axis == SDL_CONTROLLER_AXIS_TRIGGERRIGHT) ||
						(iXJoy1 > 30000) || (iXJoy2 > 30000)) /*** right ***/
					{
						if ((iOnAppleII == 1) && (iBBCMaster == 1))
						{
							iOnAppleII = 0; iOnBBCMaster = 1;
							ShowHomeComputer();
						} else if ((iOnBBCMaster == 1) && (iC64 == 1)) {
							iOnBBCMaster = 0; iOnC64 = 1;
							ShowHomeComputer();
						}
					}
					break;
				case SDL_KEYDOWN:
					switch (event.key.keysym.sym)
					{
						case SDLK_ESCAPE:
							Quit(); break;
						case SDLK_KP_ENTER:
						case SDLK_RETURN:
						case SDLK_SPACE:
							if ((iAppleII == 1) && (iOnAppleII == 1))
								{ HomeComputerAction ("one"); }
							if ((iBBCMaster == 1) && (iOnBBCMaster == 1))
								{ HomeComputerAction ("two"); }
							if ((iC64 == 1) && (iOnC64 == 1))
								{ HomeComputerAction ("three"); }
							break;
						case SDLK_1: /*** Apple II ***/
							HomeComputerAction ("one");
							break;
						case SDLK_2: /*** BBC Master ***/
							HomeComputerAction ("two");
							break;
						case SDLK_3: /*** C64 ***/
							HomeComputerAction ("three");
							break;
						case SDLK_LEFT:
							if ((iOnC64 == 1) && (iBBCMaster == 1))
							{
								iOnBBCMaster = 1; iOnC64 = 0;
								ShowHomeComputer();
							} else if ((iOnBBCMaster == 1) && (iAppleII == 1)) {
								iOnAppleII = 1; iOnBBCMaster = 0;
								ShowHomeComputer();
							}
							break;
						case SDLK_RIGHT:
							if ((iOnAppleII == 1) && (iBBCMaster == 1))
							{
								iOnAppleII = 0; iOnBBCMaster = 1;
								ShowHomeComputer();
							} else if ((iOnBBCMaster == 1) && (iC64 == 1)) {
								iOnBBCMaster = 0; iOnC64 = 1;
								ShowHomeComputer();
							}
							break;
						default: break;
					}
					ShowHomeComputer();
					break;
				case SDL_MOUSEMOTION:
					iOldXPos = iXPos;
					iOldYPos = iYPos;
					iXPos = event.motion.x;
					iYPos = event.motion.y;
					if ((iOldXPos == iXPos) && (iOldYPos == iYPos)) { break; }

					if (InArea (22, 22, 22 + 176, 22 + 417) == 1) /*** Apple II ***/
					{
						if ((iAppleII == 1) && (iOnAppleII != 1))
							{ iOnAppleII = 1; iOnBBCMaster = 0; iOnC64 = 0; }
					} else if (InArea (218, 22, 218 + 176, 22 + 417) == 1) { /*** BM ***/
						if ((iBBCMaster == 1) && (iOnBBCMaster != 1))
							{ iOnAppleII = 0; iOnBBCMaster = 1; iOnC64 = 0; }
					} else if (InArea (414, 22, 414 + 176, 22 + 417) == 1) { /*** C ***/
						if ((iC64 == 1) && (iOnC64 != 1))
							{ iOnAppleII = 0; iOnBBCMaster = 0; iOnC64 = 1; }
					} else {
						iOnAppleII = 0; iOnBBCMaster = 0; iOnC64 = 0;
					}
					ShowHomeComputer();
					break;
				case SDL_MOUSEBUTTONDOWN:
					/*** Nothing for now. ***/
					break;
				case SDL_MOUSEBUTTONUP:
					iOnAppleII = 0;
					iOnBBCMaster = 0;
					if (event.button.button == 1)
					{
						if (InArea (22, 22, 22 + 176, 22 + 417) == 1)
						{ /*** Apple II ***/
							HomeComputerAction ("one");
						}
						if (InArea (218, 22, 218 + 176, 22 + 417) == 1)
						{ /*** BBC Master ***/
							HomeComputerAction ("two");
						}
						if (InArea (414, 22, 414 + 176, 22 + 417) == 1)
						{ /*** C64 ***/
							HomeComputerAction ("three");
						}
					}
					ShowHomeComputer(); break;
				case SDL_WINDOWEVENT:
					if (event.window.event == SDL_WINDOWEVENT_EXPOSED)
						{ ShowHomeComputer(); } break;
				case SDL_QUIT:
					Quit(); break;
			}
		}
	}

	/*** prevent CPU eating ***/
	gamespeed = REFRESH_PROG;
	while ((SDL_GetTicks() - looptime) < gamespeed)
	{
		SDL_Delay (10);
	}
	looptime = SDL_GetTicks();
}
/*****************************************************************************/
void ShowHomeComputer (void)
/*****************************************************************************/
{
	/*** background ***/
	ShowImage (imghc, 0, 0, "imghc");

	/*** Apple II ***/
	if (iAppleII == 0)
	{
		/*** disabled ***/
		ShowImage (imghcadis, 22, 22, "imghcadis");
	} else if (iOnAppleII == 0) {
		/*** off ***/
		ShowImage (imghcaoff, 22, 22, "imghcaoff");
	} else {
		/*** on ***/
		ShowImage (imghcaon, 22, 22, "imghcaon");
	}

	/*** BBC Master ***/
	if (iBBCMaster == 0)
	{
		/*** disabled ***/
		ShowImage (imghcbdis, 218, 22, "imghcbdis");
	} else if (iOnBBCMaster == 0) {
		/*** off ***/
		ShowImage (imghcboff, 218, 22, "imghcboff");
	} else {
		/*** on ***/
		ShowImage (imghcbon, 218, 22, "imghcbon");
	}

	/*** C64 ***/
	if (iC64 == 0)
	{
		/*** disabled ***/
		ShowImage (imghccdis, 414, 22, "imghccdis");
	} else if (iOnC64 == 0) {
		/*** off ***/
		ShowImage (imghccoff, 414, 22, "imghccoff");
	} else {
		/*** on ***/
		ShowImage (imghccon, 414, 22, "imghccon");
	}

	/*** refresh screen ***/
	SDL_RenderPresent (ascreen);
}
/*****************************************************************************/
void PlaytestStart (int iLevel)
/*****************************************************************************/
{
	int iFd;
	int iOffsetStart;
	char sToWrite[MAX_TOWRITE + 2];

	/*** Open file. ***/
	iFd = open (sPathFile, O_RDWR|O_BINARY, 0600);
	if (iFd == -1)
	{
		printf ("[FAILED] Error opening %s: %s!\n",
			sPathFile, strerror (errno));
		exit (EXIT_ERROR);
	}

	/*** Skip the intro. ***/
	switch (iHomeComputer)
	{
		case 1: iOffsetStart = ulSkipIntroA[iDiskImageA]; break;
		case 2: iOffsetStart = ulSkipIntroB[iDiskImageB]; break;
		case 3: iOffsetStart = ulSkipIntroC[iDiskImageC]; break;
		default: printf ("[FAILED] iHomeComputer!\n"); exit (EXIT_ERROR); break;
	}
	if (iOffsetStart != 0x00)
	{
		lseek (iFd, iOffsetStart, SEEK_SET);
		snprintf (sToWrite, MAX_TOWRITE, "%c", 0xD5);
		write (iFd, sToWrite, 1);
	}

	/*** Set the starting level. ***/
	switch (iHomeComputer)
	{
		case 1: iOffsetStart = ulStartLevelA[iDiskImageA]; break;
		case 2: iOffsetStart = ulStartLevelB[iDiskImageB]; break;
		case 3: iOffsetStart = ulStartLevelC[iDiskImageC]; break;
		default: printf ("[FAILED] iHomeComputer!\n"); exit (EXIT_ERROR); break;
	}
	if (iOffsetStart != 0x00)
	{
		lseek (iFd, iOffsetStart, SEEK_SET);
		snprintf (sToWrite, MAX_TOWRITE, "%c", iLevel);
		write (iFd, sToWrite, 1);
	}

	close (iFd);

	iModified = 1;
}
/*****************************************************************************/
void PlaytestStop (void)
/*****************************************************************************/
{
	int iFd;
	int iOffsetStart;
	char sToWrite[MAX_TOWRITE + 2];

	/*** Open file. ***/
	iFd = open (sPathFile, O_RDWR|O_BINARY, 0600);
	if (iFd == -1)
	{
		printf ("[FAILED] Error opening %s: %s!\n",
			sPathFile, strerror (errno));
		exit (EXIT_ERROR);
	}

	/*** [Undo] Skip the intro. ***/
	switch (iHomeComputer)
	{
		case 1: iOffsetStart = ulSkipIntroA[iDiskImageA]; break;
		case 2: iOffsetStart = ulSkipIntroB[iDiskImageB]; break;
		case 3: iOffsetStart = ulSkipIntroC[iDiskImageC]; break;
		default: printf ("[FAILED] iHomeComputer!\n"); exit (EXIT_ERROR); break;
	}
	if (iOffsetStart != 0x00)
	{
		lseek (iFd, iOffsetStart, SEEK_SET);
		snprintf (sToWrite, MAX_TOWRITE, "%c", 0x8A);
		write (iFd, sToWrite, 1);
	}

	/*** [Undo] Set the starting level. ***/
	switch (iHomeComputer)
	{
		case 1: iOffsetStart = ulStartLevelA[iDiskImageA]; break;
		case 2: iOffsetStart = ulStartLevelB[iDiskImageB]; break;
		case 3: iOffsetStart = ulStartLevelC[iDiskImageC]; break;
		default: printf ("[FAILED] iHomeComputer!\n"); exit (EXIT_ERROR); break;
	}
	if (iOffsetStart != 0x00)
	{
		lseek (iFd, iOffsetStart, SEEK_SET);
		snprintf (sToWrite, MAX_TOWRITE, "%c", 0x01);
		write (iFd, sToWrite, 1);
	}

	close (iFd);

	iModified = 0;
}
/*****************************************************************************/
