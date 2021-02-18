#get file object reference to the file
import os
import re

i = 0

total_generated_msgs = 0 
total_dropped_msgs = 0
total_retr = 0
total_utilization = 0
Nodes = set()
while os.path.exists("SessionLOG"+str(i)+".txt"):
    file = open("SessionLOG"+str(i)+".txt", "r")
    #read content of file to string
    data = file.read()
    #get number of occurrences of the substring in the string
    GeneratedMSGs = data.count("Sending")
    DroppedMSGs = data.count("Dropped")
    Nodes = list(set(re.findall('Node [0-9]+',data)))
    retransmitted0 = data.count(Nodes[0]+" Sending") + data.count(Nodes[1]+" : Recieved Wrong") - data.count(Nodes[0]+' :Duplicate')#Dropped and wrong are the ones retransmitted
    retransmitted1 = data.count(Nodes[1]+" Sending") + data.count(Nodes[0]+" : Recieved Wrong") - data.count(Nodes[1]+' :Duplicate')#sent without dropped frames

    total_generated_msgs += GeneratedMSGs
    total_dropped_msgs += DroppedMSGs
    total_retr += (retransmitted0 + retransmitted1)
    
    total_useful0 =  re.findall('Ended session with useful msgs received = [0-9]+',data)[0].split(" = ")[1]
    total_useful1 = re.findall('End Hub with useful msgs received = [0-9]+',data)[0].split(" = ")[1]
    
    total_sent0 = re.findall('total msg sent [0-9]+',data)[0].split(" ")[-1]
    total_sent1 = re.findall('[0-9]+ Messages sent',data)[0].split(" ")[0]

    total_utilization += int(total_useful0)/(int(total_sent1) + int(total_useful0))
    total_utilization += int(total_useful1) / (int(total_sent0) + int(total_useful1))

    i += 1
if i != 0:
    total_utilization /= 2*(i)
    
print("===========================STATS FOR  "+str(i)+ " SESSIONS===========================")

print('Number of generated messages'+' is:', total_generated_msgs)
print('Number of dropped messages'+' is:', total_dropped_msgs)
print("Total retransmitted is : "+str(total_retr))

print("===============================================================")
