/*
 *
 * Copyright (C) u-blox Melbourn Ltd
 * u-blox Melbourn Ltd, Melbourn, UK
 *
 * All rights reserved.
 *
 * This source file is the sole property of u-blox Melbourn Ltd.
 * Reproduction or utilisation of this source in whole or part is
 * forbidden without the written consent of u-blox Melbourn Ltd.
 * hamed.siasi@u-blox.com
 *
 */


//#define debug
#ifndef debug

#include "simpleclient.h"
#include "mbed.h"
#include "CellInterface.h"

//#define MBED_SERVER_ADDRESS  "coap://api.connector.mbed.com:5684"
//#define MBED_SERVER_ADDRESS  "coap://169.45.82.18:5684"            // ARM LwM2M server
//#define MBED_SERVER_ADDRESS  "coap://151.9.34.90:5683"             // Sgonico CoAP echo server
//#define MBED_SERVER_ADDRESS  "coap://151.9.34.99:8080"             // Junaid Ashraf
//#define MBED_SERVER_ADDRESS  "coap://195.46.10.19:9005"            // ZELITRON LwM2M server  Newbury OpenLAB
//#define MBED_SERVER_ADDRESS  "coap://192.168.5.80:9005"            // ZELITRON Server VIP (Load balancer )
#define MBED_SERVER_ADDRESS    "coap://120.16.45.6:41000"            // Neul ecco server



#define SIMPIN      NULL                //  NULL
#define APN         "m2m.zelitron.com"  //  "giffgaff.com"
#define USERNAME    "web"               //  "giffgaff"
#define PASSWORD    "web"               //  NULL

__attribute__((section("AHBSRAM0"))) DigitalOut myled(LED1);

MbedClientDevice device = {
    "Manufacturer_String",      // Manufacturer
    "Type_String",              // Type
    "ModelNumber_String",       // ModelNumber
    "SerialNumber_String"       // SerialNumber
};


__attribute__((section("AHBSRAM0"))) MbedClient  mbed_client(device);


class TempResource {
public:

    TempResource() {
        // (1) Obj
        temp_object = M2MInterfaceFactory::create_object("3303");

        // (2) Instance
        M2MObjectInstance* temp_inst = temp_object->create_object_instance();

        // (3) Resouce
        M2MResource* status_res = temp_inst->create_dynamic_resource(
        		"5700"                                /*resource_name*/,
        		NULL                                  /*resource_type*/,
				M2MResourceInstance::STRING           /*type*/,
				false                                  /*observable*/
				                                      /*multiple_instance*/);

        // (4) Read(GET) and Write(PUT)
        status_res->set_operation(M2MBase::GET_ALLOWED);

        // (5) Value of the resource
        status_res->set_value((const uint8_t*)"20.5" /*value*/, (const uint32_t) sizeof("20.5") /*value_length*/);

        // (6) Callback
        status_res->set_execute_function( execute_callback(this, &TempResource::Handling) );

        // (7) Delay
        status_res->set_delayed_response(true);
    }
    M2MObject* get_object() {
        return temp_object;
    }
    void Handling(void *argument) {
    	printf("Temperature Sensor Handling ! %s \r\n\n\n\n\n\n\n", (char*)argument );
    }
private:
    M2MObject* temp_object;
};


class ServerObj {
public:

	ServerObj() {
        // (1) Obj
        server_object = M2MInterfaceFactory::create_object("1");//3303

        // (2) Instance
        M2MObjectInstance* server_inst = server_object->create_object_instance();

        // (3) Resouce
        M2MResource* status_res = server_inst->create_dynamic_resource(
        		"1"                                /*resource_name*/,//5700
        		NULL                               /*resource_type*/,
				M2MResourceInstance::STRING         /*type*/,
				false                               /*observable*/
				                                   /*multiple_instance*/);
        // (4) Read(GET) and Write(PUT)
        status_res->set_operation(M2MBase::GET_PUT_POST_DELETE_ALLOWED);//GET_ALLOWED

        // (5) Value of the resource
        status_res->set_value((const uint8_t*)"300" /*value*/, (const uint32_t) sizeof("300") /*value_length*/);//20.5

        // (6) Callback
        status_res->set_execute_function( execute_callback(this, &ServerObj::Handling) );

        // (7) Delay
        status_res->set_delayed_response(true);
    }
	~ServerObj() {
	    }
    M2MObject* get_object() {
        return server_object;
    }
    void Handling(void *argument) {

    	if (argument) {
    		printf("Handling!\r\n");
    	    M2MResource::M2MExecuteParameter* param = (M2MResource::M2MExecuteParameter*)argument;
    	    String    object_name          = param->get_argument_object_name();
    	    uint16_t  object_instance_id   = param->get_argument_object_instance_id();
    	    String    resource_name        = param->get_argument_resource_name();
    	    int       payload_length       = param->get_argument_value_length();
    	    uint8_t*  payload              = param->get_argument_value();

    	    printf("Resource: %s/%d/%s executed\r\n", object_name.c_str(), object_instance_id, resource_name.c_str());
    	    printf("Payload: %.*s\r\n\n\n\n", payload_length, payload);
    	}
    	else{
    		printf("Handling! No argument \r\n");
    	}

    }

private:
    M2MObject* server_object;
};



