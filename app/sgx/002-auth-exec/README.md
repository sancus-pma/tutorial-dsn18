# Authentic execution: secure remote sensor-actuator distributed enclave application

## Application overview

After completing the `../001-attestation` exercise, remote verifiers can attest
the untampered execution of a Sancus enclave with a minimal trusted computing
base. For remote attestation to be _useful_, however, the Sancus enclave should
also report on the results of some local computation. We will therefore extend
the remote attestation protocol from the previous exercises to include proof of
a genuine sensor reading (button press) on the MSP430 side.

## Your task

The `../sancus/002-auth-exec` application should now produce a MAC over both
the attestation challenge plus a sensor reading, obtained from a locally
attested PmodBTN Sancus driver enclave. Particularly, the expected 128-bit MAC
is computed as follows:

```
sm_msg = ( challenge | btn )  # concatenation of 64-bit challenge and 16-bit sensor reading
sm_mac = MAC( msg )           # authenticate 10-byte msg
```

We already extended the untrusted runtime `main.c` to process the additional
CAN messages. The only change in the trusted enclave interface is an additional
16-bit `btn` parameter in the verification ECALL:

```C
int ecall_verify_response(uint8_t *sm_mac, uint16_t btn)
```

**Do it yourself.** Based on you `../001-attestation` enclave implementation,
extend the `ecall_verify_response()` ECALL that returns whether or not remote
attestation succeeded. Particularly, your implementation should perform the
following steps:

1. Allocate a local buffer of `ATTEST_MSG_SIZE` to hold the 
   reconstructed attestation message `sm_msg`.
2. Copy inside this buffer: the 64-bit challenge, generated by the last
   `ecall_get_challenge` call, plus the 16-bit sensor reading `btn`.
3. Compute the expected attestation MAC.
4. Compare expected and received attestation MACs.

## Building and running

First upload the `../../sancus/002-auth-exec` Sancus program to the MSP430
FPGA, then run the verifier program as follows:

```bash
$ make run CAN_INTERFACE=slcanN

--------------------------------------------------------------------------------
[main.c] remote attestation challenge
--------------------------------------------------------------------------------

[main.c] enclave returned challenge 0xf9c85275ff9ed337
[../common/can.c] send: CAN frame with ID 0x40 (len=8)
MSG    = 37 d3 9e ff 75 52 c8 f9 

--------------------------------------------------------------------------------
[main.c] remote attestation response
--------------------------------------------------------------------------------

[main.c] waiting for CAN response messages...
[../common/can.c] recv: CAN frame with ID 0x40 (len=2)
MSG    = 00 00 
[../common/can.c] recv: CAN frame with ID 0x40 (len=8)
MSG    = f8 e0 25 b9 26 fd 58 20 
[../common/can.c] recv: CAN frame with ID 0x40 (len=8)
MSG    = 5f af 65 63 9e eb 9c b4 
SM_MAC = f8 e0 25 b9 26 fd 58 20 5f af 65 63 9e eb 9c b4 

--------------------------------------------------------------------------------
[main.c] comparing expected and received MACs...
--------------------------------------------------------------------------------

ATTEST_MSG = 37 d3 9e ff 75 52 c8 f9 00 00 
MY_MAC = f8 e0 25 b9 26 fd 58 20 5f af 65 63 9e eb 9c b4 
[main.c] OK   : remote attestation succeeded (sensor reading 0x20 is authentic)!

```

**Note.** Here, slcanN refers to the CAN network interface you are using to
communicate with the MSP430 FPGA. slcan0 is the default if you ommit this
parameter.

**Note.** Since this exercise changes the source code of the `foo.c` enclave,
in order to successfully complete the remote attestation protocol in the end,
you will have to fill in the correct Sancus module-specific key in the SGX
enclave again. We refer to `../../sancus/001-attestation` for details
instructions on how to do this.
