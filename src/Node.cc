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

#include "Node.h"

Define_Module(Node);

void Node::initialize()
{
    usefulBitsRecv = 0;
    totalBitsSent = 0;
    totalGenerated = 0;
    totalRetransmitted = 0;
    totalDropped = 0;
    delayProp = par("delayRate").doubleValue();
    dupProp = par("duplicateRate").doubleValue();
    dropProb = par("dropRate").doubleValue();
    corruptBitProb = par("corruptBitRate").doubleValue();
   //Window Size = 3
    nodeIndex=this->getId()-3;

    /*messageBuffer = new string[windowSize+1];
    for (int i = 0 ; i < windowSize + 1 ; i++){
        messageBuffer[i] = "Wassap" + to_string(i);
    }*/
    maxR = readFromFile();
    windowSize = maxR-1;

    ended = false;
}

void Node::handleMessage(cMessage *msg)
{
    Frame_Base *mmsg = check_and_cast<Frame_Base *>(msg);
    newMsg=new Frame_Base();
    int type=mmsg->getType();
    if(mmsg->isSelfMessage())
    {
        if(ended){
            return;
        }
        if(type==10)
        {
            //Timeout Code
            EV<<"Time "<<simTime().dbl()<<": Node "<<nodeIndex<<" : Timeout to Frame "<<mmsg->getFrameNum()<<endl;
            writeToFile("Time "+to_string(simTime().dbl())+": Node "+to_string(nodeIndex)+" : Timeout to Frame "+to_string(mmsg->getFrameNum()));
            totalRetransmitted += Sn - Sf;
            Sn = Sf; //return Sn to beginning of window
            wakeUpTransmission();
        }
        else if(type==0) //continue sending
        {
            if(!ended)
            {
                send(mmsg->dup(),"outs",0); //to actually send the frame
                if(Sn<=Sl)
                {
                    wakeUpTransmission();
                }
                //If Sn>Sl the scheduling will stop until the window shifts
            }
        }
    }
    else
    {

        string message;
        if(type==2) //start transmission
        {
            totalGenerated = 0;
            totalRetransmitted = 0;
            totalDropped = 0;
            usefulBitsRecv = 0;
            totalBitsSent = 0;
            activeSession = mmsg->getSessionNumber();
            Sn = 0;
            Sf = 0;
            Sl = windowSize-1;
            R = 0;
            ended=false;
            message=messageBuffer[Sn]+char(R);
            newMsg->setType(0); //data
            newMsg->setFrameNum(Sn);
            newMsg->setSessionNumber(activeSession);

            int charCount = message.size();
            bitset<8> charCountBits(charCount);
            string payLoad="";
            payLoad+=charCountBits.to_string(); //first byte is the char count
            string finalMessage=corruptBit(message,corruptBitProb);
            newMsg->setName(finalMessage.c_str());
            string hammingMessage=hammingCodeSender(message,finalMessage,charCount);
            payLoad+=hammingMessage;
            newMsg->setPayLoad(payLoad.c_str());
            Sn += 1;
            sendFrame(newMsg,delayProp,dropProb,dupProp);
            totalGenerated +=1;
            timeOutMsg = new Frame_Base("");
            timeOutMsg->setType(10);
            //Adds new timeOut timer
            scheduleAt(simTime() + timeOut, timeOutMsg);
        }
        else //recieved from other node
        {
            if(ended){
                return;
            }

            if(type==5)
            {
                writeToFile("Time "+to_string(simTime().dbl())+" : Node "+to_string(nodeIndex)+" : Sent End Hub with useful msgs received = " + to_string(usefulBitsRecv)  +" and "+ to_string(totalBitsSent) + " Messages sent");
                mmsg->setSessionNumber(activeSession);
                //END session type
                calculateStats();
                cancelEvent(timeOutMsg);
                mmsg->setName("End Session Hub");
                mmsg->setType(3);
                send(mmsg->dup(),"outs",0); //to actually send the frame
                ended = true;
                return;
            }

            /*-----------Recieve Code Block ------------*/


            int frameNum=mmsg->getFrameNum();
            string recievedPayLoad=mmsg->getPayLoad();
            int charCount=receiveCharCount(recievedPayLoad);
            string recievedMessage= hammingCodeReciever(recievedPayLoad,charCount);
            string message=extractMessageFromPayLoad(recievedMessage);
            int ack=(int)recievedMessage[recievedMessage.size()-1]; //i will slide window with this
            if(frameNum==R)
            {
                usefulBitsRecv += recievedMessage.size();
                R++;
                EV<<"Time "<<simTime().dbl()<<": Node "<<nodeIndex<<" : Recieved Expected Frame "<<mmsg->getFrameNum()<<" and the next Expected Frame is "<<R<<" Recieved Ack was "<<ack<<endl;
                writeToFile("Time "+to_string(simTime().dbl())+" : Node "+to_string(nodeIndex)+" : Recieved Expected Frame "+to_string(mmsg->getFrameNum())+" and the next Expected Frame is "+to_string(R)+" Recieved Ack was "+to_string(ack));
                bubble(((string)"Recieved "+message +(string)" Next Expected Frame is "+to_string(R)+" Recieved Ack was "+to_string(ack)).c_str());
            }
            else
            {
                bubble(("Not the Expected Frame "+(string)"The Expected Frame was "+to_string(R)+" Recieved Ack was "+to_string(ack)).c_str());
                EV<<"Time "<<simTime().dbl()<<": Node "<<nodeIndex<<" : Recieved Wrong Frame "<<mmsg->getFrameNum()<<" the Expected Frame was "<<R<<" Recieved Ack was "<<ack<<endl;
                writeToFile("Time "+to_string(simTime().dbl())+" : Node "+to_string(nodeIndex)+" : Recieved Wrong Frame "+to_string(mmsg->getFrameNum())+" the Expected Frame was "+to_string(R)+" Recieved Ack was "+to_string(ack));
            }

            //max Sn Sl at any time , max ack Sl+1
            if(ack>Sf && ack<=windowSize+1)
            {
                Sl = (Sl + ack-Sf >windowSize)?(windowSize):Sl + ack-Sf;
                Sf = ack;
                if(ack==windowSize+1)
                {
                    mmsg->setName("End Session");
                    mmsg->setType(5);
                    mmsg->setSessionNumber(activeSession);
                    send(mmsg->dup(),"outs",0);
                    calculateStats();
                    cancelEvent(timeOutMsg);
                    mmsg->setName("End Session Hub");
                    EV<<"Time "<<simTime().dbl()<<": Node "<<nodeIndex<<" : Sent all frames & Ended session "<<endl;
                    writeToFile("Time "+to_string(simTime().dbl())+" : Node "+to_string(nodeIndex)+" : Sent all frames & Ended session with useful msgs received = " + to_string(usefulBitsRecv) + " and total msg sent " + to_string(totalBitsSent));
                    Sn = 0;
                    Sf = 0;
                    ended=true;
                    Sl = windowSize-1;
                    R = 0;
                }
                else if(Sn<=windowSize)
                {
                    wakeUpTransmission();
                }

            }
            /*-----------Sender Code End----------------*/
        }
    }
    delete(mmsg);

}
void Node::sendFrame(Frame_Base *frame,int delayProbability,int dataDropProbability,int dupProbability)
{
    string recievedPayLoad=frame->getPayLoad();
    int charCount=receiveCharCount(recievedPayLoad);
    string recievedMessage= hammingCodeReciever(recievedPayLoad,charCount);
    totalBitsSent += recievedMessage.size();
    totalGenerated += 1;
    int randDrop=uniform(0,1)*10;
    if(randDrop<(int)(dataDropProbability/10)) //message will not be sent
    {
        totalDropped += 1;
        EV<<"Time "<<simTime().dbl()<<": Node "<<nodeIndex<<"Sending of "<<frame->getFrameNum()<<" was Dropped"<<endl;
        writeToFile("Time "+to_string(simTime().dbl())+": Node "+to_string(nodeIndex)+" Sending of "+to_string(frame->getFrameNum())+" was Dropped");
        return;
    }

    int rand=uniform(0,1)*10;
    double timeStepAfter=timestep;
    double randFactor = uniform(0.01,0.1);
    if(rand<(int)(delayProbability/10))
    {

        float simDelay=uniform(1.5,4); //delay from 0.1 to 1
        timeStepAfter+=simDelay;
        scheduleAt(simTime()+timeStepAfter,frame); //1s timeout
        EV<<"Time "<<simTime().dbl()<<": Node "<<nodeIndex<<" : Delayed Sending of Frame "<<frame->getFrameNum()<<" with delay of  "<<simDelay<<endl;
        writeToFile("Time "+to_string(simTime().dbl())+": Node "+to_string(nodeIndex)+" : Delayed Sending of Frame "+to_string(frame->getFrameNum()) + " with delay of  "+to_string(simDelay));
    }
    else
    {
        scheduleAt(simTime()+timeStepAfter+randFactor,frame); //1s timeout
        EV<<"Time "<<simTime()<<": Node "<<nodeIndex<<" : Sending of Frame "<<frame->getFrameNum()<<" Sent Without Delay "<<endl;
        writeToFile("Time "+to_string(simTime().dbl())+": Node "+to_string(nodeIndex)+" : Sending of Frame "+to_string(frame->getFrameNum())+" Sent Without Delay ");
    }

    int randDup=uniform(0,1)*10;

    if(randDup<(int)(dupProbability/10))
    {
        totalGenerated += 1;
        totalBitsSent += recievedMessage.size();
        double timeAfter=timeStepAfter+uniform(0.01,0.1);
        dupPointer=frame->dup();
        scheduleAt(simTime()+timeAfter,dupPointer); //1s timeout
        EV<<"Time "<<simTime().dbl()<<": Node "<<nodeIndex<<" :Duplicate Sending of Frame "<<frame->getFrameNum()<<endl;
        writeToFile("Time "+to_string(simTime().dbl())+": Node "+to_string(nodeIndex)+" :Duplicate Sending of Frame "+to_string(frame->getFrameNum()));
    }
}
string Node::corruptBit(string message, int percentage)
{
    int rand=uniform(0,1)*10;
    if(rand<(int)(percentage/10)) // probabilty of 30% message will be corrupted
    {
           int charIndex=uniform(0,1)*message.size(); //to choose random char to corrupt
           int bitIndex=uniform(0,1)*8; // to choose a bit to corrupt
           message[charIndex]^=(1<<bitIndex);
    }
    return message;
}
int Node::getRedundantCount(int m)
{
    //(m+r+1) <= 2^r
    int r=0;
    while(!(m+r+1<=(int)pow(2,r)))
    {
        r+=1;
    }
    return r;
}
string Node::getMessageBits(string message)
{
    string messageBits="";
    for(int i=0;i<message.size();i++)
    {
        bitset<8> charBits (message[i]);
        messageBits+=charBits.to_string();
    }
    return messageBits;
}
string Node:: getHammingMessage(string messageBits,int*parity,int r)
{
    string hammingMessage="";
    int m=messageBits.size();
    int bitIndex=1;
    int parityIndex=0;
    int messageIndex=0;
    while(bitIndex<m+r+1)
    {
        if((int)log2(bitIndex)!=(float)log2(bitIndex)) //this is not a parity bit
        {
            hammingMessage+=messageBits[messageIndex++];
        }
        else
        {
            hammingMessage+=(parity[parityIndex++]==1)?"1":"0";
        }
        bitIndex++;
    }
    return hammingMessage;
}

