#pragma once

/* Original driver implementation types; not part of the public API. */
#define MAX_FINGER_NUM      3
#define TP_MULTI_SUCCESS    0

#define TP_PEN_NONE         0
#define TP_PEN_MOVE         1
#define TP_PEN_UP           2
#define TP_PEN_DOWN         3

#define TG_NO_DETECT        0
#define TG_ZOOM_IN          1
#define TG_ZOOM_OUT         2
#define TG_UNKNOWN_STATE    3

#define TP_UP_MSG           0
#define TP_DOWN_MSG         1
#define TP_MOVE_MSG         1
#define TP_MAX_MSG          0

typedef unsigned int DAWORD;
typedef unsigned short WORD;
typedef unsigned char BYTE;

typedef int s32;
typedef short s16;
typedef char s8;

typedef DAWORD uint32;
typedef BYTE uint8;
typedef WORD uint16;
typedef s32 int32;
typedef s16 int16;
typedef BYTE TP_STATE_E;
typedef BYTE TG_STATE_E;
typedef DAWORD TP_IOCTL_CMD_E;

typedef struct _XY_DATA_T{
	WORD x_position,y_position;
	BYTE finger_id;
}XY_DATA_T;

typedef struct _TPDSVR_SIG_T{
	WORD x_key, y_key;
	BYTE cur_index;
	TG_STATE_E gesture_type;
	WORD num_of_point;
	BYTE SignalCode;
	XY_DATA_T xy_position[3];
}TPDSVR_SIG_T;

