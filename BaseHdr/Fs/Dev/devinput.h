/**
* BSD 2-Clause License
*
* Copyright (c) 2022-2023, Manas Kamal Choudhury
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*
* 1. Redistributions of source code must retain the above copyright notice, this
*    list of conditions and the following disclaimer.
*
* 2. Redistributions in binary form must reproduce the above copyright notice,
*    this list of conditions and the following disclaimer in the documentation
*    and/or other materials provided with the distribution.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
* FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
* DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
* SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
* CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
* OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
* OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*
**/

#ifndef __DEV_INPUT_H__
#define __DEV_INPUT_H__

#include <stdint.h>
#include <aurora.h>

#define LEFT_CLICK        0x01
#define RIGHT_CLICK       0x02
#define MIDDLE_CLICK      0x04
#define MOUSE_SCROLL_UP   0x05
#define MOUSE_SCROLL_DOWN 0x06

#define AU_INPUT_MOUSE  1
#define AU_INPUT_KEYBOARD 2

#define NUM_MOUSE_PACKETS    20
#define NUM_KEYBOARD_PACKETS 512

typedef struct _au_input_msg_ {
	uint8_t type;
	int32_t xpos;
	int32_t ypos;
	uint8_t button_state;
	uint32_t code;
	uint32_t code1;
	uint32_t code3;
	uint32_t code4;
}AuInputMessage;

/*
* AuDevInputInitialise -- mounts to pipe
* for hid devices, @mice and @kybrd
*/
extern void AuDevInputInitialise();

/*
* AuDevReadMice -- reads packets from pipe
* to buffer
* @para, inputmsg -- Pointer to the buffer
*/
AU_EXTERN AU_EXPORT void AuDevReadMice(AuInputMessage* inputmsg);

/*
* AuDevWriteMice -- writes a packet to pipe
* @param outmsg -- packet to write
*/
AU_EXTERN AU_EXPORT void AuDevWriteMice(AuInputMessage* outmsg);

#endif