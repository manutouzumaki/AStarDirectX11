node *
AddNodeToGraph(graph *Graph, float X, float Y, arena *NodeArena)
{
    Graph->Nodes = (node *)PushStruct(NodeArena, node);
    Graph->Nodes->XPos = X;
    Graph->Nodes->YPos = Y;
    Graph->Nodes->NumberOfNeighbours = 0;
    ++Graph->NodesCount; 
    return Graph->Nodes;
}

void
AddNeighboursToNode(node *Node, node *NeighbourNode, arena *NeighboursArena)
{
    Node->Neighbours = (node **)PushStruct(NeighboursArena, node *);
    *Node->Neighbours = NeighbourNode;
    ++Node->NumberOfNeighbours;    
}





