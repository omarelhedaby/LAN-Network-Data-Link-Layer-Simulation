package Project;

@license(LGPL);

import ned.IdealChannel;

//
// TODO documentation
//
network Network
{
    parameters:
        int n = default(5);
        @display("bgb=498.19,209.44");
    submodules:
        hub: Hub {
            parameters:
                n = n;
        }
        nodes[n]: Node;
    connections:
        for i=0..sizeof(nodes)-1 {
            nodes[i].outs++ --> IdealChannel --> hub.ins++;
            nodes[i].ins++ <-- IdealChannel <-- hub.outs++;
        }
}

