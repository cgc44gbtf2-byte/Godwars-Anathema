#define _POSIX_C_SOURCE 200809L
#include <assert.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include "merc.h"
#include "gmcp.h"
#include "telnet_input.h"

extern bool read_from_descriptor(DESCRIPTOR_DATA *d);
extern void read_from_buffer(DESCRIPTOR_DATA *d);
extern bool process_output(DESCRIPTOR_DATA *d, bool prompt);
static DESCRIPTOR_DATA d;
static CHAR_DATA ch, enemy;
static PC_DATA pc;
static int peer;

static void begin(void)
{
    int pair[2];
    assert(socketpair(AF_UNIX, SOCK_STREAM, 0, pair) == 0);
    memset(&d, 0, sizeof(d)); memset(&ch, 0, sizeof(ch));
    memset(&pc, 0, sizeof(pc)); memset(&enemy, 0, sizeof(enemy));
    d.descriptor = pair[0]; peer = pair[1];
    assert(fcntl(pair[0], F_SETFL, O_NONBLOCK) == 0);
    d.outsize = 2000; d.outbuf = alloc_mem(d.outsize);
    d.outbuf[0] = '\0';
    d.connected = CON_PLAYING; d.character = &ch;
    ch.desc = &d; ch.pcdata = &pc; ch.name = "Self\"\\\n";
    ch.level = LEV_IMP;
    ch.hit = 71; ch.max_hit = 100; ch.mana = 33; ch.max_mana = 99;
    ch.fighting = &enemy; enemy.hit = 987654321;
    enemy.name = "private-opponent";
    ch.blood[0] = 17; ch.blood[1] = 50;
    ch.quint[BODY] = 42; ch.paradox = 8;
    pc.classpoints[CP_CURRENT] = 77;
    pc.ninja[NINJA_KI] = 222;
    pc.willpower[0] = 7; pc.willpower[1] = 9;
}

static void finish(void)
{
    close((int)d.descriptor); close(peer);
    free_mem(d.outbuf, d.outsize);
}

static void clear_output(void)
{
    d.outtop = 0; d.outbuf[0] = '\0';
}

static void bytes(const unsigned char *p, size_t count)
{
    if (count) assert(write(peer, p, count) == (ssize_t)count);
    assert(read_from_descriptor(&d));
}

static void ack(bool enable)
{
    const unsigned char p[] = {255, enable ? 253 : 254, 201};
    for (size_t i = 0; i < sizeof(p); ++i) bytes(p + i, 1);
}

static void negotiation(void)
{
    begin();
    assert(gmcp_update(&d) && d.outtop == 0);
    assert(gmcp_offer(&d) && d.gmcp_state == GMCP_OFFERED);
    assert(d.outtop == 3 && !memcmp(d.outbuf, "\xff\xfb\xc9", 3));
    assert(gmcp_offer(&d) && d.outtop == 3);
    clear_output(); ack(TRUE);
    assert(d.gmcp_state == GMCP_ACTIVE && gmcp_update(&d));
    long size = d.outtop;
    assert(size > 0);
    ack(TRUE); assert(gmcp_update(&d) && d.outtop == size);
    clear_output(); ack(FALSE);
    assert(gmcp_update(&d) && gmcp_offer(&d) && d.outtop == 0);
    ack(FALSE); assert(d.outtop == 0);
    ack(TRUE); assert(gmcp_update(&d) && d.outtop > 0);
    assert(d.inbuf[0] == '\0');
    finish();
}

