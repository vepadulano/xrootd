/*****************************************************************************/
/*                                                                           */
/*                                Collector.cc                                */
/*                                                                           */
/* (c) 2005 by the Board of Trustees of the Leland Stanford, Jr., University */
/*                            All Rights Reserved                            */
/*       Produced by Jacek Becla for Stanford University under contract      */
/*              DE-AC02-76SF00515 with the Department of Energy              */
/*****************************************************************************/

// $Id$

#include "XrdMon/XrdMonCommon.hh"
#include "XrdMon/XrdMonTimer.hh"
#include "XrdMon/XrdMonCtrBuffer.hh"
#include "XrdMon/XrdMonCtrPacket.hh"
#include <sys/socket.h>
#include <assert.h>

//#define DEBUG
#define PRINT_SPEED

// for DEBUG/PRINT_SPEED only
#include "XrdMon/XrdMonCtrDebug.hh"
#include "XrdMon/XrdMonCtrMutexLocker.hh"
#include "XrdMon/XrdMonCtrSenderInfo.hh"

#include <iomanip>
#include <iostream>
using std::cout;

void
printSpeed()
{
    static int64_t noP = 0;
    static XrdMonTimer t;
    if ( 0 == noP ) {
        t.start();
    }
    ++noP;
    const int64_t EVERY = 1001;
    if ( noP % EVERY == EVERY-1) {
        double elapsed = t.stop();
        cout << noP << " packets received in " << elapsed 
             << " sec (" << EVERY/elapsed << " Hz)" << endl;
        t.reset(); t.start();
    }
}

extern "C" void* receivePackets(void*)
{
    struct sockaddr_in sAddress;

    int socket_ = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);    
    assert( -1 != socket_ );

    memset((char *) &sAddress, sizeof(sAddress), 0);
    sAddress.sin_family = AF_INET;
    sAddress.sin_port = htons(PORT);
    sAddress.sin_addr.s_addr = htonl(INADDR_ANY);

    if ( -1 == bind(socket_, 
                    (struct sockaddr*)&sAddress, 
                    sizeof(sAddress)) ) {
        cerr << "Failed to bind, port " << PORT << endl;
        ::abort();
    }

    XrdMonCtrBuffer* pb = XrdMonCtrBuffer::instance();
    cout << "ready to receive data" << endl;
    while ( 1 ) {
        XrdMonCtrPacket* packet = new XrdMonCtrPacket(MAXPACKETSIZE);
        socklen_t slen = sizeof(packet->sender);
        if ( -1 == recvfrom(socket_, 
                            packet->buf, 
                            MAXPACKETSIZE, 
                            0, 
                            (sockaddr* )(&(packet->sender)), 
                            &slen) ) {
            cerr << "Failed to receive data" << endl;
            ::abort();
        }
#ifdef DEBUG
        static int32_t packetNo = 0;
        ++packetNo;
        {
            XrdMonCtrXrdOucMutexHelper mh; mh.Lock(&XrdMonCtrDebug::_mutex);
            cout << "Received packet no " 
                 << setw(5) << packetNo << " from " 
                 << XrdMonCtrSenderInfo::hostPort(packet->sender) << endl;
        }
#endif

        pb->push_back(packet);

#ifdef PRINT_SPEED
        printSpeed();
#endif
    }
    return 0;
}
