#define _POSIX_C_SOURCE 200809L
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include "merc.h"
#include "telnet_input.h"

extern bool read_from_descriptor(DESCRIPTOR_DATA *d);
extern void read_from_buffer(DESCRIPTOR_DATA *d);
extern const char echo_off_str[], echo_on_str[];

static DESCRIPTOR_DATA d;
static int peer;

static void begin(void)
{
    int pair[2];
    assert(socketpair(AF_UNIX, SOCK_STREAM, 0, pair) == 0);
    memset(&d, 0, sizeof(d));
    d.descriptor = pair[0];
    peer = pair[1];
    assert(fcntl(pair[0], F_SETFL, O_NONBLOCK) == 0);
    d.host = "synthetic-telnet-test";
    d.outsize = 2000;
    d.outbuf = alloc_mem(d.outsize);
}

static void finish(void)
{
    close((int)d.descriptor);
    close(peer);
    free_mem(d.outbuf, d.outsize);
}

static void send_bytes(const unsigned char *bytes, size_t count)
{
    if (count == 0) return;
    assert(write(peer, bytes, count) == (ssize_t)count);
    assert(read_from_descriptor(&d));
}

static void command_is(const char *expected)
{
    read_from_buffer(&d);
    assert(strcmp(d.incomm, expected) == 0);
}

static void negotiation(void)
{
    static const unsigned char bytes[] = {
        255, 251, 91, 'l', 'o', 'o', 'k', '\r', '\n'
    };
    begin();
    send_bytes(bytes, sizeof(bytes));
    command_is("look");
    assert(d.outtop == 3);
    assert(memcmp(d.outbuf, "\xff\xfe\x5b", 3) == 0);
    finish();
}

static void fragments(void)
{
    static const unsigned char bytes[] = {
        255, 251, 91, 255, 250, 24, 0, 'M', 'u', 'd',
        255, 255, '\n', 0, 255, 240, 'l', 'o', 'o', 'k', '\r', '\n'
    };
    for (size_t cut = 0; cut <= sizeof(bytes); ++cut) {
        begin();
        send_bytes(bytes, cut);
        send_bytes(bytes + cut, sizeof(bytes) - cut);
        command_is("look");
        assert(d.outtop == 3 && d.telnet_state == TELNET_DATA);
        finish();
    }
    begin();
    for (size_t i = 0; i < sizeof(bytes); ++i) send_bytes(bytes + i, 1);
    command_is("look");
    finish();
}

static void options(void)
{
    for (unsigned int verb = 251; verb <= 254; ++verb) {
        for (unsigned int option = 0; option <= 255; ++option) {
            unsigned char bytes[] = {255, (unsigned char)verb,
                (unsigned char)option, 'l', 'o', 'o', 'k', '\n'};
            begin();
            send_bytes(bytes, sizeof(bytes));
            command_is("look");
            if (verb == 251 || (verb == 253 && option != 1 && option != 201)) {
                unsigned char expected[] = {255, verb == 251 ? 254 : 252,
                    (unsigned char)option};
                assert(d.outtop == 3 && memcmp(d.outbuf, expected, 3) == 0);
            } else assert(d.outtop == 0);
            finish();
        }
    }
}

static void binary_and_echo(void)
{
    static const unsigned char data[] = {
        'l', 0, 'o', 255, 255, 'o', 'k', '\r', 0, '\n'
    };
    static const unsigned char ack[] = {255, 253, 1};
    static const unsigned char negative_ack[] = {255, 254, 1};
    begin();
    d.fcommand = TRUE;
    write_to_buffer(&d, echo_off_str, 3);
    long hidden = d.outtop;
    assert(hidden >= 3 && memcmp(d.outbuf, "\xff\xfb\x01", 3) == 0);
    send_bytes(ack, sizeof(ack));
    assert(d.outtop == hidden); /* Never answer this ACK with WONT ECHO. */
    send_bytes(data, sizeof(data));
    command_is("look");
    write_to_buffer(&d, echo_on_str, 3);
    long restored = d.outtop;
    send_bytes(negative_ack, sizeof(negative_ack));
    assert(d.outtop == restored &&
           memcmp(d.outbuf + hidden, "\xff\xfc\x01", 3) == 0);
    finish();
}

static void binary_output_growth(void)
{
    static const unsigned char options[] = {255, 253, 0, 255, 251, 91};
    static const unsigned char expected[] = {255, 252, 0, 255, 254, 91};
    begin();
    free_mem(d.outbuf, d.outsize);
    d.outsize = 4;
    d.outbuf = alloc_mem(d.outsize);
    send_bytes(options, sizeof(options));
    assert(d.outtop == 6 && memcmp(d.outbuf, expected, sizeof(expected)) == 0);
    write_to_buffer(&d, "ordinary text must preserve pending binary replies", 0);
    assert(memcmp(d.outbuf, expected, sizeof(expected)) == 0);
    assert(memcmp(d.outbuf + 6, "ordinary text", 13) == 0);
    finish();
}

static void bounded(void)
{
    unsigned char flood[3 * (4 * MAX_INPUT_LENGTH - 10)];
    unsigned char peek;
    static const unsigned char one[] = {'x'};
    static const unsigned char offer[] = {255, 251, 91};
    for (size_t i = 0; i < sizeof(flood); ++i) flood[i] = i % 2 ? 241 : 255;
    begin();
    assert(write(peer, flood, sizeof(flood)) == (ssize_t)sizeof(flood));
    assert(read_from_descriptor(&d));
    assert(d.inbuf[0] == '\0');
    assert(recv((int)d.descriptor, &peek, 1, MSG_PEEK | MSG_DONTWAIT) == 1);
    finish();

    begin();
    memset(d.inbuf, 'x', sizeof(d.inbuf) - 10);
    d.inbuf[sizeof(d.inbuf) - 10] = '\0';
    assert(!telnet_input(&d, one, sizeof(one)));
    finish();

    begin();
    free_mem(d.outbuf, d.outsize);
    d.outsize = 128000;
    d.outbuf = alloc_mem(d.outsize);
    d.outtop = d.outsize - 3;
    assert(!telnet_input(&d, offer, sizeof(offer)));
    finish();

    begin();
    d.telnet_state = 255;
    assert(!telnet_input(&d, one, sizeof(one)));
    finish();
    assert(!telnet_input(NULL, one, sizeof(one)));
}

static void unknown_commands(void)
{
    static const unsigned char bytes[] = {
        255, 241, 255, 249, 255, 240, 's', 'a', 'y', ' ', 'h', 'i', '\n',
        'l', 'o', 'o', 'k', '\n'
    };
    begin();
    send_bytes(bytes, sizeof(bytes));
    command_is("say hi");
    d.incomm[0] = '\0';
    command_is("look");
    finish();
}

int main(void)
{
    negotiation();
    fragments();
    options();
    binary_and_echo();
    binary_output_growth();
    bounded();
    unknown_commands();
    puts("Telnet socket regressions passed");
    return 0;
}
