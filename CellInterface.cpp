/* CellularInterface
 * Copyright (c) 2015 ARM Limited
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "CellInterface.h"
#include "mbed.h"
#include <math.h>

//ZELITRON Server VIP (Load balancer )
//#define HOST "192.168.5.80"
//#define PORT 9005

//ZELITRON
//#define HOST "195.46.10.19"
//#define PORT 9005

//#define HOST "ciot.it-sgn.u-blox.com"
//#define PORT 5683

//#define HOST "api.connector.mbed.com"
//#define PORT 5684

//#define HOST "echo.u-blox.com"
//#define PORT 7

//#define HOST "coap.me"
//#define PORT 5683

//Neul ecco server
#define HOST "120.16.45.6"
#define PORT 41000


// Sara-N module power pin
__attribute__((section("AHBSRAM1")))  static DigitalOut gModulePowerOn(P1_17, 0);
// Sara-N module reset pin
__attribute__((section("AHBSRAM1")))  static DigitalOut gSaraResetBar(P1_0, 0);



CellInterface::CellInterface(const char *simpin, bool debug)
    : _debug(debug)
{
    strcpy(_pin, simpin);
    running = false;
}

CellInterface::~CellInterface()
{
    _thread.join();
}

nsapi_error_t CellInterface::set_credentials(const char *apn, const char *username , const char *password ){
	printf("CellInterface:: set_credentials\r\n");
    memset(_apn, 0, sizeof(_apn));
    strncpy(_apn, apn, sizeof(_apn));

    memset(_username, 0, sizeof(_username));
    strncpy(_username, username, sizeof(_username));

    memset(_password, 0, sizeof(_password));
    strncpy(_password, password, sizeof(_password));
    return 0;
}



nsapi_error_t CellInterface::connect(
		const char *apn          /* = 0 */,
		const char *username     /* = 0 */,
		const char *password     /* = 0 */)
{
	set_credentials(apn, username, password);
	return connect();
}

nsapi_error_t CellInterface::connect()
{
	_debug = true;
	//printf("CellInterface:: connect \r\n");
	gModulePowerOn = 1;
	gSaraResetBar = 1;

	_mdm = new MDMSerial(MDMTXD, MDMRXD, 9600, NC, NC);     // use mdm(D1,D0) if you connect the cellular shield to a C027
	if (_debug) {
		_mdm->setDebug(4);
	} else {
	    _mdm->setDebug(0);
	}
	MDMParser::DevStatus devStatus = {};
	MDMParser::NetStatus netStatus = {};
	bool mdmOk = _mdm->init(_pin, &devStatus);

	return mdmOk ? 0 : NSAPI_ERROR_DEVICE_ERROR;
}

int CellInterface::disconnect()
{
	//printf("CellInterface:: disconnect\r\n");
    if (!_mdm->disconnect()) {
        return NSAPI_ERROR_DEVICE_ERROR;
    }
    return 0; //success
}

const char *CellInterface::get_ip_address()
{
	//printf("CellInterface:: get_ip_address: %s \r\n", _ip_address.get_ip_address() );
	return _ip_address.get_ip_address();
}

const char *CellInterface::get_mac_address()
{
	//printf("CellInterface:: get_mac_address\r\n");
    return 0;
}






struct c027_socket {
    int socket;
    MDMParser::IpProtocol proto;
    MDMParser::IP ip;
    int port;
};



int CellInterface::socket_open(nsapi_socket_t *handle, nsapi_protocol_t proto)
{
	//printf("CellInterface:: socket_open\r\n");
	int fd = _mdm->socketSocket(MDMParser::IPPROTO_UDP, (int)PORT);
	if (fd < 0) {
		return NSAPI_ERROR_NO_SOCKET;
	}
	_mdm->socketSetBlocking(fd, 10000);
	struct c027_socket *socket = new struct c027_socket;
	if (!socket) {
        return NSAPI_ERROR_NO_SOCKET;
	}
	socket->socket = fd;
    socket->proto = MDMParser::IPPROTO_UDP;
    *handle = socket;
    return 0; //success
}


int CellInterface::socket_close(nsapi_socket_t handle)
{
	//printf("CellInterface:: socket_close\r\n");
    struct c027_socket *socket = (struct c027_socket *)handle;
    _mdm->socketFree(socket->socket);
    delete socket;
    return 0;
}


int CellInterface::socket_bind(nsapi_socket_t handle, const SocketAddress &address)
{
	//printf("CellInterface:: socket_bind\r\n");
	return NSAPI_ERROR_UNSUPPORTED;
}


int CellInterface::socket_listen(nsapi_socket_t handle, int backlog)
{
	//printf("CellInterface:: socket_listen\r\n");
	return NSAPI_ERROR_UNSUPPORTED;
}


int CellInterface::socket_connect(nsapi_socket_t handle, const SocketAddress &addr)
{
	//printf("CellInterface:: socket_connect\r\n");
    struct c027_socket *socket = (struct c027_socket *)handle;
    if (!_mdm->socketConnect(socket->socket, addr.get_ip_address(), addr.get_port())) {
        return NSAPI_ERROR_DEVICE_ERROR;
    }
    return 0; //0 on success, negative on failure
}


nsapi_error_t CellInterface::socket_accept(nsapi_socket_t server,  nsapi_socket_t *handle, SocketAddress *address)
{
	//printf("CellInterface:: socket_accept\r\n");
	return NSAPI_ERROR_UNSUPPORTED;
}


int CellInterface::socket_send(nsapi_socket_t handle, const void *data, unsigned size)
{
	//printf("CellInterface:: socket_send\r\n");
	struct c027_socket *socket = (struct c027_socket *)handle;
	int sent = _mdm->socketSend(socket->socket, (const char *)data, size);
	if (sent == SOCKET_ERROR) {
		return NSAPI_ERROR_DEVICE_ERROR;
	}
	return sent;
}


