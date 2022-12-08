/*
   distance vector: Bellman Ford algorithm
   Jiahao Zhang @ uiuc
   */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>

using namespace std;

#define DEBUG 1

typedef struct routing_table
{
    int des;
    int distance;
    vector<int> hop;
} routing_table;

typedef struct msg
{
    int src;
    int des;
    string content;
} msg;

// void dev(vector<vector<int> > graph, int max_num)

vector<vector<routing_table>> dev(vector<vector<int>> graph, int max_num)
{
    // inital routing table k * k * 3
    vector<vector<routing_table>> RTable(max_num, vector<routing_table>(max_num));

    for (int i = 0; i < max_num; i++)
    {
        for (int j = 0; j < max_num; j++)
        {
            RTable[i][j].des = j;
            RTable[i][j].distance = graph[i][j] == 0 ? 999 : graph[i][j];
            RTable[i][j].hop.push_back(j);
        }
        RTable[i][i].distance = 0;
    }

    // exchange routing table
    for (int b = 0; b < max_num; b++)
    {
        // buf to store the original RTable
        vector<vector<routing_table>> RTable_buf = RTable;
        for (int i = 0; i < max_num; i++)
        {
            for (int j = 0; j < max_num; j++)
            {
                if (graph[i][j] != 0)
                {
                    // node j is connected to node i
                    for (int k = 0; k < max_num; k++)
                    {
                        if (RTable[i][k].distance > RTable_buf[i][j].distance + RTable_buf[j][k].distance)
                        {
                            RTable[i][k].distance = RTable_buf[i][j].distance + RTable_buf[j][k].distance;
                            RTable[i][k].hop.clear();
                            // Path i -> k = i -> j -> k
                            // Path i -> j
                            for (int a = 0; a < RTable_buf[i][j].hop.size(); a++)
                            {
                                RTable[i][k].hop.push_back(RTable_buf[i][j].hop[a]);
                            }
                            // Path j -> k
                            for (int a = 0; a < RTable_buf[j][k].hop.size(); a++)
                            {
                                RTable[i][k].hop.push_back(RTable[j][k].hop[a]);
                            }
                            // RTable[i][k].hop.push_back(k);
                        }

                        // tie breaking 1, choose the one whose next-hop node ID is lower
                        else if (RTable[i][k].distance == RTable_buf[i][j].distance + RTable_buf[j][k].distance)
                        {
                            // i -> a -> ... -> k in RTable[i][k]
                            // i -> b -> ... -> j -> c -> .. -> k in RTable[i][j] -> RTable[j][k]

                            // compare RTable[i][k] and RTable[i][j]
                            if (RTable[i][k].hop[0] > RTable[i][j].hop[0])
                            {
                                RTable[i][k].hop.clear();
                                // Path i -> k = i -> j -> k
                                // Path i -> j
                                for (int a = 0; a < RTable_buf[i][j].hop.size(); a++)
                                {
                                    RTable[i][k].hop.push_back(RTable_buf[i][j].hop[a]);
                                }
                                // Path j -> k
                                for (int a = 0; a < RTable_buf[j][k].hop.size(); a++)
                                {
                                    RTable[i][k].hop.push_back(RTable[j][k].hop[a]);
                                }
                            }
                        }
                    }
                    // exchange file
                }
            }
        }
        // change for 1 time, need to change max_num time
    }

#if DEBUG
    cout << endl;
    for (int i = 0; i < max_num; i++)
    {
        cout << i + 1 << endl;
        for (int j = 0; j < max_num; j++)
        {
            cout << RTable[i][j].des + 1;
            for (int k = 0; k < RTable[i][j].hop.size(); k++)
            {
                cout << " " << RTable[i][j].hop[k] + 1;
            }
            cout << " " << RTable[i][j].distance;
            cout << endl;
        }
        cout << endl;
    }
#endif

    return RTable;
}

