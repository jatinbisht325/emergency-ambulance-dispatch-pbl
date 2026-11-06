#include "graph.h"

/*
 * ============================================================================
 * GRAPH CREATION & MEMORY MANAGEMENT
 * ============================================================================
 */

CityGraph* create_graph(int num_nodes) {
    if (num_nodes <= 0 || num_nodes > MAX_NODES) {
        fprintf(stderr, "[ERROR] create_graph: Invalid number of nodes (%d). Must be between 1 and %d.\n",
                num_nodes, MAX_NODES);
        return NULL;
    }

    CityGraph* g = (CityGraph*)malloc(sizeof(CityGraph));
    if (!g) {
        fprintf(stderr, "[FATAL] create_graph: Memory allocation failed for CityGraph.\n");
        return NULL;
    }

    g->num_nodes = num_nodes;
    g->num_edges = 0;

    for (int i = 0; i < MAX_NODES; i++) {
        g->adj_list[i] = NULL;
        g->landmarks[i].id = i;
        snprintf(g->landmarks[i].name, MAX_STR_LEN, "Intersection_%d", i);
        g->landmarks[i].is_hospital = false;
        g->landmarks[i].is_base = false;
    }

    return g;
}

void free_graph(CityGraph* g) {
    if (!g) return;

    /* Free all linked list edge nodes across every intersection */
    for (int i = 0; i < g->num_nodes; i++) {
        EdgeNode* curr = g->adj_list[i];
        while (curr != NULL) {
            EdgeNode* temp = curr;
            curr = curr->next;
            free(temp);
        }
        g->adj_list[i] = NULL;
    }

    free(g);
}

/*
 * ============================================================================
 * GRAPH MUTATION & CONFIGURATION
 * ============================================================================
 */

bool add_road(CityGraph* g, int u, int v, int distance, bool bidirectional) {
    if (!g) {
        fprintf(stderr, "[ERROR] add_road: Graph is NULL.\n");
        return false;
    }
    if (u < 0 || u >= g->num_nodes || v < 0 || v >= g->num_nodes) {
        fprintf(stderr, "[ERROR] add_road: Invalid nodes (%d -> %d). Out of bounds [0, %d).\n",
                u, v, g->num_nodes);
        return false;
    }
    if (distance < 0) {
        fprintf(stderr, "[ERROR] add_road: Road distance cannot be negative (%d).\n", distance);
        return false;
    }

    /* Edge u -> v */
    EdgeNode* edge1 = (EdgeNode*)malloc(sizeof(EdgeNode));
    if (!edge1) {
        fprintf(stderr, "[FATAL] add_road: Memory allocation failed for EdgeNode.\n");
        return false;
    }
    edge1->target_node = v;
    edge1->distance = distance;
    edge1->next = g->adj_list[u];
    g->adj_list[u] = edge1;
    g->num_edges++;

    /* If bidirectional road, add symmetric edge v -> u */
    if (bidirectional) {
        EdgeNode* edge2 = (EdgeNode*)malloc(sizeof(EdgeNode));
        if (!edge2) {
            fprintf(stderr, "[FATAL] add_road: Memory allocation failed for return EdgeNode.\n");
            return false;
        }
        edge2->target_node = u;
        edge2->distance = distance;
        edge2->next = g->adj_list[v];
        g->adj_list[v] = edge2;
    }

    return true;
}

void set_landmark(CityGraph* g, int id, const char* name, bool is_hospital, bool is_base) {
    if (!g || id < 0 || id >= g->num_nodes) {
        fprintf(stderr, "[ERROR] set_landmark: Invalid node ID (%d).\n", id);
        return;
    }

    g->landmarks[id].id = id;
    if (name && strlen(name) > 0) {
        strncpy(g->landmarks[id].name, name, MAX_STR_LEN - 1);
        g->landmarks[id].name[MAX_STR_LEN - 1] = '\0';
    }
    g->landmarks[id].is_hospital = is_hospital;
    g->landmarks[id].is_base = is_base;
}

int get_edge_distance(const CityGraph* g, int u, int v) {
    if (!g || u < 0 || u >= g->num_nodes || v < 0 || v >= g->num_nodes) {
        return -1;
    }

    EdgeNode* curr = g->adj_list[u];
    while (curr != NULL) {
        if (curr->target_node == v) {
            return curr->distance;
        }
        curr = curr->next;
    }
    return -1; // No direct road
}

