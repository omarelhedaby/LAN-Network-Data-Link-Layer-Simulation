//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
// 

#ifndef __PROJECT_NODE_H_
#define __PROJECT_NODE_H_

#include <math.h>
#include <cmath>
#include <bitset>
#include <fstream>
#include <vector>
#include "frame_m.h"

using namespace omnetpp;
using namespace std;
#define Ws 4 //Window Size
#define timestep 1.0
#define timeOut timestep + 5.0
#define mbits 2 //bits



/**
 * TODO - Generated class
 */
class Node : public cSimpleModule
{
  protected:
    int Sn;
    int Sf;
    int Sl;
    int R;
    int maxR ;
    int activeSession;
    int windowSize;
    bool ended;
    int nodeIndex;
    int delayProp ;
    int dupProp;
    int dropProb;
    int usefulBitsRecv;
    int totalGenerated ;
    int totalRetransmitted ;
    int totalDropped;
    int totalBitsSent;
    int corruptBitProb;
    vector<string> messageBuffer;
    Frame_Base * newMsg;
    Frame_Base * timeOutMsg ;
    Frame_Base * dupPointer;

    virtual void initialize();
    virtual void handleMessage(cMessage *msg);
    void writeToFile(string message);
    int readFromFile();
    void calculateStats();
    string corruptBit(string message, int percentage);
    void sendFrame(Frame_Base *frame,int delayProbability,int dataDropProbability,int dupProbability);
    int getRedundantCount(int m);
    string getMessageBits(string message);
    string getHammingMessage(string messageBits,int*parity,int r);
    int receiveCharCount(string recievedPayLoad);
    string getHammingMessageFromPayLoad(string recievedPayLoad);
    string getMessageFromHamming(string recievedHamming,int**parities);
    int* getHammingParity(string messageBits,int r);
    string getMessageFromBits(string messageBits,int charCount);
    string hammingCodeSender(string message,string finalMessage,int charCount);
    string hammingCodeReciever(string recievedPayLoad,int charCount);
    string extractMessageFromPayLoad(string recievedMessage);
    void wakeUpTransmission();
};

#endif