int main(int argc, char **argv)
{
    // printf("Number of arguments: %d", argc);
    if (argc != 4)
    {
        printf("Usage: ./linkstate topofile messagefile changesfile\n");
        return -1;
    }

    ifstream topofile;
    topofile.open(argv[1]);

    ifstream messagefile;
    messagefile.open(argv[2]);

    ifstream changesfile;
    changesfile.open(argv[3]);

    ofstream output;
    output.open("output.txt", ios::out | ios::trunc);

    // convert input to graph
    string aline;

    vector<int> temp;
    int max_num = 0;

    // wrong! maybe multi-digit
    while (getline(topofile, aline))
    {
        //  for(int i = 0; i < 5; i=i+2){
        //      temp.push_back(int(aline[i]-'0'));
        //  }

        int j = 0;
        for (int i = 0; i < 3; i++)
        {
            string num;
            while (aline[j] != ' ' && j != aline.size())
            {
                num += aline[j];
                j++;
            }
            j++;
            // maybe need to use stoi(num) == -999 ? 0 : stoi(num)
            temp.push_back(stoi(num));

            if (i == 2)
            {
            }
            else
            {
                max_num = max(max_num, int(aline[0] - '0'));
                max_num = max(max_num, int(aline[2] - '0'));
            }
        }

        cout << endl;
    }
    topofile.close();

    vector<vector<int>> graph(max_num, vector<int>(max_num));
    for (int i = 0; i < temp.size(); i = i + 3)
    {
        graph[temp[i] - 1][temp[i + 1] - 1] = temp[i + 2];
        graph[temp[i + 1] - 1][temp[i] - 1] = temp[i + 2];
    }

#if DEBUG

    cout << endl
         << "-----graph-----" << endl
         << endl;
    for (int i = 0; i < max_num; i++)
    {
        for (int j = 0; j < max_num; j++)
        {
            cout << graph[i][j] << " ";
        }
        cout << endl;
    }

#endif

    // get msg file
    vector<msg> message;

    while (getline(messagefile, aline))
    {
        msg temp;
        temp.src = int(aline[0] - '0');
        temp.des = int(aline[2] - '0');

        int len = aline.size();

        for (int i = 4; i < len; i++)
        {
            temp.content += aline[i];
        }

        message.push_back(temp);
    }
    messagefile.close();

    vector<int> change;
    int change_num = 0;

    while (getline(changesfile, aline))
    {
        int j = 0;
        for (int i = 0; i < 3; i++)
        {
            string num;
            while (aline[j] != ' ' && j != aline.size())
            {
                num += aline[j];
                j++;
            }
            j++;
            change.push_back(stoi(num));
        }
        change_num++;
    }
    changesfile.close();

    for (int k = 0; k <= change_num; k++)
    {
        vector<vector<routing_table>> RTable = dev(graph, max_num);

        for (int i = 0; i < max_num; i++)
        {

            for (int j = 0; j < max_num; j++)
            {
                if (RTable[i][j].distance < 999)
                {
                    output << j + 1 << " ";
                    output << RTable[i][j].hop[0] + 1 << " ";
                    output << RTable[i][j].distance;
                    output << endl;
                }
            }
            output << endl;
        }

        for (int i = 0; i < message.size(); i++)
        {
            if (RTable[message[i].src - 1][message[i].des - 1].distance < 999)
            {
                string hops;
                hops += char(message[i].src + '0');
                hops += " ";

                for (int j = 0; j < RTable[message[i].src - 1][message[i].des - 1].hop.size() - 1; j++)
                {
                    hops += char(RTable[message[i].src - 1][message[i].des - 1].hop[j] + 1 + '0');
                    hops += " ";
                }

                output << "from " << message[i].src << " to " << message[i].des
                       << " cost " << RTable[message[i].src - 1][message[i].des - 1].distance
                       << " hops " << hops
                       << "message " << message[i].content
                       << endl;
            }
            else
            {
                // inifinte
                output << "from " << message[i].src << " to " << message[i].des
                       << " cost infinite hops unreachable "
                       << "message " << message[i].content
                       << endl;
            }
        }

        output << endl;

        if (k < change_num)
        {
            graph[change[3 * k] - 1][change[3 * k + 1] - 1] = change[3 * k + 2] == -999 ? 0 : change[3 * k + 2];
            graph[change[3 * k + 1] - 1][change[3 * k] - 1] = change[3 * k + 2] == -999 ? 0 : change[3 * k + 2];
#if DEBUG
            cout << endl
                 << "-----graph-----" << endl
                 << endl;
            for (int i = 0; i < max_num; i++)
            {
                for (int j = 0; j < max_num; j++)
                {
                    cout << graph[i][j] << " ";
                }
                cout << endl;
            }
#endif
        }
    }
    output.close();
    return 0;
}
