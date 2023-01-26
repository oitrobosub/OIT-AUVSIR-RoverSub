// Tyler Lucas
// Creation Date: 1/5/2022
#pragma once
#include <iostream>
#include <fstream>
#include <sstream>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <future>
#include "IMUBuilder.h"
#include "CondenseData.h"

#define DEFAULT_BUFLEN 1024

using namespace std;

IMUBuilder* DAIMUBuilder = new IMUBuilder();
//Used to track connection state through threads
promise<bool> statePromise;

bool pollData(SOCKET sock);

int main()
{
    //Create TCP connection with FakeBot
    SOCKET sock = INVALID_SOCKET;
    bool connecting = 1;
    while(connecting)
    {
        // Initialize WinSock
        WSADATA wsaData;
        int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
        if (result != 0)
        {
            cerr << "Error initializing WinSock: \n" << result << endl;
            connecting = 1;
        }

        // Set the IP address and port number for the server
        const char* ipAddress = "127.0.0.1";
        int port = 8888;

        // Create a new socket.
        sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (sock == INVALID_SOCKET)
        {
            cerr << "Error creating socket: \n" << WSAGetLastError() << endl;
            WSACleanup();
            connecting = 1;
        }

        // Set up sockaddr_in struct
        sockaddr_in serverAddress;
        serverAddress.sin_family = AF_INET;
        serverAddress.sin_port = htons(port);
        inet_pton(AF_INET, ipAddress, &serverAddress.sin_addr);

        // Connect to the server.
        if (connect(sock, (sockaddr*)&serverAddress, sizeof(serverAddress)) == SOCKET_ERROR)
        {
            cerr << "Error connecting to server: \n" << WSAGetLastError() << endl;
            closesocket(sock);
            WSACleanup();
            connecting = 1;
        }
        connecting = 0;
    }

    //call out to other classes & communicate with server
    //below is temp to get proof of communication
    char recvbuf[DEFAULT_BUFLEN];
    int recvbuflen = DEFAULT_BUFLEN;

    const char* initMsg = "iRequest";
    int result;

    if ((send(sock, initMsg, (int)strlen(initMsg), 0) == SOCKET_ERROR))
    {
        cerr << "send failed: \n" << WSAGetLastError() << endl;
        closesocket(sock);
        WSACleanup();
        return 1;
    }

    bool communicating;
    if (recv(sock, recvbuf, recvbuflen, 0) < 0)
        communicating = 0;
    else
        communicating = 1;
    if (communicating)
    {
        //Creates object with all connected IMUs named, NOT no data present at this point
        DAIMUBuilder->IMUPopulator(recvbuf);

        //TEMPORARY count to use later
        int count = 0;
        while (communicating)
        {
            //Just do it non-async in here
            communicating = pollData(sock);
            //Calls default ctor for CondenseData class
            CondenseData;
            this_thread::sleep_for(chrono::milliseconds(250));
            printf("Count: %d\n", count);
            //temp to not loop forever
            count++;
            if (count > 3)
                communicating = false;
        }
    }
    
    //close connection to FakeBot
    send(sock, "exiting", (int)strlen("exiting"), 0);
    if(sock != INVALID_SOCKET)
        closesocket(sock);
    WSACleanup();
    DAIMUBuilder->~IMUBuilder();

    return 0;
}

bool pollData(SOCKET sock)
{
    char recvbuf[DEFAULT_BUFLEN];
    int recvbuflen = DEFAULT_BUFLEN;
    string getMsg = "gRequest";
    if ((send(sock, getMsg.c_str(), (int)getMsg.length(), 0) == SOCKET_ERROR))
    {
        cerr << "send failed: " << WSAGetLastError() << endl;
        //not communicating
        return false;
    }
    //recieve, set IMUs data, set chrono to 250ms
    else
    {
        recv(sock, recvbuf, recvbuflen, 0);
        //Data is provided here so IMUs will be populated with data
        DAIMUBuilder->IMUPopulator(recvbuf);
        //still communicating
        return true;
    }
}