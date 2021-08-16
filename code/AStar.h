#ifndef ASTAR_H
#define ASTAR_H

struct node
{
    int Id;
    float XPos; 
    float YPos;
    float GlobalDistance;
    float LoacalDistance;
    node *Parent; 
    node **Neighbours;
    int NumberOfNeighbours;
};

struct graph
{
    node *Nodes;
    int NodesCount;
};

#endif
