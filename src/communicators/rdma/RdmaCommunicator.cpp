//
// Created by Mariano Aponte on 07/12/23.
//

#include <iostream>
#include <sstream>
#include <arpa/inet.h>
#include "RdmaCommunicator.h"

#include <gvirtus/communicators/Endpoint.h>
#include <gvirtus/communicators/Endpoint_Tcp.h>
#include <gvirtus/communicators/Endpoint_Rdma.h>


using gvirtus::communicators::RdmaCommunicator;

RdmaCommunicator::RdmaCommunicator(char * hostname, char * port) {
#ifdef DEBUG
    std::cout << "Called " << "RdmaCommunicator(char * hostname, char * port)" << std::endl;
#endif

    if (port == nullptr or std::string(port).empty()) {
        throw "RdmaCommunicator: Port not specified...";
    }

    hostent *ent = gethostbyname(hostname);
    if (ent == NULL) {
        throw "RdmaCommunicator: Can't resolve hostname \"" + std::string(hostname) + "\"...";
    }

    auto addrLen = ent->h_length;
    this->hostname = new char[addrLen];
    memcpy(this->hostname, *ent->h_addr_list, addrLen);
    this->port = port;

    memset(&rdmaCmId, 0, sizeof(rdmaCmId));
    memset(&rdmaCmListenId, 0, sizeof(rdmaCmListenId));
}

RdmaCommunicator::RdmaCommunicator(rdma_cm_id *rdmaCmId) {
#ifdef DEBUG
    std::cout << "Called " << "RdmaCommunicator(rdma_cm_id *rdmaCmId)" << std::endl;
#endif
    this->rdmaCmId = rdmaCmId;
    preregisteredMr = ktm_rdma_reg_msgs(rdmaCmId, preregisteredBuffer, 1024 * 5);
}

RdmaCommunicator::~RdmaCommunicator() {
#ifdef DEBUG
    std::cout << "Called " << "~RdmaCommunicator()" << std::endl;
#endif
    rdma_disconnect(rdmaCmId);
    rdma_destroy_id(rdmaCmId);
}

void RdmaCommunicator::Serve() {
#ifdef DEBUG
    std::cout << "Called " << "Serve()" << std::endl;
#endif
    // Setup address info
    rdma_addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_port_space = rdma_port_space::RDMA_PS_IB;
    hints.ai_flags = RAI_PASSIVE;

    rdma_addrinfo * rdmaAddrinfo;

    char testhost[50] = "192.168.4.101";
    char testport[50] = "9999";
    ktm_rdma_getaddrinfo(testhost, testport, &hints, &rdmaAddrinfo);

    // Create communication manager id with queue pair attributes
    ibv_qp_init_attr qpInitAttr;
    memset(&qpInitAttr, 0, sizeof(qpInitAttr));

    qpInitAttr.cap.max_send_wr = 10;
    qpInitAttr.cap.max_recv_wr = 10;
    qpInitAttr.cap.max_send_sge = 10;
    qpInitAttr.cap.max_recv_sge = 10;
    qpInitAttr.sq_sig_all = 1;
    qpInitAttr.qp_type = ibv_qp_type::IBV_QPT_RC;

    ktm_rdma_create_ep(&rdmaCmListenId, rdmaAddrinfo, NULL, &qpInitAttr);
    rdma_freeaddrinfo(rdmaAddrinfo);

    // Listen for connections
    ktm_rdma_listen(rdmaCmListenId, BACKLOG);
}

const gvirtus::communicators::Communicator *const RdmaCommunicator::Accept() const {
#ifdef DEBUG
    std::cout << "Called " << "Accept()" << std::endl;
#endif
    rdma_cm_id * clientRdmaCmId;

    ktm_rdma_get_request(rdmaCmListenId, &clientRdmaCmId);
    ktm_rdma_accept(clientRdmaCmId, nullptr);

    auto *ibvQpAttr = static_cast<ibv_qp_attr *>(malloc(sizeof(ibv_qp_attr)));
    ibvQpAttr->min_rnr_timer = 1;
    if (ibv_modify_qp(clientRdmaCmId->qp, ibvQpAttr, IBV_QP_MIN_RNR_TIMER)) {
        fprintf(stderr, "ibv_modify_attr() failed: %s\n", strerror(errno));
    }

    return new RdmaCommunicator(clientRdmaCmId);
}

