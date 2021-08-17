#ifndef ASTAR_H
#define ASTAR_H

struct node_list;

struct node
{
    float XPos; 
    float YPos;
    float GlobalDistance;
    float LocalDistance;
    bool Visited;
    node *Parent; 
    node_list *LastNeighbour;
};

// NOTE(manuto): link-list TEST.
struct node_list
{
    node *Neighbour;
    node_list *PrevNeighbour;
};

struct mouse_neighbour_handler
{
    node *From;
    node *To;
};

struct graph
{
    node *Start;
    node *End;

    node **NotTestedNodes;
    int NotTestedNodesCount;

    node *Nodes;
    int NodesCount;
};

#endif