int main() {
    NetworkInterface *network_interface = 0;
    wait_ms(3000);

    CellInterface cell;
    cell.connect();
    network_interface = &cell;




    /*** Create endpoint interface to manage register and unregister ***/
    mbed_client.create_interface(MBED_SERVER_ADDRESS, network_interface);





    /*** Create Objects of varying types, see simpleclient.h for more details on implementation. ***/
    M2MSecurity* securityObject  = mbed_client.create_register_object();
    M2MDevice*   device_object   = mbed_client.create_device_object();





    /*** Create list of Objects to register ***/
    M2MObjectList object_list;
    TempResource  temp_resource;
    //ServerObj    server_resource;





    /*** Add objects to list ***/
    object_list.push_back(device_object);                     // obj 3
    object_list.push_back(temp_resource.get_object());        // obj 3303
    //object_list.push_back(server_resource.get_object());




    /*** Set endpoint registration object ***/
    mbed_client.set_register_object(securityObject);




    /*** Register ***/
    mbed_client.test_register(securityObject, object_list);




    while(true){
    	myled = 1;
    	wait_ms(2000000);  // 25sec
    	myled = 0;

    	mbed_client.test_update_register();
    }


    mbed_client.test_unregister();
}













#else

#include "mbed.h"


DigitalOut myled(LED1);

int main() {
    // open the PC serial port and (use the same baudrate)
//    Serial pc (USBTX, USBRX);
//    pc.baud (9600);
//    pc.set_flow_control (SerialBase::Disabled, NC, NC);

    while(1) {
    	printf("test \r\n");
        myled = 1;
        wait(2);
        myled = 0;
        wait(2);
    }
}



//#include "mbed.h"
//#include "GPS.h"
//#include "MDM.h"
//
//#define SIMPIN      NULL
//#define APN         "giffgaff.com"
//#define USERNAME    "giffgaff"
//#define PASSWORD    NULL
//
//int main(void)
//{
//    int ret;
//    char buf[512] = "";
//
//    // Create the modem object
//    MDMSerial mdm; // use mdm(D1,D0) if you connect the cellular shield to a C027
//    mdm.setDebug(4); // enable this for debugging issues
//
//    // initialize the modem
//    MDMParser::DevStatus devStatus = {};
//    MDMParser::NetStatus netStatus = {};
//    bool mdmOk = mdm.init(SIMPIN, &devStatus);
//    mdm.dumpDevStatus(&devStatus);
//    if (mdmOk) {
//        // wait until we are connected
//        mdmOk = mdm.registerNet(&netStatus);
//        mdm.dumpNetStatus(&netStatus);
//    }
//    if (mdmOk)
//    {
//        // join the internet connection
//        MDMParser::IP ip = mdm.join(APN,USERNAME,PASSWORD);
//        if (ip == NOIP)
//            printf("Not able to join network");
//        else
//        {
//            mdm.dumpIp(ip);
//            int port = 5683;
//            const char* host = "ciot.it-sgn.u-blox.com";
//            MDMParser::IP ip = mdm.gethostbyname(host);
//            char data[] = "UDP SOCKET TEST";
//
//                    printf("Testing UDP sockets with ECHO server\r\n");
//                    int socket = mdm.socketSocket(MDMParser::IPPROTO_UDP, port);
//                    if (socket >= 0)
//                    {
//                        mdm.socketSetBlocking(socket, 10000);
//                        ret = mdm.socketSendTo(socket, ip, port, data, sizeof(data)-1);
//                        if (ret == sizeof(data)-1) {
//                            printf("Socket SendTo %s:%d " IPSTR " %d \"%s\"\r\n", host, port, IPNUM(ip), ret, data);
//                        }
//                        ret = mdm.socketRecvFrom(socket, &ip, &port, buf, sizeof(buf)-1);
//                        if (ret >= 0) {
//                            printf("Socket RecvFrom " IPSTR ":%d %d \"%.*s\" \r\n", IPNUM(ip),port, ret, ret,buf);
//                        }
//                        mdm.socketFree(socket);
//                    }
//            // disconnect
//            mdm.disconnect();
//        }
//    }
//    mdm.powerOff();
//    return 0;
//}

#endif
