#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <linux/if_packet.h>
#include <linux/if_ether.h>

#define MAX_PORTS 16
#define ETH_BPDU_TYPE 0x4242 // 自定义以太网类型
#define STP_DEST_MAC "\x01\x80\xC2\x00\x00\x00"

typedef struct {
    int rootBridgeId;
    int rootPathCost;
    int senderBridgeId;
    int senderPortId;
} BPDU;

typedef struct {
    int portId;
    int state; // 0: BLOCKING, 1: FORWARDING
    int designatedCost;
    int designatedBridge;
} Port;

typedef struct {
    int bridgeId;
    int rootBridgeId;
    int rootPathCost;
    int rootPort;
    Port ports[MAX_PORTS];
    int numPorts;
} Bridge;


int create_raw_socket(const char *interface) {
    int sock = socket(AF_PACKET, SOCK_RAW, htons(ETH_BPDU_TYPE));
    if (sock < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    struct ifreq ifr = {0};
    strncpy(ifr.ifr_name, interface, IFNAMSIZ - 1);

    if (ioctl(sock, SIOCGIFINDEX, &ifr) < 0) {
        perror("ioctl");
        close(sock);
        exit(EXIT_FAILURE);
    }

    struct sockaddr_ll sll = {0};
    sll.sll_family = AF_PACKET;
    sll.sll_protocol = htons(ETH_BPDU_TYPE);
    sll.sll_ifindex = ifr.ifr_ifindex;

    if (bind(sock, (struct sockaddr *)&sll, sizeof(sll)) < 0) {
        perror("bind");
        close(sock);
        exit(EXIT_FAILURE);
    }

    return sock;
}


void construct_bpdu_frame(uint8_t *frame, const uint8_t *src_mac, const BPDU *bpdu) {
    memcpy(frame, STP_DEST_MAC, 6);        // 目的 MAC 地址
    memcpy(frame + 6, src_mac, 6);        // 源 MAC 地址
    frame[12] = ETH_BPDU_TYPE >> 8;       // 以太网类型高字节
    frame[13] = ETH_BPDU_TYPE & 0xFF;     // 以太网类型低字节

    frame[14] = bpdu->rootBridgeId >> 8;  // 根桥 ID
    frame[15] = bpdu->rootBridgeId & 0xFF;
    frame[16] = bpdu->rootPathCost >> 24; // 根路径代价
    frame[17] = (bpdu->rootPathCost >> 16) & 0xFF;
    frame[18] = (bpdu->rootPathCost >> 8) & 0xFF;
    frame[19] = bpdu->rootPathCost & 0xFF;
    frame[20] = bpdu->senderBridgeId >> 8; // 发送者桥 ID
    frame[21] = bpdu->senderBridgeId & 0xFF;
    frame[22] = bpdu->senderPortId;       // 发送者端口 ID
}

void parse_bpdu_frame(const uint8_t *frame, BPDU *bpdu) {
    bpdu->rootBridgeId = (frame[14] << 8) | frame[15];
    bpdu->rootPathCost = (frame[16] << 24) | (frame[17] << 16) | (frame[18] << 8) | frame[19];
    bpdu->senderBridgeId = (frame[20] << 8) | frame[21];
    bpdu->senderPortId = frame[22];
}



void process_bpdu(Bridge *bridge, int portId, const BPDU *bpdu) {
    int cost = bpdu->rootPathCost + 1;

    // 判断是否需要更新根桥信息
    if (bpdu->rootBridgeId < bridge->rootBridgeId ||
        (bpdu->rootBridgeId == bridge->rootBridgeId && cost < bridge->rootPathCost)) {
        bridge->rootBridgeId = bpdu->rootBridgeId;
        bridge->rootPathCost = cost;
        bridge->rootPort = portId;

        for (int i = 0; i < bridge->numPorts; i++) {
            bridge->ports[i].state = (i == portId) ? 1 : 0; // 1: FORWARDING, 0: BLOCKING
        }

        printf("Updated root bridge to %d, root port %d\n", bridge->rootBridgeId, portId);
    }
}

void display_bridge_status(const Bridge *bridge) {
    printf("Bridge ID: %d, Root Bridge ID: %d, Root Path Cost: %d, Root Port: %d\n",
           bridge->bridgeId, bridge->rootBridgeId, bridge->rootPathCost, bridge->rootPort);

    for (int i = 0; i < bridge->numPorts; i++) {
        printf("Port %d: %s\n", i,
               bridge->ports[i].state ? "FORWARDING" : "BLOCKING");
    }
}




int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <interface>\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char *interface = argv[1];
    int sock = create_raw_socket(interface);

    uint8_t src_mac[6] = {0x02, 0x00, 0x00, 0x00, 0x00, 0x01}; // 假设源 MAC
    uint8_t frame[1500];
    Bridge bridge = {0};
    bridge.bridgeId = 1;
    bridge.rootBridgeId = 1;
    bridge.rootPathCost = 0;
    bridge.numPorts = 1;

    BPDU bpdu = {
        .rootBridgeId = 1,
        .rootPathCost = 0,
        .senderBridgeId = 1,
        .senderPortId = 1,
    };

    fd_set readfds;
    while (1) {
        FD_ZERO(&readfds);
        FD_SET(sock, &readfds);

        struct timeval timeout = {2, 0}; // 每 2 秒发送一次 BPDU

        if (select(sock + 1, &readfds, NULL, NULL, &timeout) > 0) {
            ssize_t len = recv(sock, frame, sizeof(frame), 0);
            if (len > 0) {
                BPDU received_bpdu;
                parse_bpdu_frame(frame, &received_bpdu);
                process_bpdu(&bridge, 0, &received_bpdu);
                display_bridge_status(&bridge);
            }
        } else {
            construct_bpdu_frame(frame, src_mac, &bpdu);
            send(sock, frame, 60, 0);
            printf("Sent BPDU\n");
        }
    }

    close(sock);
    return 0;
}