static void classes(void)
{
    static const long ids[] = {CLASS_VAMPIRE,CLASS_WEREWOLF,CLASS_SORCERER,
        CLASS_HIGHLANDER,CLASS_DROW,CLASS_NINJA,CLASS_DRAGON,CLASS_MAGE,
        CLASS_MONK,CLASS_ELEM,CLASS_NONE,-1};
    static const char *names[] = {"vampire","werewolf","sorcerer","highlander",
        "drow","ninja","dragon","mage","monk","elemental","none","unknown"};
    begin(); ack(TRUE);
    for (size_t i = 0; i < sizeof(ids)/sizeof(*ids); ++i) {
        char expected[80];
        ch.class = ids[i]; clear_output();
        assert(gmcp_update(&d) && d.outtop > 0);
        snprintf(expected, sizeof(expected), "\"class\":\"%s\"", names[i]);
        assert(strstr(d.outbuf, expected));
        assert(strstr(d.outbuf, "Char.Vitals {\"hp\":71,\"maxhp\":100,\"mp\":33,\"maxmp\":99,\"mana\":33,\"maxmana\":99}"));
        assert(strstr(d.outbuf, "\"available\":true"));
        assert(!strstr(d.outbuf, ch.name) && !strstr(d.outbuf, enemy.name));
        assert(strstr(d.outbuf, "Char.Base {\"name\":\"Self\\\"\\\\\\u000a\","));
        assert(!strstr(d.outbuf, "987654321"));
        assert(strstr(d.outbuf, ids[i]==CLASS_VAMPIRE ?
            "\"blood\":{\"current\":17,\"max\":50}" : "\"blood\":null"));
        assert(strstr(d.outbuf, ids[i]==CLASS_NINJA ?
            "\"ki\":{\"current\":222,\"max\":300}" : "\"ki\":null"));
        assert(strstr(d.outbuf, ids[i]==CLASS_MAGE ?
            "\"quintessence\":{\"current\":42,\"max\":null}" : "\"quintessence\":null"));
        assert(strstr(d.outbuf, ids[i]==CLASS_MAGE ?
            "\"paradox\":{\"current\":8,\"max\":null}" : "\"paradox\":null"));
        assert(strstr(d.outbuf, ids[i]==CLASS_DROW || ids[i]==CLASS_DRAGON ?
            "\"class_points\":{\"current\":77,\"max\":null}" : "\"class_points\":null"));
        assert(strstr(d.outbuf, ids[i]==CLASS_WEREWOLF ?
            "\"willpower\":{\"current\":7,\"max\":9}" : "\"willpower\":null"));
        clear_output(); assert(gmcp_update(&d) && d.outtop == 0);
    }
    finish();
}

static void privacy(void)
{
    begin(); ack(TRUE);
    d.connected = CON_GET_OLD_PASSWORD;
    assert(gmcp_update(&d) && d.outtop == 0);
    d.connected = CON_PLAYING;
    assert(gmcp_update(&d) && d.outtop > 0); clear_output();
    d.original = &enemy;
    assert(gmcp_update(&d) && strstr(d.outbuf, "\"available\":false"));
    assert(strstr(d.outbuf, "\"hp\":0,\"maxhp\":0"));
    assert(strstr(d.outbuf, "\"mp\":0,\"maxmp\":0,\"mana\":0,\"maxmana\":0"));
    assert(strstr(d.outbuf, "Char.Base {\"name\":\"\",\"level\":0,\"class\":\"none\"}"));
    clear_output(); assert(gmcp_update(&d) && d.outtop == 0);
    d.original = NULL; SET_BIT(ch.act, ACT_IS_NPC);
    assert(gmcp_update(&d) && d.outtop == 0);
    REMOVE_BIT(ch.act, ACT_IS_NPC); ch.desc = NULL;
    assert(gmcp_update(&d) && d.outtop == 0);
    ch.desc = &d; ch.pcdata = NULL;
    assert(gmcp_update(&d) && d.outtop == 0);
    ch.pcdata = &pc; SET_BIT(ch.more, MORE_HEAD);
    assert(gmcp_update(&d) && d.outtop == 0);
    REMOVE_BIT(ch.more, MORE_HEAD);
    SET_BIT(ch.extra, EXTRA_OSWITCH);
    assert(gmcp_update(&d) && d.outtop == 0);
    REMOVE_BIT(ch.extra, EXTRA_OSWITCH);
    const long editors[] = {CON_NOTE_TO,CON_NOTE_FINISH,CON_AEDITOR,
                           CON_REDITOR,CON_OEDITOR,CON_MEDITOR};
    for (size_t i = 0; i < sizeof(editors)/sizeof(*editors); ++i) {
        d.connected = editors[i]; ++ch.hit;
        assert(gmcp_update(&d) && strstr(d.outbuf, "\"available\":true"));
        clear_output();
    }
    finish();
}

static void updates_and_wire(void)
{
    begin(); ack(TRUE);
    assert(gmcp_update(&d)); clear_output();
    --ch.hit; --ch.mana;
    assert(gmcp_update(&d) && d.outtop > 0);
    assert(strstr(d.outbuf, "\"hp\":70") && strstr(d.outbuf, "\"mp\":32"));
    unsigned char expected[GMCP_CACHE_SIZE], actual[GMCP_CACHE_SIZE];
    size_t size = (size_t)d.outtop;
    memcpy(expected, d.outbuf, size);
    assert(process_output(&d, FALSE) && d.outtop == 0);
    assert(recv(peer, actual, size, MSG_WAITALL) == (ssize_t)size);
    assert(!memcmp(expected, actual, size));
    assert(gmcp_update(&d) && d.outtop == 0);
    ch.hit = LONG_MIN; ch.max_hit = LONG_MAX;
    ch.mana = LONG_MAX; ch.max_mana = LONG_MIN;
    ch.class = CLASS_VAMPIRE; ch.blood[0] = LONG_MIN; ch.blood[1] = LONG_MAX;
    assert(gmcp_update(&d) && d.outtop < GMCP_CACHE_SIZE);
    assert(d.outtop == (long)strlen(d.outbuf));
    finish();
}

