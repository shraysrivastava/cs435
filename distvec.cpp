#include <iostream>
#include <fstream>
#include <sstream>
#include <map>
#include <vector>
#include <algorithm>
#include <set>

using namespace std;

static const int INF = 999999;

struct RouteInfo
{
    int nextHop;
    int cost;
};

class Router
{
public:
    int id;
    map<int, RouteInfo> routingTable; // destinations mapped to RouteInfo
    map<int, int> neighbors; // links neighbor id to cost of link to that neighbor
    Router() : id(-1) {}
    Router(int nodeId) : id(nodeId)
    {
        routingTable[nodeId] = {nodeId, 0};
    }
    void removeRoutesThrough(int other)
    {
        neighbors.erase(other);
        for (auto it = routingTable.begin(); it != routingTable.end();)
        {
            if (it->first == other || it->second.nextHop == other)
            {
                it = routingTable.erase(it);
            }
            else
            {
                ++it;
            }
        }
    }
    void printTable(FILE *out) const
    {
        vector<int> dests;
        for (auto &kv : routingTable)
        {
            dests.push_back(kv.first);
        }
        sort(dests.begin(), dests.end());
        for (int d : dests)
        {
            fprintf(out, "%d %d %d\n", d, routingTable.at(d).nextHop, routingTable.at(d).cost);
        }
    }
};

map<int, Router> network;

void distanceVectorRouting()
{
    if (network.empty())
        return;
    vector<int> nodeIds;
    for (auto &kv : network)
        nodeIds.push_back(kv.first);
    sort(nodeIds.begin(), nodeIds.end());
    map<int, map<int, RouteInfo>> dist;
    for (int i : nodeIds)
    {
        dist[i].clear();
        dist[i][i] = {i, 0};
    }
    for (int i : nodeIds)
    {
        for (auto &nbr : network[i].neighbors)
        {
            int n = nbr.first;
            int cost = nbr.second;
            if (!dist[i].count(n) || cost < dist[i][n].cost)
            {
                dist[i][n] = {n, cost};
            }
            else if (dist[i][n].cost == cost && n < dist[i][n].nextHop)
            {
                dist[i][n] = {n, cost};
            }
        }
    }
    int N = (int)nodeIds.size();
    for (int r = 0; r < N - 1; r++)
    {
        bool changed = false;
        auto newDist = dist;
        for (int i : nodeIds)
        {
            for (auto &nbr : network[i].neighbors)
            {
                int n = nbr.first;
                int cost_i_n = nbr.second;
                for (auto &dEntry : dist[n])
                {
                    int d = dEntry.first;
                    int c_nd = dEntry.second.cost;
                    if (c_nd >= INF)
                        continue;
                    long long alt = (long long)cost_i_n + c_nd;
                    if (alt > INF)
                        alt = INF;
                    if (!newDist[i].count(d) || alt < newDist[i][d].cost)
                    {
                        newDist[i][d] = {n, (int)alt};
                        changed = true;
                    }
                    else if ((int)alt == newDist[i][d].cost && (int)alt < INF)
                    {
                        if (n < newDist[i][d].nextHop)
                        {
                            newDist[i][d] = {n, (int)alt};
                            changed = true;
                        }
                    }
                }
            }
        }
        dist = newDist;
        if (!changed)
            break;
    }
    for (int i : nodeIds)
    {
        network[i].routingTable.clear();
        for (auto &x : dist[i])
        {
            if (x.second.cost < INF)
            {
                network[i].routingTable[x.first] = x.second;
            }
        }
    }
}

void processMessages(const string &msgFile, FILE *out)
{
    ifstream in(msgFile);
    if (!in)
        return;
    string line;
    while (true)
    {
        if (!getline(in, line))
            break;
        if (line.empty())
            continue;
        istringstream ss(line);
        int src, dst;
        ss >> src >> dst;
        string msg;
        getline(ss, msg);
        if (!network.count(src) || !network.count(dst) ||
            !network[src].routingTable.count(dst))
        {
            fprintf(out, "from %d to %d cost infinite hops unreachable message%s\n",
                    src, dst, msg.c_str());
            continue;
        }
        int totalCost = 0;
        int curr = src;
        vector<int> path;
        path.push_back(curr);
        set<int> visited;
        bool looped = false;
        while (curr != dst)
        {
            visited.insert(curr);
            int nxt = network[curr].routingTable[dst].nextHop;
            if (!network[curr].neighbors.count(nxt))
            {
                totalCost = INF;
                break;
            }
            totalCost += network[curr].neighbors[nxt];
            curr = nxt;
            if (visited.count(curr))
            {
                looped = true;
                break;
            }
            path.push_back(curr);
        }
        if (looped || totalCost >= INF)
        {
            fprintf(out, "from %d to %d cost infinite hops unreachable message%s\n",
                    src, dst, msg.c_str());
        }
        else
        {
            fprintf(out, "from %d to %d cost %d hops", src, dst, totalCost);
            for (int i = 0; i + 1 < (int)path.size(); i++)
            {
                fprintf(out, " %d", path[i]);
            }
            fprintf(out, " message%s\n", msg.c_str());
        }
    }
}

int main(int argc, char *argv[])
{
    if (argc != 4)
        return 1;
    string topoFile = argv[1];
    string msgFile = argv[2];
    string changesFile = argv[3];
    {
        ifstream in(topoFile);
        int a, b, c;
        while (in >> a >> b >> c)
        {
            if (!network.count(a))
                network[a] = Router(a);
            if (!network.count(b))
                network[b] = Router(b);
            network[a].neighbors[b] = c;
            network[b].neighbors[a] = c;
        }
    }
    FILE *out = fopen("output.txt", "w");
    if (!out)
        return 1;
    distanceVectorRouting();
    {
        vector<int> all;
        for (auto &kv : network)
            all.push_back(kv.first);
        sort(all.begin(), all.end());
        for (int id : all)
        {
            network[id].printTable(out);
        }
    }
    processMessages(msgFile, out);
    {
        ifstream changes(changesFile);
        string line;
        while (true)
        {
            if (!getline(changes, line))
                break;
            if (line.empty())
                continue;
            // fprintf(out, "---test line\n");
            istringstream ss(line);
            int a, b, c;
            ss >> a >> b >> c;
            if (c == -999)
            {
                if (network.count(a))
                    network[a].removeRoutesThrough(b);
                if (network.count(b))
                    network[b].removeRoutesThrough(a);
            }
            else
            {
                if (!network.count(a))
                    network[a] = Router(a);
                if (!network.count(b))
                    network[b] = Router(b);
                network[a].neighbors[b] = c;
                network[b].neighbors[a] = c;
            }
            distanceVectorRouting();
            vector<int> all;
            for (auto &kv : network)
                all.push_back(kv.first);
            sort(all.begin(), all.end());
            for (int id : all)
            {
                network[id].printTable(out);
            }
            processMessages(msgFile, out);
        }
    }
    fclose(out);
    return 0;
}
