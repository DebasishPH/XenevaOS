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

#ifndef __IOCODES_H__
#define __IOCODES_H__

#define SOUND_REGISTER_SNDPLR 100
#define SOUND_START_OUTPUT    102
#define SOUND_STOP_OUTPUT     103
#define SOUND_START_INPUT     104
#define SOUND_STOP_INPUT      105
#define SOUND_SET_VOLUME      106
#define SOUND_GET_VOLUME      107
#define SOUND_MUTE_ENABLE     108
#define SOUND_MUTE_DISABLE    109
#define SOUND_READ_AVAIL      110
#define SOUND_UNREGISTER_SNDPLR 111

#define SCREEN_SETMODE    200
#define SCREEN_GETWIDTH   201
#define SCREEN_GETHEIGHT  202
#define SCREEN_GETBPP     203
#define SCREEN_SETBPP     204
#define SCREEN_UPDATE     205
#define SCREEN_GET_SCANLINE  206
#define SCREEN_GET_PITCH     207
#define SCREEN_REG_MNGR      208
#define SCREEN_GET_FB     209

#define MOUSE_IOCODE_SETPOS  10

#define POSTBOX_CREATE  401
#define POSTBOX_DESTROY 402
#define POSTBOX_PUT_EVENT  403
#define POSTBOX_GET_EVENT  404
#define POSTBOX_CREATE_ROOT  405
#define POSTBOX_GET_EVENT_ROOT  406

/*I/O Codes used for network interfaces */
#define NET_GET_HARDWARE_ADDRESS 0x100
#define NET_SET_IPV4_ADDRESS 0x101
#define NET_GET_IPV4_ADDRESS 0x102
#define NET_GET_GATEWAY_ADDRESS 0x103
#define NET_SET_GATEWAY_ADDRESS 0x104
#define NET_GET_SUBNET_MASK 0x105
#define NET_SET_SUBNET_MASK 0x106
#define NET_GET_LINK_STATUS 0x107

#endif