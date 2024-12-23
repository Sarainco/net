#### 实现框架

​	在用户态实现一个STP生成树协议

#### 1.基础框架

* 使用原始套接字（Raw Socket）捕获和发送以太网帧
* 基于IEEE802.1D定义STP帧格式
* 创建一个事件驱动的主循环（使用select或epoll进行IO多路复用）

#### 2.核心模块

##### 2.1 数据结构

​	定义桥、端口、BPDU的核心数据结构

```c
typedef struct {
    int rootBridgeId;
    int rootPathCost;
    int senderBridgeId;
    int senderPortId;
} BPDU;

typedef struct {
    int portId;
    int state; // BLOCKING, FORWARDING, etc.
    int designatedCost;
    int designatedBridge;
} Port;

typedef struct {
    int bridgeId;
    int rootBridgeId;
    int rootPathCost;
    int rootPort;
    Port ports[MAX_PORTS];
} Bridge;

```

##### 2.2 网络操作模块

使用AF_PACKET套接字接收BPDU数据包并发送BPDU

```c
int create_raw_socket(const char *interface) {
    int sock = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    if (sock < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }
    struct sockaddr_ll sll = {0};
    sll.sll_family = AF_PACKET;
    sll.sll_protocol = htons(ETH_P_ALL);
    sll.sll_ifindex = if_nametoindex(interface);
    bind(sock, (struct sockaddr *)&sll, sizeof(sll));
    return sock;
}

```

##### 2.3 BPDU处理模块

实现BPDU的解析和构造

```c
void parse_bpdu(const uint8_t *frame, BPDU *bpdu) {
    // 假设 BPDU 从帧偏移 14 开始
    bpdu->rootBridgeId = (frame[14] << 8) | frame[15];
    bpdu->rootPathCost = (frame[16] << 24) | (frame[17] << 16) | (frame[18] << 8) | frame[19];
    bpdu->senderBridgeId = (frame[20] << 8) | frame[21];
    bpdu->senderPortId = frame[22];
}

void construct_bpdu(uint8_t *frame, const BPDU *bpdu) {
    // 假设 BPDU 从帧偏移 14 开始
    frame[14] = bpdu->rootBridgeId >> 8;
    frame[15] = bpdu->rootBridgeId & 0xFF;
    frame[16] = (bpdu->rootPathCost >> 24) & 0xFF;
    frame[17] = (bpdu->rootPathCost >> 16) & 0xFF;
    frame[18] = (bpdu->rootPathCost >> 8) & 0xFF;
    frame[19] = bpdu->rootPathCost & 0xFF;
    frame[20] = bpdu->senderBridgeId >> 8;
    frame[21] = bpdu->senderBridgeId & 0xFF;
    frame[22] = bpdu->senderPortId;
}

```

##### 2.4 主状态机

实现STP协议的状态机，根据接收到的BPDU更新桥的状态

```c
void process_bpdu(Bridge *bridge, int portId, const BPDU *bpdu) {
    int cost = bpdu->rootPathCost + 1;

    if (bpdu->rootBridgeId < bridge->rootBridgeId ||
        (bpdu->rootBridgeId == bridge->rootBridgeId && cost < bridge->rootPathCost)) {
        bridge->rootBridgeId = bpdu->rootBridgeId;
        bridge->rootPathCost = cost;
        bridge->rootPort = portId;

        for (int i = 0; i < bridge->numPorts; i++) {
            bridge->ports[i].state = (i == portId) ? FORWARDING : BLOCKING;
        }
    }
}

```

#### 3.运行环境

* 输入：通过配置文件或者命令行指定网卡接口和桥ID
* 输出：实时打印桥的状态和端口的状态变化。

#### 4.完整主函数

```c
int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <interface>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    const char *interface = argv[1];
    int sock = create_raw_socket(interface);

    Bridge bridge;
    initializeBridge(&bridge, 1, 4); // 初始化桥，假设 4 个端口

    uint8_t buffer[1500];
    while (1) {
        ssize_t len = recv(sock, buffer, sizeof(buffer), 0);
        if (len > 0) {
            BPDU bpdu;
            parse_bpdu(buffer, &bpdu);
            process_bpdu(&bridge, 0, &bpdu);
            displayBridgeStatus(&bridge);
        }
    }

    close(sock);
    return 0;
}

```