int Node::receiveCharCount(string recievedPayLoad)
{
    //char count is the first 8 bits
    int charCount=0;
    int power=0;
    for(int i=7;i>=0;i--)
    {
        charCount+=pow(2,power)*(recievedPayLoad[i]=='1'?1:0);
        power++;
    }
    return charCount;
}
string Node:: getHammingMessageFromPayLoad(string recievedPayLoad)
{
    string message="";
    for(int i=8;i<recievedPayLoad.size();i++)
    {
        message+=recievedPayLoad[i];
    }
    return message;
}

string Node::getMessageFromHamming(string recievedHamming,int**parities)
{
    int bitIndex=1;
    string message="";

    while(bitIndex<recievedHamming.size()+1) //m+r+1
    {
        if((int)log2(bitIndex)!=(float)log2(bitIndex)) //this is not a parity bit
        {
            message+=recievedHamming[bitIndex-1];
        }
        else
        {
           ((*parities)[(int)log2(bitIndex)])^=((recievedHamming[bitIndex-1]=='1')?1:0);
        }
        bitIndex++;
    }
    return message;
}
int* Node:: getHammingParity(string messageBits,int r)
{
    int*parityArray=new int[r](); //all initialized with 0
    int bitIndex=1;
    int messageIndex=0;
    int m=messageBits.size();
    while(bitIndex<m+r+1)
    {
        if((int)log2(bitIndex)!=(float)log2(bitIndex)) //this is not a parity bit
        {
            for(int parityIndex=0;parityIndex<r;parityIndex++)
            {
                if(((int)pow(2,parityIndex) & bitIndex)>0)
                {
                    parityArray[parityIndex]^=(messageBits[messageIndex]=='1')?1:0; //xor with current parity
                }
            }
            messageIndex++;
        }
        bitIndex++;
    }
    return parityArray;
}
string Node:: getMessageFromBits(string messageBits,int charCount)
{
    string message="";
    for(int i=0;i<charCount;i++)
    {
        char character=0;
        int power=0;
        for(int j=7;j>=0;j--)
        {
            character+=pow(2,power)*(messageBits[i*8+j]=='1'?1:0);
            power++;
        }
        message+=character;
    }
    return message;
}
string Node:: hammingCodeReciever(string recievedPayLoad,int charCount)
{
    int r=getRedundantCount(charCount*8);
    int*parities=new int[r]();
    string recievedHamming=getHammingMessageFromPayLoad(recievedPayLoad);
    string messageBits=getMessageFromHamming(recievedHamming,&parities);
    int*newparities=getHammingParity(messageBits,r);
    int IndextoCorrect=0;
    for(int i=0;i<r;i++)
    {
        newparities[i]^=parities[i];
        IndextoCorrect+=(int)pow(2,i)*newparities[i];
    }
    if(IndextoCorrect>0)
    {
        recievedHamming[IndextoCorrect-1]=((recievedHamming[IndextoCorrect-1]=='1')?'0':'1'); //corrected hamming
        messageBits=getMessageFromHamming(recievedHamming,&parities);
    }
    string recievedMessage=getMessageFromBits(messageBits,charCount);
    return recievedMessage;
}
string Node:: hammingCodeSender(string message,string finalMessage,int charCount)
{
    int r=getRedundantCount(charCount*8);
    string messageBits=getMessageBits(message);
    int*parity=getHammingParity(messageBits,r);
    string hammingMessage=getHammingMessage(getMessageBits(finalMessage),parity,r);
    return hammingMessage;
}
string Node::extractMessageFromPayLoad(string recievedMessage)
{
    string message="";
    for(int i=0;i<recievedMessage.size()-1;i++)
    {
        message+=recievedMessage[i];
    }
    return message;
}
void Node::wakeUpTransmission()
{
    string message=messageBuffer[Sn]+char(R);
    newMsg->setType(0); //data
    newMsg->setFrameNum(Sn);
    newMsg->setSessionNumber(activeSession);
    int charCount = message.size();
    bitset<8> charCountBits(charCount);
    string payLoad="";
    payLoad+=charCountBits.to_string(); //first byte is the char count
    string finalMessage=corruptBit(message,corruptBitProb);
    newMsg->setName(finalMessage.c_str());
    string hammingMessage=hammingCodeSender(message,finalMessage,charCount);
    payLoad+=hammingMessage;
    newMsg->setPayLoad(payLoad.c_str());
    sendFrame(newMsg,delayProp,dropProb,dupProp);
    Sn += 1;
    if(timeOutMsg->isScheduled() && timeOutMsg->isSelfMessage() ){
        cancelEvent(timeOutMsg);
    }
    timeOutMsg = new Frame_Base("");
    timeOutMsg->setFrameNum(Sn-1);

    timeOutMsg->setType(10);
    //Adds new timeOut timer
    scheduleAt(simTime() + timeOut, timeOutMsg);
}
void Node::writeToFile(string message){
    ofstream myfile;
    myfile.open ("SessionLOG"+to_string(activeSession)+".txt",std::ios_base::app);
    myfile << message+"\n";
    myfile.close();
}
int Node::readFromFile(){
    string fileName = to_string(rand()%10)+".txt";

    int number_of_lines = 0;
    std::string line;
    std::ifstream myfile(fileName);
    while (std::getline(myfile, line))
    {
        number_of_lines++;
        EV<<line<<endl;
        messageBuffer.push_back(line);
    }
    myfile.close();
    return number_of_lines;
}
void Node::calculateStats(){
    ofstream myfile;
    myfile.open ("Stats"+to_string(activeSession)+".txt",std::ios_base::app);
    myfile <<    to_string(usefulBitsRecv) + " "+to_string(totalBitsSent)+ " "+to_string(totalGenerated) + " "+to_string(totalRetransmitted)+" "+ to_string(totalDropped) + "\n";
    myfile.close();
}