void RdmaCommunicator::Connect() {
#ifdef DEBUG
    std::cout << "Called " << "Connect()" << std::endl;
#endif
    // Setup address info
    rdma_addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_port_space = rdma_port_space::RDMA_PS_IB;

    rdma_addrinfo * rdmaAddrinfo;

    char testhost[50] = "192.168.4.101";
    char testport[50] = "9999";
    ktm_rdma_getaddrinfo(testhost, testport, &hints, &rdmaAddrinfo);

    // Create communication manager id with queue pair attributes
    ibv_qp_init_attr qpInitAttr;
    memset(&qpInitAttr, 0, sizeof(qpInitAttr));

    qpInitAttr.cap.max_send_wr = 10;
    qpInitAttr.cap.max_recv_wr = 10;
    qpInitAttr.cap.max_send_sge = 10;
    qpInitAttr.cap.max_recv_sge = 10;
    qpInitAttr.sq_sig_all = 1;
    qpInitAttr.qp_type = ibv_qp_type::IBV_QPT_RC;

    ktm_rdma_create_ep(&rdmaCmId, rdmaAddrinfo, nullptr, &qpInitAttr);
    rdma_freeaddrinfo(rdmaAddrinfo);

    ktm_rdma_connect(rdmaCmId, nullptr);

    auto *ibvQpAttr = static_cast<ibv_qp_attr *>(malloc(sizeof(ibv_qp_attr)));
    ibvQpAttr->min_rnr_timer = 1;
    if (ibv_modify_qp(rdmaCmId->qp, ibvQpAttr, IBV_QP_MIN_RNR_TIMER)) {
        fprintf(stderr, "ibv_modify_attr() failed: %s\n", strerror(errno));
    }
    preregisteredMr = ktm_rdma_reg_msgs(rdmaCmId, preregisteredBuffer, 1024 * 5);
}

size_t RdmaCommunicator::Read(char *buffer, size_t size) {
#ifdef DEBUG
    std::cout << "Called " << "Read(char *buffer, size_t size)" << std::endl;
    std::cout << "Size: " << size << std::endl;
#endif
    if (size < 1024 * 5) {
        ktm_rdma_post_recv(rdmaCmId, nullptr, preregisteredBuffer, size, preregisteredMr);
    }
    else {
        memoryRegion = ktm_rdma_reg_msgs(rdmaCmId, buffer, size);
        ktm_rdma_post_recv(rdmaCmId, nullptr, buffer, size, memoryRegion);
    }

    int num_comp;
    do num_comp = ibv_poll_cq(rdmaCmId->recv_cq, 1, &workCompletion); while (num_comp == 0);
    if (num_comp < 0) throw "ibv_poll_cq() failed";
    if (workCompletion.status != IBV_WC_SUCCESS) throw "Failed status " + std::string(ibv_wc_status_str(workCompletion.status));

    if (size < 1024 * 5) {
        memcpy(buffer, preregisteredBuffer, size);
    }

    return size;
}

size_t RdmaCommunicator::Write(const char *buffer, size_t size) {
#ifdef DEBUG
    std::cout << "Called " << "Write(const char *buffer, size_t size)" << std::endl;
    std::cout << "Size: " << size << std::endl;
#endif
    char * actualBuffer = nullptr;

    if (size < 1024 * 5) {
        memcpy(preregisteredBuffer, buffer, size);
        ktm_rdma_post_send(rdmaCmId, nullptr, preregisteredBuffer, size, preregisteredMr, IBV_SEND_SIGNALED);
    }
    else {
        actualBuffer = (char *) malloc(size);
        memcpy(actualBuffer, buffer, size);
        memoryRegion = ktm_rdma_reg_msgs(rdmaCmId, actualBuffer, size);
        ktm_rdma_post_send(rdmaCmId, nullptr, actualBuffer, size, memoryRegion, IBV_SEND_SIGNALED);
    }

    int num_comp;
    do num_comp = ibv_poll_cq(rdmaCmId->send_cq, 1, &workCompletion); while (num_comp == 0);
    if (num_comp < 0) throw "ibv_poll_cq() failed";
    if (workCompletion.status != IBV_WC_SUCCESS) throw "Failed status " + std::string(ibv_wc_status_str(workCompletion.status));

    if (size > 1024 * 5) {
        free(actualBuffer);
    }

    return size;
}

void RdmaCommunicator::Sync() {
#ifdef DEBUG
    std::cout << "RdmaCommunicator::Sync(): called." << std::endl;
#endif
}

void RdmaCommunicator::Close() {
#ifdef DEBUG
    std::cout << "RdmaCommunicator::Close(): called." << std::endl;
#endif
    rdma_disconnect(rdmaCmId);
    rdma_destroy_id(rdmaCmId);
}


extern "C" std::shared_ptr <RdmaCommunicator> create_communicator(std::shared_ptr <gvirtus::communicators::Endpoint> end) {
    /*
    std::string arg =
            "rdma://" +
            std::dynamic_pointer_cast<gvirtus::communicators::Endpoint_Rdma>(end)->address() +
            ":" +
            std::to_string(std::dynamic_pointer_cast<gvirtus::communicators::Endpoint_Rdma>(end)->port());
*/
    std::string hostname;
    std::string port;

    hostname = std::dynamic_pointer_cast<gvirtus::communicators::Endpoint_Rdma>(end)->address();
    port = std::to_string(std::dynamic_pointer_cast<gvirtus::communicators::Endpoint_Rdma>(end)->port());

    return std::make_shared<RdmaCommunicator>(hostname.data(), port.data());
}


