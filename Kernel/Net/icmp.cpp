/**
* BSD 2-Clause License
*
* Copyright (c) 2022-2024, Manas Kamal Choudhury
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

#include <net\socket.h>
#include <process.h>
#include <Fs/vfs.h>
#include <Mm/kmalloc.h>
#include <string.h>
#include <Net/ipv4.h>
#include <Net/aunet.h>
#include <_null.h>
#include <Hal/serial.h>
#include <Net/icmp.h>

AuSocket* current_icmp_sock;

uint16_t AuICMPChecksum(IPv4Header* packet) {
	uint32_t sum = 0;
	uint16_t* s = (uint16_t*)packet->payload;
	for (int i = 0; i < (ntohs(packet->totalLength) - 20) / 2; ++i)
		sum += ntohs(s[i]);

	if (sum > 0xFFFF)
		sum = (sum >> 16) + (sum & 0xFFFF);

	return ~(sum & UINT16_MAX) & UINT16_MAX;
}

/*
 * AuICMPHandle -- ICMP handler
 */
void AuICMPHandle(IPv4Header* ipv4, AuVFSNode* nic) {
	ICMPHeader* header = (ICMPHeader*)&ipv4->payload;


	AuNetworkDevice* netdev = (AuNetworkDevice*)nic->device;
	if (!netdev) {
		SeTextOut("[AuNet]: ICMP Handle no network device found \r\n");
		return;
	}

	/* PING */
	if (header->type == 8 && header->code == 0) {
		SeTextOut("[AuNet]: Ping with %d bytes of payload \r\n", ntohs(ipv4->totalLength));
		SeTextOut("From -> ");
		ip_ntoa(ntohl(ipv4->srcAddress));
		SeTextOut("\r\n");
		if (ntohs(ipv4->totalLength) & 1)
			ipv4->totalLength = htons(ntohs(ipv4->totalLength) + 1);

		IPv4Header* resp = (IPv4Header*)kmalloc(ntohs(ipv4->totalLength));
		memcpy(resp, ipv4, ntohs(ipv4->totalLength));
		resp->totalLength = ipv4->totalLength;
		resp->destAddress = ipv4->srcAddress;
		resp->srcAddress = htonl(netdev->ipv4addr);
		resp->timeToLive = 64;
		resp->protocol = 1;
		resp->identification = ipv4->identification;
		resp->flagsFragOffset = htons(0x4000);
		resp->versionHeaderLen = 0x45;
		resp->typeOfService = 0;
		resp->headerChecksum = 0;
		resp->headerChecksum = htons(IPv4CalculateChecksum(resp));

		ICMPHeader* reply = (ICMPHeader*)&resp->payload;
		reply->checksum = 0;
		reply->type = 0;
		SeTextOut("reply->code -> %d \r\n", reply->code);
		reply->checksum = htons(AuICMPChecksum(resp));

		IPV4SendPacket(resp, nic);
		kfree(resp);
	}
	else if (header->type == 0 && header->code == 0) {
		SeTextOut("[AuNet]:ICMP ping reply got \r\n");
		if (current_icmp_sock)
			AuSocketAdd(current_icmp_sock, ipv4, ntohs(ipv4->totalLength));
	}
	else {
		SeTextOut("[AuNet]: NIC -> %s, ICMP type-%d code-%d \r\n", nic->filename, header->type, header->code);
	}
}
/*
* AuICMPReceive -- ICMP protocol receive interface
* @param sock -- Pointer to socket
* @param msghdr -- Message header containing every information
* @param flags -- extra flags
*/
int AuICMPReceive(AuSocket* sock, msghdr *msg, int flags){
	if (msg->msg_iovlen > 1)
		return -1;

	if (msg->msg_iovlen == 0)return 0;
	char* packet = (char*)AuSocketGet(sock);
	if (!packet) return 0;
	size_t packet_sz = *(size_t*)packet - sizeof(IPv4Header);
	IPv4Header* src = (IPv4Header*)(packet + sizeof(size_t));
	if (packet_sz > msg->msg_iov[0].iov_len) 
		packet_sz = msg->msg_iov[0].iov_len;
	
	if (msg->msg_namelen == sizeof(sockaddr_in)) {
		if (msg->msg_name) {
			((sockaddr_in*)msg->msg_name)->sin_family = AF_INET;
			((sockaddr_in*)msg->msg_name)->sin_port = 0;
			((sockaddr_in*)msg->msg_name)->sin_addr.s_addr = src->srcAddress;
			((sockaddr_in*)msg->msg_name)->sin_zero[0] = src->timeToLive;
		}
	}

	memcpy(msg->msg_iov[0].iov_base, src->payload, packet_sz);
	kfree(packet);
	return packet_sz;
}

