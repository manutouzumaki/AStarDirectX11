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
NodesPopBack(arena *Arena, int *ElementCount, node ***NotTestedNodes)
{
    --(*NotTestedNodes);
    --(*ElementCount); 
    Arena->Use -= sizeof(node **);
}

void
NodesPopFront(arena *Arena, int *ElementCount, node ***NotTestedNodes)
{
    node **FirstNode = *NotTestedNodes;
    FirstNode -= (*ElementCount - 1);

    for(int NodeIndex = 0;
        NodeIndex < (*ElementCount - 1);
        ++NodeIndex)
    {
        node **ActualNode = FirstNode + NodeIndex;
        node **NextNode = FirstNode + NodeIndex + 1;
        *ActualNode = *NextNode;
    }
    --(*NotTestedNodes);
    --(*ElementCount);
    Arena->Use -= sizeof(node **);
}

void
NodesSort(arena *Arena, int *ElementCount, node **NotTestedNodes)
{
    node **FirstNode = NotTestedNodes;
    FirstNode -= (*ElementCount - 1);

    for(int I = 0;
        I < *ElementCount;
        ++I)
    {
        for(int J = I;
            J < *ElementCount;
            ++J)
        {
            node **ActualNode = FirstNode + I;
            node **TargetNode = FirstNode + J;
            if((*ActualNode)->GlobalDistance > (*TargetNode)->GlobalDistance)
            {
                node *Temp = *ActualNode;
                *ActualNode = *TargetNode;
                *TargetNode = Temp;
            } 
        }
    }
}
node *
NodeFront(node **NotTestedNodes, int NotTestedNodesCount)
{
    node **FirstNode = NotTestedNodes;
    FirstNode -= (NotTestedNodesCount - 1);
    return (*FirstNode);
}

bool
AStar(graph *Graph, arena *AStarArena)
{
    ClearArena(AStarArena);
    Graph->NotTestedNodesCount = 0;

    node *FirstNode = Graph->Nodes;
    FirstNode -= (Graph->NodesCount - 1);
    for(int NodeIndex = 0;
        NodeIndex < Graph->NodesCount;
        ++NodeIndex)
    {
        node *ActualNode = FirstNode + NodeIndex;
        ActualNode->Visited = false;
        ActualNode->Parent = NULL;
        ActualNode->GlobalDistance = INFINITY;
        ActualNode->LocalDistance = INFINITY;
    }
 
    node *CurrentNode = Graph->Start;

    v2 StartPos = {Graph->Start->XPos, Graph->Start->YPos}; 
    v2 EndPos = {Graph->End->XPos, Graph->End->YPos}; 
    Graph->Start->LocalDistance = 0.0f;
    Graph->Start->GlobalDistance = LengthV2(StartPos - EndPos);

    Graph->NotTestedNodes = (node **)PushStruct(AStarArena, node *);
    *Graph->NotTestedNodes = Graph->Start;
    ++Graph->NotTestedNodesCount;
    

    while(Graph->NotTestedNodesCount > 0 && CurrentNode != Graph->End)
    {
        NodesSort(AStarArena, &Graph->NotTestedNodesCount, Graph->NotTestedNodes);

        while(Graph->NotTestedNodesCount > 0 && NodeFront(Graph->NotTestedNodes, Graph->NotTestedNodesCount)->Visited)
        {
            NodesPopFront(AStarArena, &Graph->NotTestedNodesCount, &Graph->NotTestedNodes);
        }
        if(Graph->NotTestedNodes <= 0)
        {
            break;
        }

        CurrentNode = NodeFront(Graph->NotTestedNodes, Graph->NotTestedNodesCount);
        CurrentNode->Visited = true;

        node_list *CurrentNodeList = CurrentNode->LastNeighbour;
        while(CurrentNodeList)
        {
            node *CurrentNeighbour = CurrentNodeList->Neighbour;

            if(!CurrentNeighbour->Visited)
            {
                Graph->NotTestedNodes = (node **)PushStruct(AStarArena, node *);
                *Graph->NotTestedNodes = CurrentNeighbour;
                ++Graph->NotTestedNodesCount;
            }
            
            v2 CurrentNodePos = {CurrentNode->XPos, CurrentNode->YPos};
            v2 CurrentNeighbourPos = {CurrentNeighbour->XPos, CurrentNeighbour->YPos};
            float PossiblyLowerDist = CurrentNode->LocalDistance + LengthV2(CurrentNodePos - CurrentNeighbourPos);

            if(PossiblyLowerDist < CurrentNeighbour->LocalDistance)
            {
                CurrentNeighbour->Parent = CurrentNode;
                CurrentNeighbour->LocalDistance = PossiblyLowerDist;
                CurrentNeighbour->GlobalDistance = CurrentNeighbour->LocalDistance + LengthV2(CurrentNeighbourPos - EndPos);
            }

            CurrentNodeList = CurrentNodeList->PrevNeighbour;
        }     
    } 
    return true;
}

void 
DrawLineBetweenNodes(ID3D11DeviceContext *RenderContext, node *Start, node *End, float R, float G, float B)
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
                 R, G, B);
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
            DrawLineBetweenNodes(RenderContext, ActualNode, ActualNeighbour, 1.0f, 1.0f, 0.5f);
            ActualNodeList = ActualNodeList->PrevNeighbour;
        }     
    }
}

void 
DrawShotestPath(ID3D11DeviceContext *RenderContext, graph *Graph)
{
    node *ActualNode = Graph->End;
    while(ActualNode->Parent)
    {
        node *To = ActualNode;
        node *From = ActualNode->Parent;
        DrawLineBetweenNodes(RenderContext, To, From, 1.0f, 0.0f, 0.6f);
        ActualNode = ActualNode->Parent;
    } 
}
