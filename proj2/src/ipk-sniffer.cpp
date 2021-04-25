//
// Created by terez on 24.04.2021.
//

#include "ipk-sniffer.h"


int main(int argc, char* argv[]) {
    parseargs(argc, argv);
    if (args.i_val == NULL) { //TODO
        p_devs();
        return 0;
    }
    pcap_t *opened = pcap_create(args.i_val, errbuf);
    if (pcap_set_promisc(opened, 1) != 0) {
        std::cerr << "Setting the promisc mode failed." << std::endl;
        exit(1);
    }
    pcap_set_timeout(opened, 100);
    if (pcap_set_snaplen(opened, 65535) != 0) {
        std::cerr << "Setting the snaplen failed." << std::endl;
        exit(1);
    }
    if (pcap_activate(opened) != 0) {
        std::cerr << "Activeting the pcap failed." << std::endl;
        pcap_close(opened);
        exit(1);
    }
    set_filter(opened);
    if (pcap_loop(opened, args.n_val, handler, packet) != 0) {
        std::cerr << "pcap_loop failed." << std::endl;
        exit(1);
    }
}

void handler(u_char *useless, const struct pcap_pkthdr* header, const u_char* packet) {
    format_time(header->ts);
}

void parseargs(int argc, char** argv) {
    int arg;
    char *pEnd;
    extern char *optarg;
    if (argc < 2) {
        return;
    }
    while (1) {
        static struct option long_options[] = {
                {"interface", optional_argument, 0, 'i'},
                {"tcp",       no_argument,       0, 't'},
                {"udp",       no_argument,       0, 'u'},
                {"arp",       no_argument,       0, 'a'},
                {"icmp",      no_argument,       0, 'c'},
                {"help",      no_argument,       0, 'h'},
                {0,           0,                 0, 0}
        };
        int index = 0;
        arg = getopt_long(argc, argv, "i::p:tun:h", long_options, &index);
        if (arg == -1) {
            // end of arguments
            break;
        }
        switch (arg) {
            case 'i':
                args.i = true;
                args.i_val = optarg;
                break;
            case 'p':
                args.p = true;
                args.p_val = strtol(optarg, &pEnd, 10);
                break;
            case 't':
                args.tcp = true;
                break;
            case 'u':
                args.udp = true;
                break;
            case 'a':
                args.arp = true;
                break;
            case 'c':
                args.icmp = true;
                break;
            case 'n':
                args.n = true;
                args.n_val = strtol(optarg, &pEnd, 10);
                break;
            case 'h':
                p_help();
                break;
            case '?':
                break;
        }
    }
}

void p_help() {
    printf("help");
    exit(0);
}

void p_devs() {
    int test = pcap_findalldevs(&dev, errbuf);
    if (test == PCAP_ERROR) {
        printf("uh oh");
        exit(1);
    }
    while (dev->next != NULL) {
        printf("%s\n", dev->name);
        dev = dev->next;
    }
}

void set_filter(pcap_t *opened) {
    std::string filter = "";
    std::string filter_types = "";
    if (args.udp) {
        if (filter_types.length() == 0) filter_types += "udp";
        else filter_types += " or udp";
    }
    if (args.tcp) {
        if (filter_types.length() == 0) filter_types += "tcp";
        else filter_types += " or tcp";
    }
    if (args.icmp) {
        if (filter_types.length() == 0) filter_types += "icmp";
        else filter_types += " or icmp";
    }
    if (args.arp) {
        if (filter_types.length() == 0) filter_types += "arp";
        else filter_types += " or arp";
    }
    if (args.p) {
        filter = "(" + filter_types + ")" + " and " + "port " + std::to_string(args.p_val);
    } else {
        filter = filter_types;
    }
    std::cout << filter << "\n";
    if (filter.length() == 0) {
        return;
    }
    const char* filter_c = filter.c_str();
    if (pcap_compile(opened, &pcap_prog, filter_c, 0, netp) != 0) {
        std::cerr << "Compiling the filter failed." << std::endl;
        exit(1);
    }
    if (pcap_setfilter(opened, &pcap_prog) != 0) {
        std::cerr << "Setting the filter failed." << std::endl;
        exit(1);
    }
}

void format_time(struct timeval timestamp) {
    time_t t = time(&timestamp.tv_sec);
    char new_time[60];
    char new_time2[60];
    struct tm *tmp = localtime(&t);
    char const *format = "%Y-%m-%dT%H:%M:%S.";
    strftime(new_time, 60, format, tmp);
    strftime(new_time2, 60, "%z", tmp);
    int ms = timestamp.tv_usec/10;
    std::string ms_s = std::to_string(ms);

    std::cout << new_time << ms_s.substr(0, 3) << new_time2 << " ";
}

