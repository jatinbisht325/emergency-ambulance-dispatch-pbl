#!/usr/bin/env python3
"""
Independent Verification Script for City Graph & Dijkstra Engine.
Reads city_map.txt and computes shortest paths using Dijkstra's algorithm.
Used to cross-validate C implementation against mathematical ground truth.
"""

import heapq
import sys

def load_map(filename):
    landmarks = {}
    graph = {}
    with open(filename, 'r') as f:
        for line in f:
            line = line.strip()
            if not line or line.startswith('#'):
                continue
            parts = line.split(maxsplit=4)
            if parts[0] == 'NODE':
                node_id = int(parts[1])
                is_hosp = int(parts[2])
                is_base = int(parts[3])
                name = parts[4] if len(parts) > 4 else f"Node_{node_id}"
                landmarks[node_id] = {'name': name, 'is_hosp': is_hosp, 'is_base': is_base}
                if node_id not in graph:
                    graph[node_id] = []
            elif parts[0] == 'ROAD':
                u = int(parts[1])
                v = int(parts[2])
                dist = int(parts[3])
                bidir = int(parts[4]) if len(parts) > 4 else 1
                if u not in graph: graph[u] = []
                if v not in graph: graph[v] = []
                graph[u].append((v, dist))
                if bidir:
                    graph[v].append((u, dist))
    return landmarks, graph

def dijkstra(graph, start, end):
    distances = {node: float('inf') for node in graph}
    parent = {node: None for node in graph}
    distances[start] = 0
    pq = [(0, start)]
    
    while pq:
        curr_dist, u = heapq.heappop(pq)
        if curr_dist > distances[u]:
            continue
        if u == end:
            break
        for v, weight in graph.get(u, []):
            if distances[u] + weight < distances[v]:
                distances[v] = distances[u] + weight
                parent[v] = u
                heapq.heappush(pq, (distances[v], v))
                
    if distances[end] == float('inf'):
        return None, float('inf')
        
    path = []
    curr = end
    while curr is not None:
        path.append(curr)
        curr = parent[curr]
    path.reverse()
    return path, distances[end]

def main():
    print("=" * 65)
    print("  PYTHON GROUND TRUTH VERIFICATION FOR CITY MAP & DIJKSTRA  ")
    print("=" * 65)
    landmarks, graph = load_map('city_map.txt')
    print(f"Loaded {len(landmarks)} landmarks and {sum(len(v) for v in graph.values()) // 2} bidirectional roads.\n")

    scenarios = [
        ("Scenario 1: Base A (0) -> GEHU Campus (3)", 0, 3),
        ("Scenario 2: GEHU Campus (3) -> Civil Hospital (5)", 3, 5),
        ("Scenario 3: Mallital (2) -> Trauma Center (8)", 2, 8),
    ]

    for title, s, e in scenarios:
        path, dist = dijkstra(graph, s, e)
        print(f"[*] {title}")
        print(f"    Expected Distance: {dist} km")
        route_str = " -> ".join(f"[{node}] {landmarks[node]['name']}" for node in path)
        print(f"    Expected Path    : {route_str}\n")

if __name__ == "__main__":
    main()
