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

#ifndef CELL_INTERFACE_H
#define CELL_INTERFACE_H

#include "nsapi.h"
#include "CellularInterface.h"
#include "MDM.h"



#define NUMSOCKETS 12


class CellInterface : public NetworkStack, public CellularInterface
{
public:
	    /*
	     * @param simpin    Optional PIN for the SIM
	     * @param debug     Enable debugging
	     */
	CellInterface(const char *simpin = NULL, bool debug = true);

	~CellInterface();


	   /** Start the interface
	     *
	     *  Attempts to connect to a cellular network based on supplied credentials
	     *
	     *  @return         0 on success, negative error code on failure
	     */
    virtual nsapi_error_t connect(const char *apn, const char *username = 0, const char *password = 0);
    virtual nsapi_error_t connect();


       /** Stop the interface
         *  @return             0 on success, negative on failure
         */
	virtual int disconnect();


     	/** Get the internally stored IP address
	     *  @return             IP address of the interface or null if not yet connected
	     */
    virtual const char *get_ip_address();


       /** Get the internally stored MAC address
         *  @return             MAC address of the interface
         */
    virtual const char *get_mac_address();


       /** Set the cellular network APN and credentials
         *
         *  @param apn      Optional name of the network to connect to
         *  @param user     Optional username for the APN
         *  @param pass     Optional password fot the APN
         *  @return         0 on success, negative error code on failure
         */
    virtual nsapi_error_t set_credentials(const char *apn, const char *username = 0, const char *password = 0);







protected:


    /** Open a socket
     *  @param handle       Handle in which to store new socket
     *  @param proto        Type of socket to open, NSAPI_TCP or NSAPI_UDP
     *  @return             0 on success, negative on failure
     */
    virtual int socket_open(void **handle, nsapi_protocol_t proto);




    /** Close the socket
     *  @param handle       Socket handle
     *  @return             0 on success, negative on failure
     *  @note On failure, any memory associated with the socket must still
     *        be cleaned up
     */
    virtual int socket_close(void *handle);






       /** Bind a server socket to a specific port
         *  @param handle       Socket handle
         *  @param address      Local address to listen for incoming connections on
         *  @return             0 on success, negative on failure.
         */
    virtual int socket_bind(void *handle, const SocketAddress &address);






       /** Start listening for incoming connections
         *  @param handle       Socket handle
         *  @param backlog      Number of pending connections that can be queued up at any
         *                      one time [Default: 1]
         *  @return             0 on success, negative on failure
         */
    virtual int socket_listen(void *handle, int backlog);






       /** Connects this TCP socket to the server
         *  @param handle       Socket handle
         *  @param address      SocketAddress to connect to
         *  @return             0 on success, negative on failure
         */
    virtual int socket_connect(void *handle, const SocketAddress &addr);





       /** Accept a new connection.
         *  @param handle       Handle in which to store new socket
         *  @param server       Socket handle to server to accept from
         *  @return             0 on success, negative on failure
         *  @note This call is not-blocking, if this call would block, must
         *        immediately return NSAPI_ERROR_WOULD_WAIT
         */
    virtual nsapi_error_t socket_accept(nsapi_socket_t server, nsapi_socket_t *handle, SocketAddress *address=0);






       /** Send data to the remote host
         *  @param handle       Socket handle
         *  @param data         The buffer to send to the host
         *  @param size         The length of the buffer to send
         *  @return             Number of written bytes on success, negative on failure
         *  @note This call is not-blocking, if this call would block, must
         *        immediately return NSAPI_ERROR_WOULD_WAIT
         */
    virtual int socket_send(void *handle, const void *data, unsigned size);






       /** Send a packet to a remote endpoint
         *  @param handle       Socket handle
         *  @param address      The remote SocketAddress
         *  @param data         The packet to be sent
         *  @param size         The length of the packet to be sent
         *  @return the         number of written bytes on success, negative on failure
         *  @note This call is not-blocking, if this call would block, must
         *        immediately return NSAPI_ERROR_WOULD_WAIT
         */
    virtual int socket_recv(void *handle, void *data, unsigned size);





       /** Send a packet to a remote endpoint
         *  @param handle       Socket handle
         *  @param address      The remote SocketAddress
         *  @param data         The packet to be sent
         *  @param size         The length of the packet to be sent
         *  @return the         number of written bytes on success, negative on failure
         *  @note This call is not-blocking, if this call would block, must
         *        immediately return NSAPI_ERROR_WOULD_WAIT
         */
    virtual int socket_sendto(void *handle, const SocketAddress &addr, const void *data, unsigned size);






       /** Receive a packet from a remote endpoint
         *  @param handle       Socket handle
         *  @param address      Destination for the remote SocketAddress or null
         *  @param buffer       The buffer for storing the incoming packet data
         *                      If a packet is too long to fit in the supplied buffer,
         *                      excess bytes are discarded
         *  @param size         The length of the buffer
         *  @return the         number of received bytes on success, negative on failure
         *  @note This call is not-blocking, if this call would block, must
         *        immediately return NSAPI_ERROR_WOULD_WAIT
         */
    virtual int socket_recvfrom(void *handle, SocketAddress *addr, void *data, unsigned size);







       /** Register a callback on state change of the socket
         *  @param handle       Socket handle
         *  @param callback     Function to call on state change
         *  @param data         Argument to pass to callback
         *  @note Callback may be called in an interrupt context.
         */
    virtual void socket_attach(void *handle, void (*callback)(void *), void *data);







       /** Provide access to the NetworkStack object
         *
         *  @return The underlying NetworkStack object
         */
    virtual NetworkStack *get_stack(){
             return this;
    }







private:
    bool _debug;
    bool running;

    MDMSerial *_mdm; /*modem obj*/
    SocketAddress _ip_address;

    char _mac_address[NSAPI_MAC_SIZE];
    char _pin[sizeof "1234"];
    char _apn[1];/*50*/
    char _username[1];/*25*/
    char _password[1];/*25*/

    Mutex m;
    Thread _thread;

    struct {
            void (*callback)(void *);
            void *data;
    } _cbs[NUMSOCKETS]; /* Callbacks for socket_attach */  //_cbs[NUMSOCKETS];


    void (*_callback)(void *); void *_data;
    void *_handle;
    void socket_check();
    void event();
};


#endif

/* Hamed Siasi */
