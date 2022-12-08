/*
 * File:   sender_main.cpp
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
#include <sys/stat.h>
#include <signal.h>
#include <string.h>
#include <sys/time.h>
// my lib
#include <iostream>
#include <vector>
#include <cmath>

using namespace ::std;

#define DEBUG 1
#define MSS 1 // 1024
struct sockaddr_in si_other;
int s;
socklen_t slen;

// Send Packets as Segments
typedef struct TCP_Segment
{
    int seq_num; // start from 0; if == -3, then finish
    int length;
    char data[MSS];
} TCP_Segment;

void diep(char *s)
{
    perror(s);
    exit(1);
}

// send package, range from last_sent to CW_header + CW
int send_packet(int s, int last_sent, int CW_header, double CW, FILE *fp, unsigned long long int bytesToTransfer, int packet_num, vector<timeval> &time_stamp)
{
    // read the file and write it to the buf
    int seq = CW_header;
    TCP_Segment buf[int(CW)];

    // mv fp at header
    int offset = 0;
    offset = MSS * (CW_header) * sizeof(char);
    fseek(fp, offset, 0);

    for (int i = 0; i < int(CW); i++)
    {
        buf[i].seq_num = 0;
        buf[i].length = 0;
        memset(buf[i].data, 0, MSS);
    }

    while (!feof(fp) && seq < int(CW) + CW_header)
    {
        if (seq < packet_num - 1) // until the last seg
        {
            fread(buf[seq - CW_header].data, sizeof(char), MSS, fp);
            buf[seq - CW_header].seq_num = seq;
            buf[seq - CW_header].length = sizeof(buf[seq - CW_header].data);
        }
        else // last seg
        {
            fread(buf[seq - CW_header].data, sizeof(char), bytesToTransfer - (packet_num - 1) * MSS, fp);
            buf[seq - CW_header].seq_num = seq;
            buf[seq - CW_header].length = bytesToTransfer - (packet_num - 1) * MSS;
        }
        seq++;
    }

    // send the seg
    for (int i = last_sent - CW_header; i < int(CW); i++)
    {
        TCP_Segment *seg_ptr = &(buf[i]);
        sendto(s, (char *)seg_ptr, sizeof(buf[i]), 0, (struct sockaddr *)&si_other, slen);
        // Add time stamp
        struct timeval cur_time;
        gettimeofday(&cur_time, NULL);
        time_stamp.push_back(cur_time);

        last_sent++;

#if DEBUG
        cout << "send packet: " << buf[i].seq_num << endl;
#endif
    }
    return last_sent;
}

void reliablyTransfer(char *hostname, unsigned short int hostUDPport, char *filename, unsigned long long int bytesToTransfer)
{
    // Open the file
    FILE *fp;
    fp = fopen(filename, "rb");
    if (fp == NULL)
    {
        printf("Could not open file to send.");
        exit(1);
    }

    // init socket
    slen = sizeof(si_other);

    if ((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
        diep("socket");

    memset((char *)&si_other, 0, sizeof(si_other));
    si_other.sin_family = AF_INET;
    si_other.sin_port = htons(hostUDPport);
    if (inet_aton(hostname, &si_other.sin_addr) == 0)
    {
        fprintf(stderr, "inet_aton() failed\n");
        exit(1);
    }

    /* Send data and receive acknowledgements on s*/

    double CW = 1;
    int CW_header = 0;
    int SST = 5;
    int dup_ack_count = 0;
    int last_ack = -1;
    int cur_ack = -1;
    int last_sent = 0;
    string state = "slow start";

    struct timeval cur_time;
    // socket time out
    struct timeval time_out;
    time_out.tv_sec = 0;
    time_out.tv_usec = 20000; // micro seconds

    // time stamp
    vector<timeval> time_stamp;

    /* Determine how many bytes to transfer */
    int packet_num = ceil(double(bytesToTransfer) / double(MSS));

#if DEBUG
    cout << "state: " << state << endl;
    cout << "cur_ack: " << cur_ack << endl;
    cout << "last_ack: " << last_ack << endl;
    cout << "CW_header: " << CW_header << endl;
    cout << "CW: " << CW << endl;
