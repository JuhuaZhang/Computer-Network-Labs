#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cmath>
#include <sstream>
#include <iomanip>

using namespace std;

typedef struct node
{
    int id;
    int R;
    int backoff;
    int trans;     // the time tick that the pkt has been trans, <= pkt size L
    int collision; // collision times, <= M
} node;

int main(int argc, char **argv)
{
    // printf("Number of arguments: %d", argc);
    if (argc != 2)
    {
        printf("Usage: ./csma input.txt\n");
        return -1;
    }

    // read input
    ifstream input;
    input.open(argv[1]);
    vector<int> _input;
    vector<int> R;

    string aline;
    while (getline(input, aline))
    {
        if (aline[0] == 'R')
        {
            stringstream ss(aline.substr(2));
            int n;
            while (ss >> n)
            {
                R.push_back(n);
            }
        }
        string num;
        for (int i = 2; i != aline.size() && aline[i] != ' '; i++)
        {
            num += aline[i];
        }
        _input.push_back(stoi(num));
    }
    input.close();

    // system init
    int channel = -1;        // -1 means idle, if occupied by node -> value = node_id
    int effect_time = 0;     // successful trans tick
    vector<int> ReadyToSend; // store the node id that has its backoff == 0

    vector<node> nodes(_input[0]);
    for (int i = 0; i < nodes.size(); i++)
    {
        nodes[i].id = i;
        nodes[i].R = _input[3];
        nodes[i].backoff = i % _input[3];
        nodes[i].trans = 0;
        nodes[i].collision = 0;
    }

    // ofstream debugoutput;
    // debugoutput.open("deoutput.txt", ios::out | ios::trunc);

    // T = _input[4]
    for (int t = 0; t < _input[4]; t++)
    {

        // for (int i = 0; i < nodes.size(); i++)
        // {
        //     debugoutput << t << " " << i << " " << nodes[i].backoff << " " << nodes[i].R << endl;
        // }

        if (channel == -1)
        {
            // channel is idle
            for (int i = 0; i < nodes.size(); i++)
            {
                // examine num of nodes ready to send
                if (nodes[i].backoff == 0)
                {
                    ReadyToSend.push_back(i);
                }
            }
            if (ReadyToSend.size() > 1)
            {
                t++;
                // collision happen!
                for (int i = 0; i < ReadyToSend.size(); i++)
                {
                    nodes[ReadyToSend[i]].collision++;
                    if (nodes[ReadyToSend[i]].collision == _input[2])
                    {
                        // abort transmission
                        nodes[ReadyToSend[i]].collision = 0;
                        // nodes[ReadyToSend[i]].backoff = (nodes[ReadyToSend[i]].id + t) % nodes[ReadyToSend[i]].R;
                        nodes[ReadyToSend[i]].R = _input[3];
                        nodes[ReadyToSend[i]].backoff = (nodes[ReadyToSend[i]].id + t) % nodes[ReadyToSend[i]].R;
                    }
                    else
                    {
                        nodes[ReadyToSend[i]].R = R[nodes[ReadyToSend[i]].collision % _input[2]];
                        nodes[ReadyToSend[i]].backoff = (nodes[ReadyToSend[i]].id + t) % nodes[ReadyToSend[i]].R;
                    }
                }
                // clear for next term
                ReadyToSend.clear();
                t--;
            }
            else if (ReadyToSend.size() == 1)
            {
                // send pkt
                channel = ReadyToSend[0];
                nodes[channel].trans++;
                // clear for next term
                ReadyToSend.clear();

                // success time ++
                effect_time++;
            }
            else
            {
                // channel is idle, count down for each node
                for (int i = 0; i < nodes.size(); i++)
                {
                    nodes[i].backoff--;
                }
            }
        }
        else
        {
            // nodes[channel] is transmitting
            nodes[channel].trans++;
            // pkt size L = _input[1], successful transmission
            if (nodes[channel].trans == _input[1])
            {
                // reset
                nodes[channel].trans = 0;
                nodes[channel].collision = 0;
                nodes[channel].R = _input[3];
                nodes[channel].backoff = (nodes[channel].id + t + 1) % nodes[channel].R;

                // transmision is over
                channel = -1;
            }
            // success time ++
            effect_time++;
        }
    }
    // debugoutput.close();

    // int res = (100 * (1.0 * effect_time) / (1.0 * _input[4])); // _input[4] starts with 0
    double res_double = (1.0 * effect_time) / (1.0 * _input[4]);

    ofstream output;
    output.open("output.txt", ios::out | ios::trunc);

    output << fixed;
    output << setprecision(2);
    output << res_double;
    output << char('\n');

    // if (res == 100)
    // {
    //     output << "1.00";
    // }
    // else
    // {
    //     output << char('0');
    //     output << '.';
    //     output << char('0' + res / 10);
    //     output << char('0' + res % 10);
    //     output << char('\n');
    // }

    output.close();

    return 0;
}
