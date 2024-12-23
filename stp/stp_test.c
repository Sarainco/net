#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_PORTS 16
#define INF 9999

// BPDU ����
typedef struct {
    int rootBridgeId;      // ���� ID
    int rootPathCost;      // �����ŵĳɱ�
    int senderBridgeId;    // �������� ID
    int senderPortId;      // �����߶˿� ID
} BPDU;

// �˿�״̬
typedef enum {
    PORT_DISABLED,
    PORT_BLOCKING,
    PORT_LISTENING,
    PORT_LEARNING,
    PORT_FORWARDING
} PortState;

// �˿���Ϣ
typedef struct {
    int portId;            // �˿� ID
    PortState state;       // �˿�״̬
    int designatedCost;    // ָ���ɱ�
    int designatedBridge;  // ָ����
    int designatedPort;    // ָ���˿�
} Port;

// ����Ϣ
typedef struct {
    int bridgeId;          // �� ID
    Port ports[MAX_PORTS]; // �˿�����
    int numPorts;          // �˿�����
    int rootBridgeId;      // ���� ID
    int rootPathCost;      // �����ŵĳɱ�
    int rootPort;          // ���˿�
} Bridge;

// ��ʼ����
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

// ģ����� BPDU
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

// ģ�ⷢ�� BPDU
void sendBPDU(Bridge *bridge, int portId, BPDU *bpdu) {
    bpdu->rootBridgeId = bridge->rootBridgeId;
    bpdu->rootPathCost = bridge->rootPathCost;
    bpdu->senderBridgeId = bridge->bridgeId;
    bpdu->senderPortId = portId;
}

// ��ʾ��״̬
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
    // ��ʼ���� A ���� B
    Bridge bridgeA, bridgeB;
    initializeBridge(&bridgeA, 1, 3);
    initializeBridge(&bridgeB, 2, 3);

    // ģ�ⷢ�ͺͽ��� BPDU
    BPDU bpdu;
    sendBPDU(&bridgeA, 0, &bpdu);
    receiveBPDU(&bridgeB, 1, bpdu);

    sendBPDU(&bridgeB, 1, &bpdu);
    receiveBPDU(&bridgeA, 0, bpdu);

    // ��ʾ״̬
    printf("Bridge A Status:\n");
    displayBridgeStatus(&bridgeA);

    printf("\nBridge B Status:\n");
    displayBridgeStatus(&bridgeB);

    return 0;
}