#### 5.实现BPDU报文发送

##### 5.1 创建原始套接字

使用socket创建可以直接发送以太网帧的原始套接字

```c
int create_raw_socket(const char *interface) {
    int sock = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    if (sock < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    struct ifreq ifr;
    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, interface, IFNAMSIZ - 1);

    if (ioctl(sock, SIOCGIFINDEX, &ifr) < 0) {
        perror("ioctl");
        close(sock);
        exit(EXIT_FAILURE);
    }

    struct sockaddr_ll sll = {0};
    sll.sll_family = AF_PACKET;
    sll.sll_protocol = htons(ETH_P_ALL);
    sll.sll_ifindex = ifr.ifr_ifindex;

    if (bind(sock, (struct sockaddr *)&sll, sizeof(sll)) < 0) {
        perror("bind");
        close(sock);
        exit(EXIT_FAILURE);
    }

    return sock;
}

```

##### 5.2 构造BPDU报文

BPDU报文是嵌套在以太网帧中的，按照IEEE 802.1D规范，BPDU数据在以太网帧格式如下：

* 目的MAC地址：01:80:C2:00:00:00（标准 STP 多播地址）
* 源MAC地址：本桥或发送端口的MAC地址
* 以太网类型：0x8000或LLC封装（通常是BPDU的特定值）
* BPDU数据：包括协议ID、版本号、BPDU类型、根桥ID等。

```c
void construct_ethernet_bpdu(uint8_t *frame, const uint8_t *src_mac, const BPDU *bpdu) {
    // 目的 MAC 地址: 01:80:C2:00:00:00
    uint8_t dest_mac[6] = {0x01, 0x80, 0xC2, 0x00, 0x00, 0x00};
    memcpy(frame, dest_mac, 6);

    // 源 MAC 地址
    memcpy(frame + 6, src_mac, 6);

    // 以太类型 (BPDU 用 0x0800)
    frame[12] = 0x42;
    frame[13] = 0x42;

    // 构造 BPDU 数据 (从偏移 14 开始)
    frame[14] = bpdu->rootBridgeId >> 8;
    frame[15] = bpdu->rootBridgeId & 0xFF;
    frame[16] = (bpdu->rootPathCost >> 24) & 0xFF;
    frame[17] = (bpdu->rootPathCost >> 16) & 0xFF;
    frame[18] = (bpdu->rootPathCost >> 8) & 0xFF;
    frame[19] = bpdu->rootPathCost & 0xFF;
    frame[20] = bpdu->senderBridgeId >> 8;
    frame[21] = bpdu->senderBridgeId & 0xFF;
    frame[22] = bpdu->senderPortId;
}

```

##### 5.3 发送BPDU报文

使用sendto函数将BPDU报文发送出去

```c
void send_bpdu(int sock, const char *interface, const BPDU *bpdu) {
    uint8_t src_mac[6] = {0x02, 0x00, 0x00, 0x00, 0x00, 0x01}; // 假设的源 MAC 地址
    uint8_t frame[1500] = {0};

    construct_ethernet_bpdu(frame, src_mac, bpdu);

    struct sockaddr_ll sll = {0};
    sll.sll_family = AF_PACKET;
    sll.sll_protocol = htons(ETH_P_ALL);
    sll.sll_ifindex = if_nametoindex(interface);

    if (sendto(sock, frame, 60, 0, (struct sockaddr *)&sll, sizeof(sll)) < 0) {
        perror("sendto");
    } else {
        printf("BPDU sent successfully!\n");
    }
}

```

##### 5.3 示例主函数

```c
int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <interface>\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char *interface = argv[1];
    int sock = create_raw_socket(interface);

    BPDU bpdu = {
        .rootBridgeId = 1,
        .rootPathCost = 0,
        .senderBridgeId = 1,
        .senderPortId = 1,
    };

    while (1) {
        send_bpdu(sock, interface, &bpdu);
        sleep(2); // 每 2 秒发送一次
    }

    close(sock);
    return 0;
}

```

