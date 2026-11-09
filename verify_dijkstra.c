#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

#define MAX_NODES 100
#define INF 999999

// Function to find the vertex with the minimum distance value
int getMinDistanceVertex(int distance[], int visited[], int n) {
    int min = INF, min_index = -1;
    for (int v = 0; v < n; v++) {
        if (!visited[v] && distance[v] <= min) {
            min = distance[v];
            min_index = v;
        }
    }
    return min_index;
}

// Function to print the computed shortest path recursively/iteratively
void printPath(int parent[], int j) {
    if (parent[j] == -1)
        return;
    printPath(parent, parent[j]);
    printf(" -> %d", j);
}

// Core Dijkstra's Algorithm implementation for verification
void runDijkstraVerification(int n, int graph[MAX_NODES][MAX_NODES], int src) {
    int distance[MAX_NODES];
    int visited[MAX_NODES];
    int parent[MAX_NODES];

    for (int i = 0; i < n; i++) {
        distance[i] = INF;
        visited[i] = 0;
        parent[i] = -1;
    }

    distance[src] = 0;

    for (int count = 0; count < n - 1; count++) {
        int u = getMinDistanceVertex(distance, visited, n);
        if (u == -1) break;
        
        visited[u] = 1;

        for (int v = 0; v < n; v++) {
            if (!visited[v] && graph[u][v] && distance[u] != INF && 
                distance[u] + graph[u][v] < distance[v]) {
                parent[v] = u;
                distance[v] = distance[u] + graph[u][v];
            }
        }
    }

    // Print Verification Results
    printf("\n--- DIJKSTRA ALGORITHM VERIFICATION REPORT ---\n");
    printf("Source Vertex: %d\n", src);
    printf("Vertex\t\tDistance from Source\tPath\n");
    for (int i = 0; i < n; i++) {
        if (i == src) continue;
        printf("%d \t\t %d \t\t\t %d", i, distance[i], src);
        printPath(parent, i);
        printf("\n");
    }
    printf("-----------------------------------------------\n");
}

int main() {
    FILE *file = fopen("city_map.txt", "r");
    if (!file) {
        printf("Error: Could not open city_map.txt. Make sure it is in the working directory.\n");
        
        // Fallback to a hardcoded test graph if city_map.txt isn't present yet
        printf("Running fallback matrix verification...\n");
        int n = 5;
        int sampleGraph[MAX_NODES][MAX_NODES] = {
            {0, 10, 0, 30, 100},
            {10, 0, 50, 0, 0},
            {0, 50, 0, 20, 10},
            {30, 0, 20, 0, 60},
            {100, 0, 10, 60, 0}
        };
        runDijkstraVerification(n, sampleGraph, 0);
        return 0;
    }

    int n;
    if (fscanf(file, "%d", &n) != 1) {
        printf("Error reading number of vertices from city_map.txt\n");
        fclose(file);
        return 1;
    }

    int graph[MAX_NODES][MAX_NODES];
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            fscanf(file, "%d", &graph[i][j]);
        }
    }
    fclose(file);

    // Run verification starting from node 0
    runDijkstraVerification(n, graph, 0);

    return 0;
}
