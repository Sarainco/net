#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_PORTS 16
#define INF 9999

// BPDU 类型
typedef struct {
    int rootBridgeId;      // 根桥 ID
    int rootPathCost;      // 到根桥的成本
    int senderBridgeId;    // 发送者桥 ID
    int senderPortId;      // 发送者端口 ID
} BPDU;

// 端口状态
typedef enum {
    PORT_DISABLED,
    PORT_BLOCKING,
    PORT_LISTENING,
    PORT_LEARNING,
    PORT_FORWARDING
} PortState;

// 端口信息
typedef struct {
    int portId;            // 端口 ID
    PortState state;       // 端口状态
    int designatedCost;    // 指定成本
    int designatedBridge;  // 指定桥
    int designatedPort;    // 指定端口
} Port;

// 桥信息
typedef struct {
    int bridgeId;          // 桥 ID
    Port ports[MAX_PORTS]; // 端口数组
    int numPorts;          // 端口数量
    int rootBridgeId;      // 根桥 ID
    int rootPathCost;      // 到根桥的成本
    int rootPort;          // 根端口
} Bridge;

// 初始化桥
void initializeBridge(Bridge *bridge, int bridgeId, int numPorts) {
    bridge->bridgeId = bridgeId;
    bridge->numPorts = numPorts;
    bridge->rootBridgeId = bridgeId;
    bridge->rootPathCost = 0;
    bridge->rootPort = -1;

    for (int i = 0; i < numPorts; i++) {
        bridge->ports[i].portId = i;
        bridge->ports[i].state = PORT_BLOCKING;
        bridge->ports[i].designatedCost = INF;
        bridge->ports[i].designatedBridge = bridgeId;
        bridge->ports[i].designatedPort = i;
    }
}

// 模拟接收 BPDU
void receiveBPDU(Bridge *bridge, int portId, BPDU bpdu) {
    int cost = bpdu.rootPathCost + 1;

    if (bpdu.rootBridgeId < bridge->rootBridgeId ||
        (bpdu.rootBridgeId == bridge->rootBridgeId && cost < bridge->rootPathCost) ||
        (bpdu.rootBridgeId == bridge->rootBridgeId && cost == bridge->rootPathCost && bpdu.senderBridgeId < bridge->bridgeId)) {
        bridge->rootBridgeId = bpdu.rootBridgeId;
        bridge->rootPathCost = cost;
        bridge->rootPort = portId;

        for (int i = 0; i < bridge->numPorts; i++) {
            if (i == portId) {
                bridge->ports[i].state = PORT_FORWARDING;
            } else {
                bridge->ports[i].state = PORT_BLOCKING;
            }
        }
    }
}

// 模拟发送 BPDU
void sendBPDU(Bridge *bridge, int portId, BPDU *bpdu) {
    bpdu->rootBridgeId = bridge->rootBridgeId;
    bpdu->rootPathCost = bridge->rootPathCost;
    bpdu->senderBridgeId = bridge->bridgeId;
    bpdu->senderPortId = portId;
}

// 显示桥状态
void displayBridgeStatus(Bridge *bridge) {
    printf("Bridge ID: %d\n", bridge->bridgeId);
    printf("Root Bridge ID: %d\n", bridge->rootBridgeId);
    printf("Root Path Cost: %d\n", bridge->rootPathCost);
    printf("Root Port: %d\n", bridge->rootPort);
    printf("Ports:\n");

    for (int i = 0; i < bridge->numPorts; i++) {
        printf("  Port %d: State = %d\n", bridge->ports[i].portId, bridge->ports[i].state);
    }
}

int main() {
    // 初始化桥 A 和桥 B
    Bridge bridgeA, bridgeB;
    initializeBridge(&bridgeA, 1, 3);
    initializeBridge(&bridgeB, 2, 3);

    // 模拟发送和接收 BPDU
    BPDU bpdu;
    sendBPDU(&bridgeA, 0, &bpdu);
    receiveBPDU(&bridgeB, 1, bpdu);

    sendBPDU(&bridgeB, 1, &bpdu);
    receiveBPDU(&bridgeA, 0, bpdu);

    // 显示状态
    printf("Bridge A Status:\n");
    displayBridgeStatus(&bridgeA);

    printf("\nBridge B Status:\n");
    displayBridgeStatus(&bridgeB);

    return 0;
}

