#ifndef ASTAR_H
#define ASTAR_H

struct node_list;

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

    node_list *LastNeighbour;
};

// NOTE(manuto): link-list TEST.
struct node_list
{
    node *Neighbour;
    node_list *PrevNeighbour;
};

struct graph
{
    node *Nodes;
    int NodesCount;
};

#endif
