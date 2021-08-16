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

void 
DrawLineBetweenNodes(ID3D11DeviceContext *RenderContext, node *Start, node *End)
{
    for(float T = 0.0f;
        T <= 1.0f;
        T += 0.01f)
    {
        v2 RectPos = LerpV2({Start->XPos, Start->YPos}, 
                            {End->XPos, End->YPos}, T);
        DrawRect(RenderContext, 
                 RectPos.X, RectPos.Y,
                 2.0f, 2.0f,
                 1.0f, 1.0f, 0.5f);
    } 
}

void
DrawLineBetweenNeighboursEx(ID3D11DeviceContext *RenderContext, graph *Graph)
{
    node *FirstNode = Graph->Nodes;
    FirstNode -= (Graph->NodesCount - 1);
    for(int NodeIndex = 0;
        NodeIndex < Graph->NodesCount;
        ++NodeIndex)
    {
        node *ActualNode = FirstNode + NodeIndex;
        
        node_list *ActualNodeList = ActualNode->LastNeighbour;
        while(ActualNodeList)
        {
            node *ActualNeighbour = ActualNodeList->Neighbour;
            DrawLineBetweenNodes(RenderContext, ActualNode, ActualNeighbour);
            ActualNodeList = ActualNodeList->PrevNeighbour;
        }     
    }
}


