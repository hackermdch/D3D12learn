#pragma once
#define HID_USAGE_PAGE_GENERIC 0x01
#define HID_USAGE_GENERIC_MOUSE 0x02
#define HID_USAGE_GENERIC_KEYBOARD 0x06
#define RI_KEY_MAKE 0	
#define RI_KEY_BREAK 1	
#define RI_KEY_E0 2
#define RI_KEY_E1 4

inline bool IsButtonDown(UINT flags, UINT button) {
	return ((flags >> (button - 1) * 2) & 0b1) == 1;
}

inline bool IsButtonUp(UINT flags, UINT button) {
	return ((flags >> (button - 1) * 2) & 0b10) == 0b10;
}