/* MudForge's documented status widget uses mana/maxmana. Existing Mudlet
 * consumers keep mp/maxmp; both pairs must be exact aliases, not new stats. */
static void mudforge_fields(void)
{
    const long values[]={LONG_MIN,-123,-1,0,1,123,LONG_MAX};
    const long ids[]={CLASS_VAMPIRE,CLASS_WEREWOLF,CLASS_SORCERER,
        CLASS_HIGHLANDER,CLASS_DROW,CLASS_NINJA,CLASS_DRAGON,CLASS_MAGE,
        CLASS_MONK,CLASS_ELEM,CLASS_NONE,-1};
    unsigned cases=0;
    begin();ack(TRUE);
    for(unsigned cls=0;cls<12;cls++)for(unsigned a=0;a<7;a++)
    for(unsigned b=0;b<7;b++) {
        char expected[512];
        ch.class=ids[cls];
        ch.hit=values[a];ch.max_hit=values[b];
        ch.mana=values[6-a];ch.max_mana=values[6-b];
        clear_output();
        assert(gmcp_update(&d));
        int n=snprintf(expected,sizeof expected,
            "Char.Vitals {\"hp\":%ld,\"maxhp\":%ld,\"mp\":%ld,\"maxmp\":%ld,"
            "\"mana\":%ld,\"maxmana\":%ld}",
            ch.hit,ch.max_hit,ch.mana,ch.max_mana,ch.mana,ch.max_mana);
        assert(n>0&&(size_t)n<sizeof expected&&strstr(d.outbuf,expected));
        assert(!strstr(d.outbuf,"\"movement\"")&&!strstr(d.outbuf,"\"maxmove\""));
        clear_output();assert(gmcp_update(&d)&&d.outtop==0);
        ++cases;
    }
    assert(cases==588);
    finish();
}

static void payloads_and_recovery(void)
{
    static const unsigned char packet[] = "\xff\xfa\xc9" "Core.Hello "
        "{\"client\":\"fake\\nshutdown\"}\xff\xff\0\n\xff\xf0look\n";
    begin(); ack(TRUE);
    for (size_t i=0; i<sizeof(packet)-1; ++i) bytes(packet+i,1);
    read_from_buffer(&d); assert(!strcmp(d.incomm, "look"));
    d.incomm[0] = '\0'; /* The interpreter clears each completed command. */
    assert(gmcp_update(&d)); clear_output();
    assert(gmcp_recover(&d) && d.gmcp_state == GMCP_OFFERED);
    assert(d.outtop==6 && !memcmp(d.outbuf,"\xff\xfc\xc9\xff\xfb\xc9",6));
    clear_output(); assert(gmcp_update(&d) && d.outtop==0);
    ack(FALSE); ack(TRUE);
    assert(gmcp_update(&d) && d.outtop>0);
    finish();
}

static void limits(void)
{
    begin(); ack(TRUE);
    free_mem(d.outbuf,d.outsize); d.outsize=128000; d.outbuf=alloc_mem(d.outsize);
    d.outtop=127998; d.outbuf[d.outtop]='\0';
    assert(!gmcp_update(&d) && d.outtop==127998 && d.gmcp_cache[0]=='\0');
    assert(!telnet_write(&d,NULL,1));
    assert(!telnet_write(&d,(const unsigned char *)"x",(size_t)-1));
    clear_output(); d.gmcp_state=255;
    assert(!gmcp_update(&d) && !gmcp_offer(&d));
    assert(!gmcp_update(NULL) && !gmcp_offer(NULL) && !gmcp_recover(NULL));
    finish();
}

