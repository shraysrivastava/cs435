#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <map>
#include <algorithm>

using namespace std;

struct RouteInfo
{
    int nextHop;
    int cost;
};

class Router
{
public:
    int id;
    map<int, RouteInfo> routingTable; // destination -> {nextHop, cost}
    map<int, int> neighbors;          // neighbor -> cost

    Router() : id(-1) {}
    Router(int nodeId) : id(nodeId)
    {
        routingTable[id] = {id, 0};
    }

    void updateFromNeighbor(int neighborId, const map<int, RouteInfo> &neighborTable, int costToNeighbor)
    {
        for (const auto &[dest, route] : neighborTable)
        {
            if (dest == id)
                continue;
                
            int newCost = route.cost + costToNeighbor;

            auto it = routingTable.find(dest);
            if (it == routingTable.end() || newCost < it->second.cost ||
                (newCost == it->second.cost && neighborId < it->second.nextHop))
            {
                routingTable[dest] = {neighborId, newCost};
            }
        }
    }

    void removeRoutesThrough(int removedNode)
    {
        for (auto it = routingTable.begin(); it != routingTable.end();)
        {
            if (it->second.nextHop == removedNode || it->first == removedNode)
            {
                it = routingTable.erase(it);
            }
            else
            {
                ++it;
            }
        }
        neighbors.erase(removedNode);
    }

    void printTable(FILE *out) const
    {
        vector<int> destinations;
        for (const auto &entry : routingTable)
            destinations.push_back(entry.first);
        sort(destinations.begin(), destinations.end());

        for (int dest : destinations)
        {
            const auto &r = routingTable.at(dest);
            fprintf(out, "%d %d %d\n", dest, r.nextHop, r.cost);
        }
    }
};

map<int, Router> network;

void distanceVectorRouting()
{
    bool updated;
    do
    {
        updated = false;
        for (auto &[nodeId, node] : network)
        {
            for (const auto &[neighborId, cost] : node.neighbors)
            {
                size_t beforeSize = node.routingTable.size();
                node.updateFromNeighbor(neighborId, network[neighborId].routingTable, cost);
                if (node.routingTable.size() != beforeSize)
                {
                    updated = true;
                }
            }
        }
    } while (updated);
}

void processMessages(const string &file, FILE *out)
{
    ifstream in(file);
    string line;

    while (getline(in, line))
    {
        istringstream ss(line);
        int src, dst;
        ss >> src >> dst;
        string msg;
        getline(ss, msg);

        if (!network.count(src) || !network.count(dst) || !network[src].routingTable.count(dst))
        {
            fprintf(out, "from %d to %d cost infinite hops unreachable message%s\n", src, dst, msg.c_str());
            continue;
        }

        vector<int> path = {src};
        int curr = src, totalCost = 0;
        while (curr != dst)
        {
            int next = network[curr].routingTable[dst].nextHop;
            totalCost += network[curr].neighbors[next];
            path.push_back(next);
            curr = next;
        }

        fprintf(out, "from %d to %d cost %d hops", src, dst, totalCost);
        for (size_t i = 0; i < path.size() - 1; ++i)
        {
            fprintf(out, " %d", path[i]);
        }
        fprintf(out, " message%s\n", msg.c_str());
    }
}

int main(int argc, char *argv[])
{
    if (argc != 4)
    {
        cout << "Usage: ./distvec topofile messagefile changesfile\n";
        return 1;
    }

    string topoFile = argv[1];
    string msgFile = argv[2];
    string changesFile = argv[3];

    FILE *output = fopen("output.txt", "w");

    ifstream in(topoFile);
    int a, b, cost;
    while (in >> a >> b >> cost)
    {
        if (!network.count(a))
            network[a] = Router(a);
        if (!network.count(b))
            network[b] = Router(b);
        network[a].neighbors[b] = cost;
        network[b].neighbors[a] = cost;
    }
    distanceVectorRouting();

    for (const auto &[id, router] : network)
        router.printTable(output);

    processMessages(msgFile, output);

    ifstream changes(changesFile);
    string line;
    while (getline(changes, line))
    {
        fprintf(output, "-----\n");
        istringstream ss(line);
        int a, b, cost;
        ss >> a >> b >> cost;

        if (cost == -999)
        {
            if (network.count(a))
                network[a].removeRoutesThrough(b);
            if (network.count(b))
                network[b].removeRoutesThrough(a);
        }
        else
        {
            network[a].neighbors[b] = cost;
            network[b].neighbors[a] = cost;
        }
        distanceVectorRouting();
        for (const auto &[id, router] : network)
            router.printTable(output);
        processMessages(msgFile, output);
    }

    fclose(output);
    return 0;
}
