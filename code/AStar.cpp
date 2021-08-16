node *
AddNodeToGraph(graph *Graph, float X, float Y, arena *NodeArena)
{
    Graph->Nodes = (node *)PushStruct(NodeArena, node);
    Graph->Nodes->XPos = X;
    Graph->Nodes->YPos = Y;
    ++Graph->NodesCount; 
    return Graph->Nodes;
}

void 
AddNodeToList(node *Node, node *NewNode, arena *NodeListArena)
{
    node_list *NewNodeList = (node_list *)PushStruct(NodeListArena, node_list);
    NewNodeList->Neighbour = NewNode;
    NewNodeList->PrevNeighbour = Node->LastNeighbour;
    Node->LastNeighbour = NewNodeList;
}