static void snoop_privacy(void)
{
    DESCRIPTOR_DATA watcher;
    CHAR_DATA staff;
    unsigned char wire[GMCP_CACHE_SIZE+32];
    begin(); ack(TRUE); assert(gmcp_update(&d));
    size_t size=(size_t)d.outtop;
    memcpy(wire,d.outbuf,size); memcpy(wire+size,"visible text",12); size+=12;
    memset(&staff,0,sizeof(staff)); staff.level=LEV_IMP;
    for (size_t cut=0;cut<=size;++cut) {
        memset(&watcher,0,sizeof(watcher));
        watcher.character=&staff; watcher.outsize=2000;
        watcher.outbuf=alloc_mem(watcher.outsize);
        watcher.outbuf[0]='\0';
        d.telnet_output_state=TELNET_DATA; d.snoop_text_started=FALSE;
        d.snoop_by=NULL;
        telnet_snoop_output(&d,wire,cut); /* Attach even in the middle of SB. */
        d.snoop_by=&watcher;
        telnet_snoop_output(&d,wire+cut,size-cut);
        assert(!strstr(watcher.outbuf,"Char.Vitals"));
        assert(!strstr(watcher.outbuf,"Char.Base"));
        assert(!strstr(watcher.outbuf,"Self"));
        assert(!strstr(watcher.outbuf,"Anathema.Vitals"));
        assert(!strstr(watcher.outbuf,"\"hp\""));
        assert(!memchr(watcher.outbuf,255,(size_t)watcher.outtop));
        if (cut <= size-12) assert(strstr(watcher.outbuf,"visible text"));
        free_mem(watcher.outbuf,watcher.outsize);
    }
    memset(&watcher,0,sizeof(watcher)); watcher.character=&staff;
    watcher.outsize=2000; watcher.outbuf=alloc_mem(watcher.outsize);
    watcher.outbuf[0]='\0';
    d.snoop_by=&watcher; d.telnet_output_state=TELNET_DATA;
    d.snoop_text_started=FALSE;
    assert(telnet_write(&d,(const unsigned char *)"visible text",12));
    assert(process_output(&d,FALSE));
    assert(strstr(watcher.outbuf,"visible text") && !strstr(watcher.outbuf,"Vitals"));
    assert(!d.snoop_text_started);
    free_mem(watcher.outbuf,watcher.outsize);
    d.snoop_by=NULL;
    finish();
}

static void identity(void)
{
    char byte[2] = {0,0}, expected[128], long_name[65];
    begin(); ack(TRUE);
    for (unsigned c = 1; c <= 255; ++c) {
        byte[0] = (char)c; ch.name = byte; clear_output();
        assert(gmcp_update(&d));
        if (c < 32 || c >= 127)
            snprintf(expected,sizeof expected,"Char.Base {\"name\":\"\\u%04x\",",c);
        else if (c == '"' || c == '\\')
            snprintf(expected,sizeof expected,"Char.Base {\"name\":\"\\%c\",",c);
        else
            snprintf(expected,sizeof expected,"Char.Base {\"name\":\"%c\",",c);
        assert(strstr(d.outbuf,expected));
        assert(d.outtop < GMCP_CACHE_SIZE && d.outtop == (long)strlen(d.outbuf));
    }
    /* Deliberately non-terminated input: the exporter must not read byte 65. */
    char bounded[64];
    memset(bounded,255,sizeof bounded); ch.name = bounded;
    ch.level = LONG_MIN; clear_output();
    assert(gmcp_update(&d) && strstr(d.outbuf,"\"level\":-"));
    assert(strstr(d.outbuf,"\\u00ff\\u00ff"));
    memset(long_name,'A',64); long_name[64]='\0';
    ch.name = long_name; clear_output(); assert(gmcp_update(&d));
    assert(strstr(d.outbuf,long_name));
    ch.name = NULL; ch.level = LONG_MAX; clear_output();
    assert(gmcp_update(&d) && strstr(d.outbuf,"Char.Base {\"name\":\"\","));
    clear_output(); assert(gmcp_update(&d) && d.outtop == 0);
    ch.name = "Luto"; clear_output();
    assert(gmcp_update(&d) && strstr(d.outbuf,"\"name\":\"Luto\""));
    clear_output(); --ch.level;
    assert(gmcp_update(&d) && d.outtop > 0);
    clear_output(); d.connected = CON_GET_OLD_PASSWORD;
    assert(gmcp_update(&d) && d.outtop == 0 && d.gmcp_cache[0] == '\0');
    d.connected = CON_PLAYING;
    assert(gmcp_update(&d) && strstr(d.outbuf,"\"name\":\"Luto\""));
    finish();
}

/* Independent Python JSON decoding can exercise the actual engine output. */
static void identity_wire(void)
{
    char byte[2] = {0,0};
    begin(); ack(TRUE);
    for (unsigned c = 1; c <= 255; ++c) {
        byte[0] = (char)c; ch.name = byte; clear_output();
        assert(gmcp_update(&d));
        assert(fwrite(d.outbuf,1,(size_t)d.outtop,stdout) == (size_t)d.outtop);
    }
    finish();
}

int main(int argc, char **argv)
{
    if (argc == 2 && !strcmp(argv[1],"--identity-wire")) {
        identity_wire(); return 0;
    }
    identity();
    mudforge_fields();
    negotiation(); classes(); privacy(); updates_and_wire();
    payloads_and_recovery(); limits(); snoop_privacy();
    puts("GMCP identity/255 name bytes, negotiation, ten-class self-only snapshots, 588 MudForge alias cases, snoop isolation, cache, wire, recovery and bounds passed");
    return 0;
}