/*
 * ============================================================================
 * FLAT-FILE MAP LOADER
 * Parses plain-text road network configuration
 * ============================================================================
 */

bool load_map_from_file(CityGraph* g, const char* filename) {
    if (!g || !filename) {
        fprintf(stderr, "[ERROR] load_map_from_file: Invalid graph or filename pointer.\n");
        return false;
    }

    FILE* fp = fopen(filename, "r");
    if (!fp) {
        fprintf(stderr, "[ERROR] load_map_from_file: Unable to open file '%s'.\n", filename);
        return false;
    }

    char line[256];
    int line_num = 0;

    while (fgets(line, sizeof(line), fp)) {
        line_num++;
        /* Strip leading whitespace */
        char* p = line;
        while (*p == ' ' || *p == '\t') p++;

        /* Skip empty lines and comment lines */
        if (*p == '\0' || *p == '\n' || *p == '\r' || *p == '#') {
            continue;
        }

        /* Directive 1: NODES <count> */
        if (strncmp(p, "NODES", 5) == 0) {
            int count = 0;
            if (sscanf(p, "NODES %d", &count) == 1) {
                if (count > MAX_NODES) {
                    fprintf(stderr, "[WARN] Line %d: Node count %d exceeds MAX_NODES %d. Capping.\n",
                            line_num, count, MAX_NODES);
                    count = MAX_NODES;
                }
                g->num_nodes = count;
            }
        }
        /* Directive 2: NODE <id> <is_hospital> <is_base> <name> */
        else if (strncmp(p, "NODE", 4) == 0) {
            int id = -1;
            int is_hosp = 0, is_base = 0;
            char name[MAX_STR_LEN] = {0};

            int matched = sscanf(p, "NODE %d %d %d %63[^\r\n]", &id, &is_hosp, &is_base, name);
            if (matched >= 3) {
                /* Trim trailing whitespace from name */
                int len = strlen(name);
                while (len > 0 && (name[len - 1] == ' ' || name[len - 1] == '\t' || name[len - 1] == '\r')) {
                    name[--len] = '\0';
                }
                set_landmark(g, id, name, is_hosp != 0, is_base != 0);
            }
        }
        /* Directive 3: ROAD <u> <v> <distance> <bidirectional> */
        else if (strncmp(p, "ROAD", 4) == 0) {
            int u = -1, v = -1, dist = -1, bidir = 1;
            int matched = sscanf(p, "ROAD %d %d %d %d", &u, &v, &dist, &bidir);
            if (matched >= 3) {
                add_road(g, u, v, dist, bidir != 0);
            }
        }
    }

    fclose(fp);
    return true;
}

/*
 * ============================================================================
 * DIJKSTRA'S SHORTEST PATH & PATH RECONSTRUCTION
 * ============================================================================
 */

