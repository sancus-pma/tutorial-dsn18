/* utility headers */
#include <string.h>
#include "debug.h"
#include "can.h"

/* SGX untrusted runtime */
#include <sgx_urts.h>
#include "Enclave/encl_u.h"

/* for configurable SPONGENT_KEY_SIZE */
#include "../libspongent/libspongent/spongewrap.h"
#include "../libspongent/libspongent/config.h"

#if SPONGENT_TAG_SIZE != (CAN_PAYLOAD_SIZE*2)
    #error expecting 128-bit Sancus core
#endif

#ifndef CAN_INTERFACE
#define CAN_INTERFACE       "slcan0"
#endif
#define CAN_ATTEST_ID        0x40

int can_socket = -1;

sgx_enclave_id_t create_enclave(void)
{
    sgx_launch_token_t token = {0};
    int updated = 0;
    sgx_enclave_id_t eid = -1;

    info_event("Creating enclave...");
    SGX_ASSERT( sgx_create_enclave( "./Enclave/encl.so", /*debug=*/1,
                                    &token, &updated, &eid, NULL ) );

    return eid;
}

int main( int argc, char **argv )
{
    uint64_t challenge = 0x0;
    uint8_t sm_mac[SPONGENT_TAG_SIZE] = {0x0};
    uint16_t id = 0x0;
    int i, len, rv;
    sgx_enclave_id_t eid = create_enclave();

    info("setup");
    ASSERT( (can_socket = can_open(CAN_INTERFACE)) >= 0 );

    /* ---------------------------------------------------------------------- */
    info_event("remote attestation challenge");
    SGX_ASSERT( ecall_get_challenge(eid, &challenge) );
    info("enclave returned challenge 0x%llx", challenge);
    ASSERT( can_send(can_socket, CAN_ATTEST_ID, (uint8_t*) &challenge, sizeof(uint64_t)) > 0 );

    /* ---------------------------------------------------------------------- */
    info_event("remote attestation response");

    info("waiting for CAN response messages...");
    len = can_recv(can_socket, &id, &sm_mac[0]);
    ASSERT( (id == CAN_ATTEST_ID) && (len == CAN_PAYLOAD_SIZE) &&
            "unexpected CAN attestation challenge message format" );

    len = can_recv(can_socket, &id, &sm_mac[CAN_PAYLOAD_SIZE]);
    ASSERT( (id == CAN_ATTEST_ID) && (len == CAN_PAYLOAD_SIZE) &&
            "unexpected CAN attestation challenge message format" );
    dump_hex("SM_MAC", sm_mac, SPONGENT_TAG_SIZE);

    /* ---------------------------------------------------------------------- */
    info_event("comparing expected and received MACs...");
    SGX_ASSERT( ecall_verify_response(eid, &rv, sm_mac) );

    if (rv)
        info("OK   : remote attestation succeeded!");
    else
        info("FAIL : remote attestation failed!");

    /* ---------------------------------------------------------------------- */
    can_close(can_socket);

	return 0;
}
