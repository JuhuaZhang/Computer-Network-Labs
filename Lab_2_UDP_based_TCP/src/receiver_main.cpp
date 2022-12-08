/*
 * File:   receiver_main.cpp
 * Author: Jiahao Zhang
 *
 * Created on 10/11/2022
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <pthread.h>
// my lib
#include <iostream>
#include <vector>
using namespace ::std;

#define DEBUG 1
#define MSS 1           // 1024
#define WINDOW_SIZE 100 // 1024

struct sockaddr_in si_me, si_other;
int s;
socklen_t slen;

// Send Packets as Segments
typedef struct TCP_Segment
{
    int seq_num; // if == -1, then finish
    int length;
    char data[MSS];
} TCP_Segment;

void diep(char *s)
{
    perror(s);
    exit(1);
}

void reliablyReceive(unsigned short int myUDPport, char *destinationFile)
{
    slen = sizeof(si_other);

    // bind socket with UDP and IP Address
    if ((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
        diep("socket");

    memset((char *)&si_me, 0, sizeof(si_me));
    si_me.sin_family = AF_INET;
    si_me.sin_port = htons(myUDPport);
    si_me.sin_addr.s_addr = htonl(INADDR_ANY);
    printf("Now binding\n");
    if (bind(s, (struct sockaddr *)&si_me, sizeof(si_me)) == -1)
        diep("bind");

    /* Now receive data and send acknowledgements */
    // Init rcv_buffer
    int numbytes;
    int last_ack = -1;
    TCP_Segment buf[WINDOW_SIZE];
    for (int i = 0; i < WINDOW_SIZE; i++)
    {
        buf[i].seq_num = -2; // means empty
    }
    FILE *pf = fopen(destinationFile, "w");
    fclose(pf);

    // rcv files
    while (1)
    {

#if DEBUG
        cout << endl
             << "-----------------------------------------------------------" << endl
             << endl;
#endif

        TCP_Segment seg;

        numbytes = recvfrom(s, &seg, sizeof(seg), 0, (struct sockaddr *)&si_other, &slen);
#if DEBUG
        cout << "recving: " << seg.seq_num << " seg" << endl;
        // cout << "content: " << seg.data << endl;
#endif
        if (numbytes == -1)
        {
            diep("Recv Error");
        }
        if (seg.seq_num == last_ack + 1) // write buffer into file
        {
#if DEBUG
            cout << "case 1" << endl;
#endif
            buf[0] = seg;
            int i, j, k;
            i = j = k = 0;
            FILE *pf = fopen(destinationFile, "a");
            while (buf[i].seq_num != -2)
            {
                fwrite(buf[i].data, buf[i].length, 1, pf);
                // #if DEBUG
                //                 cout << "write: " << buf[i].data << endl;
                //                 cout << "length: " << buf[i].length << endl;
                // #endif
                buf[i].length = -2;
                i++;
            }
            fclose(pf);

            last_ack = last_ack + i;
            // move buf element
            for (j = 0; j < WINDOW_SIZE - i; j++)
            {
                buf[j] = buf[j + i];
            }
            for (k = 0; k < i - 1; k++)
            {
                buf[j].seq_num = -2;
                j++;
            }
            sendto(s, &last_ack, sizeof(last_ack), 0, (struct sockaddr *)&si_other, slen);
        }
        else if (seg.seq_num > last_ack + 1)
        {
#if DEBUG
            cout << "case 2" << endl;
#endif
            // write something to buffer
            if (seg.seq_num - last_ack < WINDOW_SIZE)
            {
                buf[seg.seq_num - last_ack - 1] = seg;
#if DEBUG
                cout << "write pkg to " << seg.seq_num - last_ack - 1 << " buffer" << endl;
#endif
            }
            else
            {
#if DEBUG
                cout << "Too much in the buf, abort the pkg." << endl;
#endif
            }

            sendto(s, &last_ack, sizeof(last_ack), 0, (struct sockaddr *)&si_other, slen);
        }
        else if (seg.seq_num == -3) // connection finished
        {
#if DEBUG
            cout << "case 3" << endl;
#endif
            break;
        }
        else
        {
#if DEBUG
            cout << "case 4" << endl;
#endif
            // do nothing
            sendto(s, &last_ack, sizeof(last_ack), 0, (struct sockaddr *)&si_other, slen);
        }

#if DEBUG
        cout << "ack: " << last_ack << endl;
#endif
    }

#if DEBUG
    cout << "Recving FIN ..." << endl;
#endif
    // close connection, 3 way handshake
    int count = 0;
    while (1)
    {
        count++;
        last_ack = -4;
        struct timeval time_out;
        time_out.tv_sec = 0;
        time_out.tv_usec = 20000; // micro second

        setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &time_out, sizeof(time_out));
        sendto(s, &last_ack, sizeof(last_ack), 0, (struct sockaddr *)&si_other, slen);
#if DEBUG
        cout << "send 1 st FIN ack" << endl;
#endif

        setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &time_out, sizeof(time_out));
        int numbytes = recvfrom(s, &last_ack, sizeof(last_ack), 0, (struct sockaddr *)&si_other, &slen);
        if (count == 5)
        {
            break;
        }
        if (numbytes == -1)
        {
// do nothing
#if DEBUG
            cout << "Re-send 1 st FIN ack" << endl;
#endif
        }
        else if (last_ack == -3)
        {
            count = 0;
        }
        else if (last_ack == -4)
        {
            break;
        }
    }

    // stop
    // wirte complete
    // fclose(pf);

    close(s);
    cout << destinationFile << " received." << endl;
    return;
}

/*
 *
 */
int main(int argc, char **argv)
{

    unsigned short int udpPort;

    if (argc != 3)
    {
        fprintf(stderr, "usage: %s UDP_port filename_to_write\n\n", argv[0]);
        exit(1);
    }

    udpPort = (unsigned short int)atoi(argv[1]);

    reliablyReceive(udpPort, argv[2]);
}
