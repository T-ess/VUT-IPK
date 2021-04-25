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
        std::cerr << "Activating the pcap failed." << std::endl;
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
    // convert IP source and destination taken from the iphdr struct to char
    // https://linux.die.net/man/3/inet_ntoa
    struct iphdr *iph = (struct iphdr*)(packet + sizeof(struct ethhdr)); 
    if (iph->protocol == TCP_PROTOCOL) {
        tcp_packet(packet, header, iph);
    } else if (iph->protocol == UDP_PROTOCOL) {
        udp_packet(packet, header, iph);
    }
}

void tcp_packet(const u_char* packet, const struct pcap_pkthdr* header, struct iphdr *iph) {
    unsigned int iph_l = (iph->ihl) * 4; // internet header length in bytes
    struct tcphdr *tcp = (struct tcphdr*)(packet + iph_l + sizeof(struct ethhdr));
    // https://stackoverflow.com/questions/4221448/getting-tcp-options-beyond-tcphdr-doff-out-of-char-eth-ip-tcp-packer-repres
    unsigned int tcphdr_len =  sizeof(struct ether_header) + iph_l + tcp->doff*4; // start of data
    struct in_addr ip_src_s;
    struct in_addr ip_dest_s;
    ip_src_s.s_addr = iph->saddr;
    ip_dest_s.s_addr = iph->daddr;
    char *ip_src = inet_ntoa(ip_src_s); // source IP
    char *ip_dest = inet_ntoa(ip_dest_s); // destination IP
    uint16_t port_src = ntohs(tcp->th_sport);
    uint16_t port_dest = ntohs(tcp->th_dport);
    std::cout << ip_src << " : " << port_src << " > " << ip_dest << " : " << port_dest;
    std::cout << ", length " << header->len << " bytes" << std::endl;
    print_content(packet, tcphdr_len, 0);
    std::cout << std::endl;
}

void udp_packet(const u_char* packet, const struct pcap_pkthdr* header, struct iphdr *iph) {
    unsigned int iph_l = (iph->ihl) * 4; // internet header length in bytes
    struct udphdr *udp = (struct udphdr*)(packet + iph_l + sizeof(struct ethhdr));
    unsigned int udphdr_len = iph_l + sizeof(udphdr) + sizeof(struct ether_header) ;
    struct in_addr ip_src_s;
    struct in_addr ip_dest_s;
    ip_src_s.s_addr = iph->saddr;
    ip_dest_s.s_addr = iph->daddr;
    char *ip_src = inet_ntoa(ip_src_s); // source IP
    char *ip_dest = inet_ntoa(ip_dest_s); // destination IP
    uint16_t port_src = ntohs(udp->uh_sport);
    uint16_t port_dest = ntohs(udp->uh_dport);
    std::cout << ip_src << " : " << port_src << " > " << ip_dest << " : " << port_dest;
    std::cout << ", length " << header->len << " bytes" << std::endl;
    print_content(packet, udphdr_len, 0);
    std::cout << std::endl;
}

/**
 * This function is taken from the site https://www.binarytides.com/packet-sniffer-code-c-libpcap-linux-sockets/
 * and edited to meet the requirements of this project.
 **/
void print_content (const u_char *data , int size, int h_size) {   
	int i , j;
	for(i = 0; i < size; i++) {
        // https://stackoverflow.com/questions/14733761/printf-formatting-for-hex
        // printf("0x%04x: ", h_size); 
		if(i%16==0)	{
			for(j = i-16; j < i; j++) {
				if (isprint(data[j])) {
                    printf("%c",(unsigned char)data[j]);
                }
				else {
                    printf(".");
                }
			}
			printf("\n");
		} 
		
		if(i%16==0) printf(" ");
		printf(" %02X",(unsigned int)data[i]);
				
		if( i == size-1) {
			for(j=0;j<15-i%16;j++) {
			  printf(" "); //extra spaces
			}
			for(j=i-i%16 ; j<=i ; j++) {
				if(data[j]>=32 && data[j]<=128) {
				  printf("%c",(unsigned char)data[j]);
				}
				else {
				  printf(".");
				}
			}
			printf("\n");
		}
	}
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
        if (filter_types.length() == 0) {
            filter = "port " + std::to_string(args.p_val);
            std::cout << filter;
        } else {
            filter = "(" + filter_types + ")" + " and " + "port " + std::to_string(args.p_val);
        }
    } else {
        filter = filter_types;
    }
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