RouteResult find_shortest_path(const CityGraph* g, int start_node, int end_node) {
    RouteResult result;
    result.path_length = 0;
    result.total_distance = -1;
    result.is_reachable = false;

    /* Defensive Bounds Checking */
    if (!g) {
        printf("[ERROR] find_shortest_path: Graph is NULL.\n");
        return result;
    }
    if (start_node < 0 || start_node >= g->num_nodes || end_node < 0 || end_node >= g->num_nodes) {
        printf("[NOTICE] find_shortest_path: Query node out of bounds (%d -> %d). Valid range: [0, %d).\n",
               start_node, end_node, g->num_nodes);
        return result;
    }

    /* Trivial Case: Start and End are identical */
    if (start_node == end_node) {
        result.path[0] = start_node;
        result.path_length = 1;
        result.total_distance = 0;
        result.is_reachable = true;
        return result;
    }

    int dist[MAX_NODES];
    bool visited[MAX_NODES];
    int parent[MAX_NODES];

    /* Initialization */
    for (int i = 0; i < g->num_nodes; i++) {
        dist[i] = INF;
        visited[i] = false;
        parent[i] = -1;
    }

    dist[start_node] = 0;

    /* Dijkstra Main Loop */
    for (int iter = 0; iter < g->num_nodes; iter++) {
        /* Find unvisited node with minimum tentative distance */
        int min_dist = INF;
        int u = -1;

        for (int i = 0; i < g->num_nodes; i++) {
            if (!visited[i] && dist[i] < min_dist) {
                min_dist = dist[i];
                u = i;
            }
        }

        /* If no unvisited reachable node remains, terminate */
        if (u == -1 || min_dist == INF) {
            break;
        }

        /* Early exit: once target node is settled, its shortest distance is guaranteed */
        if (u == end_node) {
            break;
        }

        visited[u] = true;

        /* Relax all outgoing edges from node u */
        EdgeNode* edge = g->adj_list[u];
        while (edge != NULL) {
            int v = edge->target_node;
            int weight = edge->distance;

            if (!visited[v] && dist[u] + weight < dist[v]) {
                dist[v] = dist[u] + weight;
                parent[v] = u;
            }
            edge = edge->next;
        }
    }

    /* Target unreachable check */
    if (dist[end_node] == INF) {
        return result; // is_reachable remains false
    }

    /*
     * Backtrack from end_node to start_node using parent[] pointers
     * to reconstruct the turn-by-turn route.
     */
    int temp_path[MAX_NODES];
    int count = 0;
    int curr = end_node;

    while (curr != -1 && count < MAX_NODES) {
        temp_path[count++] = curr;
        if (curr == start_node) {
            break;
        }
        curr = parent[curr];
    }

    /* Reverse temp_path into result.path to obtain forward driving order */
    for (int i = 0; i < count; i++) {
        result.path[i] = temp_path[count - 1 - i];
    }

    result.path_length = count;
    result.total_distance = dist[end_node];
    result.is_reachable = true;

    return result;
}

/*
 * ============================================================================
 * VISUALIZATION & OUTPUT FORMATTING
 * ============================================================================
 */

void print_graph(const CityGraph* g) {
    if (!g) return;

    printf("\n===============================================================\n");
    printf("         CITY ROAD NETWORK TOPOLOGY (Adjacency List)          \n");
    printf("===============================================================\n");
    printf(" Total Intersections: %d | Total Road Segments: %d\n", g->num_nodes, g->num_edges);
    printf("---------------------------------------------------------------\n");

    for (int i = 0; i < g->num_nodes; i++) {
        char tag[32] = "";
        if (g->landmarks[i].is_hospital && g->landmarks[i].is_base) {
            strcpy(tag, "[HOSPITAL + BASE]");
        } else if (g->landmarks[i].is_hospital) {
            strcpy(tag, "[HOSPITAL]");
        } else if (g->landmarks[i].is_base) {
            strcpy(tag, "[AMBULANCE BASE]");
        }

        printf(" Node %02d | %-26s %-18s\n", i, g->landmarks[i].name, tag);

        EdgeNode* edge = g->adj_list[i];
        if (!edge) {
            printf("   └── (No outgoing roads)\n");
        } else {
            while (edge != NULL) {
                printf("   ├──> Node %02d (%-22s) [%d km]\n",
                       edge->target_node,
                       g->landmarks[edge->target_node].name,
                       edge->distance);
                edge = edge->next;
            }
        }
    }
    printf("===============================================================\n\n");
}

void print_route(const CityGraph* g, const RouteResult* route) {
    if (!g || !route) return;

    if (!route->is_reachable) {
        printf("[ROUTING ALERT] Destination is UNREACHABLE via road network!\n");
        return;
    }

    printf("\n+-------------------------------------------------------------+\n");
    printf("|                 OPTIMIZED DISPATCH ROUTE                    |\n");
    printf("+-------------------------------------------------------------+\n");
    printf(" Total Driving Distance : %d km\n", route->total_distance);
    printf(" Total Intersections    : %d stops\n", route->path_length);
    printf(" Turn-by-Turn Waypoints :\n");

    for (int i = 0; i < route->path_length; i++) {
        int node = route->path[i];
        printf("   [%02d] %-28s", node, g->landmarks[node].name);

        if (i < route->path_length - 1) {
            int next_node = route->path[i + 1];
            int seg_dist = get_edge_distance(g, node, next_node);
            printf("  ──(%d km)──> \n", seg_dist);
        } else {
            printf("  [DESTINATION]\n");
        }
    }
    printf("+-------------------------------------------------------------+\n\n");
}