int CellInterface::socket_recv(nsapi_socket_t handle, void *data, unsigned size)
{
	//printf("CellInterface:: socket_recv\r\n");
	struct c027_socket *socket = (struct c027_socket *)handle;
	if (!_mdm->socketReadable(socket->socket)) {
        return NSAPI_ERROR_WOULD_BLOCK;
	}
	int recv = _mdm->socketRecv(socket->socket, (char *)data, size);
    if (recv == SOCKET_ERROR) {
    	return NSAPI_ERROR_DEVICE_ERROR;
	}
    return recv;
}







/** Send a packet to a remote endpoint
*
*
*  @param  handle       Socket handle
*  @param  address      The remote SocketAddress
*  @param  data         The packet to be sent
*  @param  size         The length of the packet to be sent
*
*  @return the          number of written bytes on success, negative on failure
*
*  @note This call is not-blocking, if this call would block, must
*        immediately return NSAPI_ERROR_WOULD_WAIT
*
*/
nsapi_size_or_error_t CellInterface::socket_sendto(nsapi_socket_t handle, const SocketAddress &addr, const void *data, unsigned size)
{
	//printf("CellInterface:: socket_sendto ----------------------------- size:%d \r\n", size);
    struct c027_socket *socket = (struct c027_socket *)handle;
    int sent = _mdm->socketSendTo(
    		socket->socket,
    		*(MDMParser::IP *)addr.get_ip_bytes(),
    		addr.get_port(),
    		reinterpret_cast<const char*>(data),
    		(int)size);

    if (sent == SOCKET_ERROR) {
        return NSAPI_ERROR_DEVICE_ERROR;
    }
    return sent;
}





/** Receive a packet from a remote endpoint
 *
 *
 *  @param handle       Socket handle
 *  @param address      Destination for the remote SocketAddress or null
 *  @param buffer       The buffer for storing the incoming packet data
 *                      If a packet is too long to fit in the supplied buffer,
 *                      excess bytes are discarded
 *  @param size         The length of the buffer
 *
 *  @return the         number of received bytes on success, negative on failure
 *  @note This call is not-blocking, if this call would block, must
 *        immediately return NSAPI_ERROR_WOULD_WAIT
 *
 */
nsapi_size_or_error_t CellInterface::socket_recvfrom(nsapi_socket_t handle, SocketAddress *addr, void *data, unsigned size)
{
	struct c027_socket *socket = (struct c027_socket *)handle;
	if (!_mdm->socketReadable(socket->socket)) {
		return NSAPI_ERROR_WOULD_BLOCK;
	}
	MDMParser::IP ip;
	int port;

	int recv = _mdm->socketRecvFrom(socket->socket, &ip, &port, (char *)data, size);
	if (recv == SOCKET_ERROR) {
		return NSAPI_ERROR_DEVICE_ERROR;
	}
	if (addr) {
		addr->set_ip_bytes(&ip, NSAPI_IPv4);
		addr->set_port(port);
	}
	return recv;
}




/**
 *  Function to call on state change
 */
void CellInterface::socket_check()
{
	//printf(" ************************** socket_check *************************\r\n");
    while(running)
    {
        //for (int i = 0; i < NUMSOCKETS; i++)
        //{
            m.lock();
            int readable = _mdm->socketReadable(0);
            m.unlock();

            if (readable != -1 && readable != 0)
            {
                event();
            }
        //}
        wait(1);
    }
    running = false;// do not change it
}

void CellInterface::event()
{
	//printf(" ************************** event *************************\r\n");
    //for (int i = 0; i < NUMSOCKETS; i++){
    	//printf("i=%d \r\n", i);
        //if (_cbs[i].callback) {
        	//printf("call i=%d \r\n", i);
            _cbs[0].callback(_cbs[0].data);
        //}
    //}
}



//void CellInterface::_run()
//{
//	printf("CellInterface::_run\r\n");
//    while (true) {
//        _callback(_data);
//        wait(0.25f);
//    }
//}



/** Register a callback on state change of the socket
 *
 *  The specified callback will be called on state changes such as when the socket
 *  can recv/send/accept successfully and on when an error occurs.
 *  The callback may also be called spuriously without reason.
 *
 *  The callback may be called in an interrupt context and should
 *  not perform expensive operations such as recv/send calls
 *
 *  @param  handle       Socket handle
 *  @param  callback     Function to call on state change
 *  @param  data         Argument to pass to callback
 *
 *  @note   Callback may be called in an interrupt context.
 *
 */
void CellInterface::socket_attach(void *handle, void (*callback)(void *), void *data)
{
    struct c027_socket *socket = (struct c027_socket *)handle;

    //printf("***** socket_attach ***** ip(%d) port(%d) proto(%d) socket(%d) *****\r\n", socket->ip, socket->port, socket->proto, socket->socket);

    _cbs[socket->socket].callback = callback;
    _cbs[socket->socket].data = data;

    if(!running)
    {
        running = true;
        _thread.start(this, &CellInterface::socket_check);
    }
}


//void CellInterface::socket_attach(nsapi_socket_t handle, void (*callback)(void *), void *data)
//{
//	mutex.lock();
//
//	_callback = callback;
//	_data = data;
//
//	printf("CellInterface:: socket_attach [Register callback on socket state change]\r\n");
//
//
//	if ((_callback != NULL) && (_data != NULL))
//	{
//		printf("[start thread]\r\n");
//    	_thread.start(this, &CellInterface::_run);
//	}
//	else
//	{
//		printf("[terminate thread]\r\n");
//		_thread.terminate();
//	}
//
//
//	mutex.unlock();
//}





/*
 * Hamed Siasi
 *
 */