/*
* AuICMPSend -- ICMP protocol send interface
* @param sock -- Pointer to socket
* @param msghdr -- Message header containing every information
* @param flags -- extra flags
*/
int AuICMPSend(AuSocket* sock, msghdr* msg, int flags){
	if (msg->msg_iovlen > 1)
		return -1;
	if (msg->msg_iovlen == 0)return 0;
	if (msg->msg_namelen != sizeof(sockaddr_in))
		return -1;

	sockaddr_in* name = (sockaddr_in*)msg->msg_name;
	AuVFSNode* nic = AuNetworkRoute(name->sin_addr.s_addr);
	if (!nic) return -1;
	AuNetworkDevice* netdev = (AuNetworkDevice*)nic->device;
	if (!netdev)
		return -1;

	size_t totalLen = sizeof(IPv4Header) + msg->msg_iov[0].iov_len;

	IPv4Header* resp = (IPv4Header*)kmalloc(totalLen);
	memset(resp, 0, totalLen);
	resp->totalLength = htons(totalLen);
	resp->destAddress = htonl(name->sin_addr.s_addr);
	resp->srcAddress = netdev->ipv4addr;
	resp->timeToLive = 64;
	resp->protocol = 1;
	resp->identification = 0;
	resp->flagsFragOffset = htons(0x4000);
	resp->versionHeaderLen = 0x45;
	resp->typeOfService = 0;
	resp->headerChecksum = 0;
	resp->headerChecksum = htons(IPv4CalculateChecksum(resp));

	memcpy(resp->payload, msg->msg_iov[0].iov_base, msg->msg_iov[0].iov_len);
	IPV4SendPacket(resp, nic);
	kfree(resp);
	return 0;
}

/*
* AuICMPClose -- ICMP protocol close call
* @param sock -- Pointer to socket
*/
void AuICMPClose(AuSocket* sock) {
	return;
}


int AuICMPBind(AuSocket* sock, sockaddr* addr, socklen_t addrlen){
	return 0;
}


uint64_t AuICMPRead(AuVFSNode* node, AuVFSNode* file, uint64_t* buffer, uint32_t len){
	return 0;
}

uint64_t AuICMPWrite(AuVFSNode* node, AuVFSNode* file, uint64_t* buffer, uint32_t len) {
	return 0;
}

int AuICMPFileClose(AuVFSNode* fsys, AuVFSNode* file) {
	/* here both fs and file points to socket file , better
	 * would be using file pointer */
	AuSocket* sock = (AuSocket*)file->device;

	if (sock) {
		if (sock->rxstack) {
			while (sock->rxstack->itemCount) {
				void* data = AuStackPop(sock->rxstack);
				kfree(data);
			}
			kfree(sock->rxstack);
		}
		
		kfree(sock);
	}
	
	kfree(file);
	current_icmp_sock = NULL;
	SeTextOut("ICMP: Socket Closed \r\n");
	return 0;
}

/*
 * ICMPInitialise -- initialise ICMP
 */
void ICMPInitialise() {
	current_icmp_sock = 0;
}

/*
 * CreateICMPSocket -- create a new Internet
 * Control Message Protocol (ICMP) protocol
 */
int CreateICMPSocket() {
	if (current_icmp_sock) {
		SeTextOut("ICMP: Already there is a socket listening to ICMP, close that \r\n");
		return -1;
	}
	int fd = -1;
	AuThread *thread = AuGetCurrentThread();
	if (!thread)
		return -1;
	AuProcess *proc = AuProcessFindThread(thread);
	if (!proc)
		proc = AuProcessFindSubThread(thread);
	if (!proc)
		return -1;	
	fd = AuProcessGetFileDesc(proc);
	AuSocket* sock = AuNetCreateSocket();
	sock->bind = AuICMPBind;
	sock->close = AuICMPClose;
	sock->connect = 0;
	sock->receive = AuICMPReceive;
	sock->send = AuICMPSend;
	AuVFSNode* node = (AuVFSNode*)kmalloc(sizeof(AuVFSNode));
	memset(node, 0, sizeof(AuVFSNode));
	strcpy(node->filename, "icmp");
	node->flags |= FS_FLAG_SOCKET;
	node->device = sock;
	node->close = AuICMPFileClose;
	node->iocontrol = SocketIOControl;
	proc->fds[fd] = node;
	current_icmp_sock = sock;
	SeTextOut("ICMP Socket created \r\n");
	return fd;
}