/* Real-engine room/exit output, without network or live world state. */
#define main identify_fixture_main
#include "test_identify.c"
#undef main
static ROOM_INDEX_DATA destinations[6];
static EXIT_DATA doors[6];
static void room_fixture(void)
{
    fixture();memset(destinations,0,sizeof destinations);memset(doors,0,sizeof doors);
    room.name="A quiet chamber";room.description="Stone walls surround you.\n\r";
    room.sector_type=SECT_INSIDE;
    for(int n=0;n<6;n++){
        destinations[n].vnum=910+n;destinations[n].name="A distant passage";
        destinations[n].description="A distant passage stretches onward.\n\r";
        destinations[n].area=&area;destinations[n].sector_type=SECT_INSIDE;
        destinations[n].light=1;doors[n].to_room=destinations+n;
        doors[n].keyword="wooden gate";doors[n].exit_info=EX_ISDOOR;
        room.exit[n]=doors+n;
    }
    obj.name="flask";obj.short_descr="a modest flask";
    obj.description="A modest flask lies here.\n\r";
    obj.item_type=ITEM_DRINK_CON;obj.value[0]=100;obj.value[1]=55;obj.value[2]=0;
    obj.in_room=&room;obj.next_content=NULL;
}
static void room_native(int dump)
{
    static const char *commands[]={"","auto","north","","auto","in flask","in flask","down"};
    uint64_t total=UINT64_C(1469598103934665603);unsigned rows=0;
    for(int mode=0;mode<8;mode++)for(int view=0;view<7;view++)
    for(int env=0;env<4;env++)for(int staff=0;staff<2;staff++)
    for(int automatic=0;automatic<2;automatic++)for(int ansi=0;ansi<2;ansi++){
        room_fixture();if(staff)actor.level=LEV_BUI;
        if(ansi)SET_BIT(actor.act,PLR_ANSI);
        if(automatic)SET_BIT(actor.act,PLR_AUTOEXIT);
        if(view==1)SET_BIT(actor.affected_by,AFF_SHADOWPLANE);
        if(view==2||view==3)SET_BIT(actor.affected_by,AFF_SHADOWPLANE|AFF_SHADOWSIGHT);
        if(view==3)SET_BIT(actor.affected_by,AFF_DEEPUMBRA);
        if(view==4)SET_BIT(actor.affected_by,AFF_BLIND);
        if(view==5)SET_BIT(actor.extra,BLINDFOLDED);
        if(view==6)SET_BIT(actor.act,PLR_BRIEF);
        if(env==1){world_affects=WORLD_FOG;doors[0].exit_info|=EX_CLOSED;
            room.exit[5]=NULL;}
        if(env==2){destinations[0].light=0;destinations[0].room_flags|=ROOM_DARK;}
        if(env==3){room.light=0;room.room_flags|=ROOM_DARK;}
        if(mode==5||mode==6){room.contents=&obj;if(mode==6){obj.item_type=ITEM_CONTAINER;obj.value[1]=CONT_CLOSED;}}
        char command[40];strcpy(command,commands[mode]);
        if(mode==3||mode==4)do_exits(&actor,command);else do_look(&actor,command);
        assert(actor.in_room==&room);
        uint64_t row=hash(UINT64_C(1469598103934665603),output,desc.outtop);
        total=hash(total,&row,sizeof row);rows++;
        if(dump)printf("%d,%d,%d,%d,%d,%d,%" PRIu64 "\n",mode,view,env,staff,automatic,ansi,row);
    }
    assert(rows==1792);fprintf(stderr,"ROOM_NATIVE_HASH=%" PRIu64 "\n",total);
    if(!dump)assert(total==UINT64_C(10727128317469626511));
}
static void room_edge(int n)
{
    room_fixture();memset(large,'x',sizeof large-1);large[sizeof large-1]=0;
    char empty[]="",north[]="north",flask[]="flask",inside[]="in flask";
    if(n==0){room.name=large;do_look(&actor,empty);}
    if(n==1){for(int k=0;k<6;k++)destinations[k].name=large;do_exits(&actor,empty);}
    if(n==2){actor.in_room=NULL;do_exits(&actor,empty);}
    if(n==3)do_look(NULL,empty);
    if(n==4)do_exits(NULL,empty);
    if(n==5){room.contents=&obj;obj.pIndexData=NULL;do_look(&actor,flask);}
    if(n==6){room.contents=&obj;obj.value[2]=LONG_MAX;do_look(&actor,inside);}
    if(n==7){room.contents=&obj;obj.value[2]=LONG_MIN;do_look(&actor,inside);}
    if(n==8){room.name=NULL;do_look(&actor,empty);}
    if(n==9){destinations[0].name=NULL;do_exits(&actor,empty);}
    if(n==10){actor.pcdata=NULL;do_look(&actor,empty);}
    if(n==11){actor.pcdata=NULL;do_exits(&actor,empty);}
    if(n==12)do_look(&actor,NULL);
    if(n==13)do_exits(&actor,NULL);
    if(n==14){destinations[0].name=large;do_look(&actor,north);}
    assert(desc.outtop<MSL*2);
    if(n!=2)assert(actor.in_room==&room);
    if(n==5)assert(strstr(output,"A modest flask lies here."));
    if(n==6||n==7)assert(strstr(output,"colour cannot be determined"));
    if(n==8||n==9)assert(strstr(output,"Somewhere"));
}
static void navigation_and_colours(void)
{
    for(int ansi=0;ansi<2;ansi++)for(int hashes=1;hashes<4;hashes++)
    for(int list=0;list<2;list++){
        room_fixture();if(ansi)SET_BIT(actor.act,PLR_ANSI);
        memset(large,'x',sizeof large-1);large[sizeof large-1]=0;
        char prefix[100];
        snprintf(prefix,sizeof prefix,"#yObvious exits#r:#n\n\r#y%-5s #r-#n ","North");
        size_t used=list?strlen(prefix):2;
        memset(large+MSL-3-used-hashes,'#',(size_t)hashes);
        char empty[]="";
        if(list){for(int n=0;n<6;n++)destinations[n].name=large;do_exits(&actor,empty);}
        else{room.name=large;do_look(&actor,empty);}
        assert(desc.outtop>0&&desc.outtop<MSL*2&&strstr(output,"\n\r"));
        if(!list)assert(strstr(output,"Stone walls surround you."));
        else assert(output[desc.outtop-9]=='\n'&&output[desc.outtop-8]=='\r');
    }
    for(int viewer=0;viewer<2;viewer++)for(int portal_shadow=0;portal_shadow<2;portal_shadow++){
        room_fixture();OBJ_DATA return_portal=obj;
        obj.item_type=return_portal.item_type=ITEM_PORTAL;
        obj.value[0]=destinations[0].vnum;obj.value[3]=room.vnum;obj.value[2]=0;
        return_portal.value[0]=room.vnum;return_portal.value[3]=destinations[0].vnum;
        return_portal.next_content=NULL;destinations[0].contents=&return_portal;
        room.contents=&obj;obj.name="portal";
        if(viewer)SET_BIT(actor.affected_by,AFF_SHADOWPLANE|AFF_SHADOWSIGHT);
        if(portal_shadow)SET_BIT(return_portal.extra_flags,ITEM_SHADOWPLANE);
        long before=actor.affected_by;
        long bucket=destinations[0].vnum%MAX_KEY_HASH;
        assert(room_index_hash[bucket]==NULL);room_index_hash[bucket]=destinations;
        char command[]="in portal";do_look(&actor,command);
        room_index_hash[bucket]=NULL;
        assert(actor.in_room==&room&&actor.affected_by==before);
        assert(strstr(output,viewer||!portal_shadow?"A distant passage":"The Umbra"));
    }
}
int main(int argc,char **argv)
{
    if(argc>1&&!strcmp(argv[1],"--native-dump")){room_native(1);return 0;}
    if(argc>2&&!strcmp(argv[1],"--boundary")){room_edge(atoi(argv[2]));return 0;}
    room_native(0);for(int n=0;n<15;n++)room_edge(n);
    navigation_and_colours();
    puts("Room display:1792 native snapshots,15 boundaries and portal/colour restoration passed.");
    return 0;
}
