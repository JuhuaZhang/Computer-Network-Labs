/*
   link State: Djikstra’s algorithm
   Jiahao Zhang @ uiuc
   */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <sstream>

using namespace std;

typedef struct msg
{
    int src;
    int des;
    string content;
} msg;

int min_distance(vector<int> distance, vector<bool> Tset, int max_num)
{
    // to be written
    int min = 999;
    int ind;

    for (int k = 0; k < max_num; k++)
    {
        if (Tset[k] == false && distance[k] <= min)
        {
            min = distance[k];
            ind = k;
        }
    }

    return ind;
}

void Dijkstra(vector<vector<int>> graph, int src, int max_num, vector<vector<int>> &result)
{
    vector<int> distance(max_num, 999);
    vector<bool> Tset(max_num, false);

    distance[src - 1] = 0;

    for (int i = 0; i < max_num; i++)
    {
        int m = min_distance(distance, Tset, max_num);
        Tset[m] = true;

        for (int k = 0; k < max_num; k++)
        {
            if (!Tset[k] && graph[m][k] && distance[m] != 999 && distance[m] + graph[m][k] <= distance[k])
            {
                // if (distance[m] + graph[m][k] < distance[k] || (distance[m] + graph[m][k] == distance[k] && result[k][0] > result[m][0]))
                if (distance[m] + graph[m][k] < distance[k])
                {
                    distance[k] = distance[m] + graph[m][k];

                    // update path
                    result[k].clear();
                    for (int j = 0; j < result[m].size(); j++)
                    {
                        result[k].push_back(result[m][j]);
                    }
                    result[k].push_back(k);
                }
                else if (distance[m] + graph[m][k] == distance[k])
                {
                    // tie breaking 3
                    int a = result[k].size();
                    int b = result[m].size();
#if DEBUG
                    cout << "src: " << src << endl;
                    cout << "k: " << k + 1 << endl;
                    cout << "m: " << m + 1 << endl;
                    cout << "last hop of orgin path: " << result[k][a - 2] + 1 << endl;
                    cout << "last hop of new path: " << result[m][b - 1] + 1 << endl;
#endif
                    if (result[k][a - 2] > result[m][b - 1])
                    {
                        // update path
                        result[k].clear();
                        for (int j = 0; j < result[m].size(); j++)
                        {
                            result[k].push_back(result[m][j]);
                        }
                        result[k].push_back(k);
                    }
                }
            }
        }
    }

    result[src - 1].push_back(src - 1);

    for (int i = 0; i < max_num; i++)
    {
        result[i].push_back(distance[i]);
    }

#if DEBUG

    cout << endl
         << "-----result-----" << endl
         << endl;
    for (int i = 0; i < max_num; i++)
    {
        cout << i + 1 << " ";

        for (int j = 0; j < result[i].size() - 1; j++)
        {
            cout << result[i][j] + 1 << " ";
        }

        cout << result[i].back() << endl;
    }

#endif
}

int main(int argc, char **argv)
{

    if (argc != 4)
    {
        printf("Usage: ./distvec topofile messagefile changesfile\n");
        return -1;
    }

    // open files
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
                max_num = max(max_num, stoi(num));
                // max_num = max(max_num, stoi(num));
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
        int j = 0;

        string num;
        while (aline[j] != ' ' && j != aline.size())
        {
            num += aline[j];
            j++;
        }
        j++;
        temp.src = (stoi(num));

        num = "";
        while (aline[j] != ' ' && j != aline.size())
        {
            num += aline[j];
            j++;
        }
        j++;
        temp.des = (stoi(num));

        while (j != aline.size())
        {
            temp.content += aline[j];
            j++;
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
        // calucalte
        vector<vector<vector<int>>> results(max_num, vector<vector<int>>(max_num));

        for (int i = 0; i < max_num; i++)
        {
            Dijkstra(graph, i + 1, max_num, results[i]);

            for (int j = 0; j < max_num; j++)
            {
                if (results[i][j].back() < 999)
                {
                    output << j + 1 << " ";
                    output << results[i][j][0] + 1 << " ";
                    output << results[i][j].back();
                    output << endl;
                }
            }
            output << endl;
        }

        for (int i = 0; i < message.size(); i++)
        {
            if (results[message[i].src - 1][message[i].des - 1].back() < 999)
            {
                string hops;
                hops += to_string(message[i].src);
                hops += " ";

                for (int j = 0; j < results[message[i].src - 1][message[i].des - 1].size() - 2; j++)
                {
                    hops += to_string(results[message[i].src - 1][message[i].des - 1][j] + 1);
                    hops += " ";
                }

                output << "from " << message[i].src << " to " << message[i].des
                       << " cost " << results[message[i].src - 1][message[i].des - 1].back()
                       << " hops " << hops
                       << "message " << message[i].content
                       << endl;
            }
            else
            {
                // infinite
                output << "from " << message[i].src << " to " << message[i].des
                       << " cost infinite hops unreachable "
                       << "message " << message[i].content
                       << endl;
            }
        }
        output << endl;
        // apply changes
        // let k = change_num
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