#endif

    // send the first packet
    last_sent = send_packet(s, 0, CW_header, CW, fp, bytesToTransfer, packet_num, time_stamp);

    while (1)
    {

#if DEBUG
        cout << endl
             << "-----------------------------------------------------------" << endl
             << endl;
#endif

        last_ack = cur_ack;
        // set timeout
        setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &time_out, sizeof(time_out));

        int numbytes = recvfrom(s, &cur_ack, sizeof(int), 0, (struct sockaddr *)&si_other, &slen);

        gettimeofday(&cur_time, NULL);

        int delta_time = 1000000 * (cur_time.tv_sec - time_stamp[last_ack + 1].tv_sec) + (cur_time.tv_usec - time_stamp[last_ack + 1].tv_usec);

        if (numbytes == -1 || delta_time > 30000)
        {
#if DEBUG
            cout << endl;
            cout << "Time out!" << endl;
            cout << endl;
#endif
            // time out
            state = "slow start";
            SST = int(CW / 2);
            CW = 1;

            // Resend last_ack + 1
            TCP_Segment buf;
            fseek(fp, (last_ack + 1) * MSS * sizeof(char), 0);
            // size may not be MSS !!!
            fread(buf.data, sizeof(char), MSS, fp);
            buf.seq_num = CW_header;
            buf.length = MSS;
            TCP_Segment *seg_ptr = &(buf);
            sendto(s, (char *)seg_ptr, sizeof(buf), 0, (struct sockaddr *)&si_other, slen);
            // update time stamp
            gettimeofday(&cur_time, NULL);
            time_stamp[last_ack + 1] = cur_time;
#if DEBUG
            cout << "Re-Transmit: " << CW_header << " seg." << endl;
#endif
        }

        if (cur_ack + 1 == packet_num)
        {
            break;
        }
        if (cur_ack == last_ack)
        {
            dup_ack_count++;
            CW++;

#if DEBUG
            cout << "dup ack detect, "
                 << "dup ack count: " << dup_ack_count << endl;
#endif

            if (dup_ack_count == 3)
            {
                dup_ack_count = 0;
                state = "fast recovery";
                SST = CW / 2;
                CW = SST + 3;

                // re-send CW_header
                TCP_Segment buf;
                // size may not be MSS !!!
                fseek(fp, (last_ack + 1) * MSS * sizeof(char), 0);
                fread(buf.data, sizeof(char), MSS, fp);
                buf.seq_num = CW_header;
                buf.length = MSS;
                TCP_Segment *seg_ptr = &(buf);
                sendto(s, (char *)seg_ptr, sizeof(buf), 0, (struct sockaddr *)&si_other, slen);
                // update time stamp
                gettimeofday(&cur_time, NULL);
                time_stamp[last_ack + 1] = cur_time;
#if DEBUG
                cout << "Re-Transmit: " << CW_header << " seg." << endl;
#endif
            }
        }
        else // successfully recv
        {
            CW_header = cur_ack + 1;
            dup_ack_count = 0;
            if (state == "slow start")
            {
                if (CW > SST)
                {
                    state = "congestion avoidence";
                    CW += 1.0 / int(CW);
                }
                else
                {
                    CW++;
                }
            }
            else if (state == "congestion avoidence")
            {
                CW += 1.0 / int(CW);
            }
            else if (state == "fast recovery")
            {
                CW += 1;
            }
        };

#if DEBUG
        cout << "state: " << state << endl;
        cout << "cur_ack: " << cur_ack << endl;
        cout << "last_ack: " << last_ack << endl;
        cout << "CW_header: " << CW_header << endl;
        cout << "CW: " << CW << endl;
#endif
        // if reach to the end
        if (CW_header + int(CW) > packet_num)
        {
            // sending complete
            if (CW_header == packet_num)
            {
                break;
            }
            else if (CW)
                last_sent = send_packet(s, last_sent, CW_header, packet_num - CW_header, fp, bytesToTransfer, packet_num, time_stamp);
        }
        else
        {
            last_sent = send_packet(s, last_sent, CW_header, CW, fp, bytesToTransfer, packet_num, time_stamp);
        }
    }
#if DEBUG
    // cout << "time_stamp: " << time_stamp.size() << endl;
    // for (int i = 0; i < time_stamp.size(); i++) // size()容器中实际数据个数
    // {
    //     cout << i << "" << time_stamp[i].tv_sec << " " << time_stamp[i].tv_usec << endl;
    // }
#endif
    // close connection, using 3 way handshake
    TCP_Segment end;
    TCP_Segment *end_ptr = &end;
    end.seq_num = -3;

    while (1)
    {
        // send fin msg
        sendto(s, (char *)end_ptr, sizeof(end), 0, (struct sockaddr *)&si_other, slen);

        // recv FIN ack,
        setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &time_out, sizeof(time_out));
        int numbytes = recvfrom(s, &cur_ack, sizeof(int), 0, (struct sockaddr *)&si_other, &slen);
        if (numbytes == -1)
        {
            // time out, resend
        }
        else if (cur_ack == -4)
        {
            break;
        }
    }

    cur_ack = -4;
    sendto(s, &cur_ack, sizeof(cur_ack), 0, (struct sockaddr *)&si_other, slen);

    fclose(fp);
    printf("Closing the socket\n");
    close(s);
    return;
}

/*
 *
 */
int main(int argc, char **argv)
{

    unsigned short int udpPort;
    unsigned long long int numBytes;

    if (argc != 5)
    {
        fprintf(stderr, "usage: %s receiver_hostname receiver_port filename_to_xfer bytes_to_xfer\n\n", argv[0]);
        exit(1);
    }
    udpPort = (unsigned short int)atoi(argv[2]);
    numBytes = atoll(argv[4]);

    reliablyTransfer(argv[1], udpPort, argv[3], numBytes);

    return (EXIT_SUCCESS);
}
