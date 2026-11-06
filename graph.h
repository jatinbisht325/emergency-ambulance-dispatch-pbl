#ifndef GRAPH_H
#define GRAPH_H

#include "common.h"

/*
 * ============================================================================
 * ADJACENCY LIST EDGE NODE
 * Represents a directed or undirected road connecting two intersections
 * ============================================================================
 */
typedef struct EdgeNode {
    int target_node;         // Destination intersection ID
    int distance;            // Road weight (kilometers or minutes)
    struct EdgeNode* next;   // Pointer to next neighbor in adjacency list
} EdgeNode;

/*
 * ============================================================================
 * LANDMARK / INTERSECTION INFORMATION
 * Describes each node on the city map
 * ============================================================================
 */
typedef struct {
    int id;                  // Node index (0 to num_nodes - 1)
    char name[MAX_STR_LEN];  // Human-readable name (e.g. "Bhimtal Base A", "Civil Hospital")
    bool is_hospital;        // True if this intersection hosts a medical center
    bool is_base;            // True if this intersection hosts an ambulance station
} Landmark;

/*
 * ============================================================================
 * CITY ROAD NETWORK GRAPH
 * Stored as an Adjacency List for O(V + E) sparse space complexity
 * ============================================================================
 */
typedef struct {
    int num_nodes;                   // Total number of intersections
    int num_edges;                   // Total number of unique road segments
    EdgeNode* adj_list[MAX_NODES];   // Array of head pointers for each node's neighbor list
    Landmark landmarks[MAX_NODES];   // Metadata for each intersection
} CityGraph;

/*
 * ============================================================================
 * GRAPH CREATION & MEMORY MANAGEMENT
 * ============================================================================
 */
CityGraph* create_graph(int num_nodes);
void free_graph(CityGraph* g);

/*
 * ============================================================================
 * GRAPH MUTATION & FILE LOADING
 * ============================================================================
 */
bool add_road(CityGraph* g, int u, int v, int distance, bool bidirectional);
void set_landmark(CityGraph* g, int id, const char* name, bool is_hospital, bool is_base);
bool load_map_from_file(CityGraph* g, const char* filename);

/*
 * ============================================================================
 * DIJKSTRA'S SHORTEST PATH & PATH RECONSTRUCTION
 * ============================================================================
 */
RouteResult find_shortest_path(const CityGraph* g, int start_node, int end_node);
int get_edge_distance(const CityGraph* g, int u, int v);

/*
 * ============================================================================
 * VISUALIZATION & DEBUGGING
 * ============================================================================
 */
void print_graph(const CityGraph* g);
void print_route(const CityGraph* g, const RouteResult* route);

#endif /* GRAPH_H */
