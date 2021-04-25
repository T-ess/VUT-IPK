//
// Created by terez on 24.04.2021.
//

#ifndef PROJ2_IPK_SNIFFER_H
#define PROJ2_IPK_SNIFFER_H

#endif //PROJ2_IPK_SNIFFER_H

#include <getopt.h>
#include <cstdio>
#include <iostream>
#include <cstring>
#include <string>

#include <pcap.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

// http://yuba.stanford.edu/~casado/pcap/section1.html
pcap_if *dev;
char *net;
char *mask;
char errbuf[PCAP_ERRBUF_SIZE];
struct bpf_program pcap_prog;
bpf_u_int32 netp;
bpf_u_int32 maskp;
struct in_addr addr;

u_char* packet;
struct pcap_pkthdr* header;


struct s_args {
    s_args() : i(false), p(false), tcp(false), udp(false), arp(false), icmp(false), n(false), i_val(NULL), p_val(0), n_val(1) {}
    bool i;
    bool p;
    bool tcp;
    bool udp;
    bool arp;
    bool icmp;
    bool n;
    char *i_val;
    int p_val;
    int n_val;
};

struct s_args args;


void handler(u_char *useless, const struct pcap_pkthdr* header, const u_char* packet);

// https://www.gnu.org/software/libc/manual/html_node/Getopt-Long-Option-Example.html
void parseargs(int argc, char** argv);

void p_help();

void p_devs();

void set_filter(pcap_t *opened);

void format_time(struct timeval timestamp);

