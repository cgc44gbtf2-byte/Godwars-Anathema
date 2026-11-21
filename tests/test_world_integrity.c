#define _POSIX_C_SOURCE 200809L
/* Boot the actual versioned world in private scratch space, not live state. */
#include <assert.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include "merc.h"

static int errors, dangling, missing_keys, asymmetric, dump;
extern void load_rooms(FILE *);
extern void reset_area(AREA_DATA *);
static void door_roundtrip(void)
{
    static AREA_DATA fixture;
    AREA_DATA *saved=area_last;
    FILE *fp=tmpfile();assert(fp);
    for(int locks=0;locks<=8;locks++)
        fprintf(fp,"#%d\nFixture~\nFixture~\n0 0 0\nD0\n~\ndoor~\n%d -1 0\nS\n",900000+locks,locks);
    fputs("#0\n",fp);rewind(fp);area_last=&fixture;load_rooms(fp);area_last=saved;fclose(fp);
    for(int locks=0;locks<=8;locks++) {
        long flags=0,bits=locks-1;
        if(locks)flags=EX_ISDOOR | ((bits&1)?EX_PICKPROOF:0) |
                      ((bits&2)?EX_BASHPROOF:0) | ((bits&4)?EX_PASSPROOF:0);
        EXIT_DATA *exit=get_room_index(900000+locks)->exit[0];
        assert(exit->exit_info==flags && exit->rs_flags==flags);
    }
    reset_area(&fixture);
    for(int locks=0;locks<=8;locks++) {
        EXIT_DATA *exit=get_room_index(900000+locks)->exit[0];
        assert(exit->exit_info==exit->rs_flags);
    }
    /* A maze's nominal opposite direction may lead to a third room. */
    static AREA_DATA other;
    static EXIT_DATA reverse;
    ROOM_INDEX_DATA *a=get_room_index(900000), *b=get_room_index(900001);
    ROOM_INDEX_DATA *c=get_room_index(900002);
    b->area=c->area=&other;
    a->exit[0]->to_room=b;
    b->exit[rev_dir[0]]=&reverse;
    reverse.to_room=c;
    reverse.rs_flags=EX_ISDOOR;
    reverse.exit_info=EX_ISDOOR|EX_CLOSED;
    reset_area(&fixture);
    assert(IS_SET(reverse.exit_info,EX_CLOSED));
    for(int bad=-1;bad<=9;bad+=10) {
        fp=tmpfile();assert(fp);
        fprintf(fp,"#900100\nFixture~\nFixture~\n0 0 0\nD0\n~\ndoor~\n%d -1 0\nS\n#0\n",bad);
        rewind(fp);
        pid_t child=fork();assert(child>=0);
        if(!child){area_last=&fixture;load_rooms(fp);_exit(99);}
        int status;assert(waitpid(child,&status,0)==child);
        assert(WIFEXITED(status) && WEXITSTATUS(status)==1);
        fclose(fp);
    }
}
static void copy_file(const char *from, const char *to)
{
    FILE *in=fopen(from,"rb"), *out=fopen(to,"wb");
    assert(in && out);
    char data[8192]; size_t n;
    while ((n=fread(data,1,sizeof(data),in))!=0) assert(fwrite(data,1,n,out)==n);
    assert(!ferror(in) && fclose(in)==0 && fclose(out)==0);
}
static void need(char kind, long vnum)
{
    bool found=kind=='O' ? get_obj_index(vnum)!=NULL :
               kind=='M' ? get_mob_index(vnum)!=NULL : get_room_index(vnum)!=NULL;
    if (!found) { fprintf(stderr,"WORLD ERROR missing %c %ld\n",kind,vnum); errors++; }
}
static void spell(long object, long sn)
{
    if (sn<0) return; /* -1 is the engine's empty slot. */
    if (sn>=MAX_SKILL || !skill_table[sn].name || !skill_table[sn].spell_fun ||
        skill_table[sn].spell_fun==spell_null) {
        fprintf(stderr,"WORLD ERROR invalid item spell: object=%ld skill=%ld\n",object,sn);
        errors++;
    }
}
int main(int argc, char **argv)
{
    char cwd[4096], source[8192], target[16384], name[256];
    char scratch[]="/tmp/anathema-world-XXXXXX";
    const char *dirs[]={"area","player","backup","notes","log","clans"};
    dump=argc==2 && strcmp(argv[1],"--dump")==0;
    assert(getcwd(cwd,sizeof(cwd)) && mkdtemp(scratch));
    for (size_t i=0;i<6;i++) { snprintf(target,sizeof(target),"%s/%s",scratch,dirs[i]);assert(mkdir(target,0700)==0); }
    snprintf(source,sizeof(source),"%s/Genesis/area/area.lst",cwd);
    snprintf(target,sizeof(target),"%s/area/area.lst",scratch);copy_file(source,target);
    FILE *list=fopen(source,"r");assert(list);
    while (fscanf(list,"%255s",name)==1 && strcmp(name,"$")) {
        assert(!strchr(name,'/') && !strchr(name,'\\') && strcmp(name,".."));
        snprintf(source,sizeof(source),"%s/Genesis/area/%s",cwd,name);
        snprintf(target,sizeof(target),"%s/area/%s",scratch,name);copy_file(source,target);
    }
    assert(fclose(list)==0);
    snprintf(target,sizeof(target),"%s/area",scratch);assert(chdir(target)==0);
    current_time=time(NULL); fpReserve=fopen(NULL_FILE,"r");assert(fpReserve);
    boot_db();
    long objects[]={OBJ_VNUM_MONEY_ONE,OBJ_VNUM_MONEY_SOME,OBJ_VNUM_CORPSE_NPC,
      OBJ_VNUM_CORPSE_PC,OBJ_VNUM_SEVERED_HEAD,OBJ_VNUM_MUSHROOM,OBJ_VNUM_LIGHT_BALL,
      OBJ_VNUM_SPRING,OBJ_VNUM_DROW_CROSSBOW,OBJ_VNUM_SHADOW_WEAPON,OBJ_VNUM_DROW_BOLT,
      OBJ_VNUM_SOULBLADE,OBJ_VNUM_PORTAL,OBJ_VNUM_SPILT_BLOOD,OBJ_VNUM_VOODOO_DOLL,
      OBJ_VNUM_PROTOPLASM,OBJ_VNUM_QUESTCARD,OBJ_VNUM_QUESTMACHINE,
      29600,29601,29602,29603,29610,29611,29612,29613,29614,29620,29621,
      29630,29631,29632,29639,29640,29641,29642,29650,29652,29653,29654,
      29655,29658,29659,29660,29661,29662};
    long mobs[]={MOB_VNUM_GUARDIAN,MOB_VNUM_MOUNT,MOB_VNUM_FROG,MOB_VNUM_RAVEN,
      MOB_VNUM_CAT,MOB_VNUM_DOG,MOB_VNUM_EYE,MOB_VNUM_MOLERAT,MOB_VNUM_YAK,MOB_VNUM_ASTRAL,MOB_VNUM_CLONE};
    long rooms[]={ROOM_VNUM_LIMBO,ROOM_VNUM_CHAT,ROOM_VNUM_TEMPLE,ROOM_VNUM_FREEZE,
      ROOM_VNUM_ALTAR,ROOM_VNUM_SCHOOL,ROOM_VNUM_CRYPT,
      ROOM_VNUM_DISCONNECTION,ROOM_VNUM_IN_OBJECT};
    for(size_t i=0;i<sizeof(objects)/sizeof(*objects);i++)need('O',objects[i]);
    for(size_t i=0;i<sizeof(mobs)/sizeof(*mobs);i++)need('M',mobs[i]);
    for(size_t i=0;i<sizeof(rooms)/sizeof(*rooms);i++)need('R',rooms[i]);
    long nc=0,oc=0,rc=0,ec=0,ac=0,resets=0,spell_slots=0;
    for(int h=0;h<MAX_KEY_HASH;h++) {
      for(MOB_INDEX_DATA *m=mob_index_hash[h];m;m=m->next) {
        nc++; if(dump)printf("{\"kind\":\"mob\",\"vnum\":%ld,\"level\":%ld,\"special\":%d}\n",m->vnum,m->level,m->spec_fun!=NULL);
      }
      for(OBJ_INDEX_DATA *o=obj_index_hash[h];o;o=o->next) {
        oc++;
        if(dump)printf("{\"kind\":\"object\",\"vnum\":%ld,\"type\":%ld,\"values\":[%ld,%ld,%ld,%ld],\"wear\":%ld}\n",o->vnum,o->item_type,o->value[0],o->value[1],o->value[2],o->value[3],o->wear_flags);
        if(o->item_type==ITEM_POTION||o->item_type==ITEM_PILL||o->item_type==ITEM_SCROLL)
          for(int s=1;s<4;s++){spell(o->vnum,o->value[s]);if(o->value[s]>=0)spell_slots++;}
        if(o->item_type==ITEM_WAND||o->item_type==ITEM_STAFF){spell(o->vnum,o->value[3]);if(o->value[3]>=0)spell_slots++;}
        if(o->item_type==ITEM_MISSILE) {
          OBJ_INDEX_DATA *ammo=get_obj_index(o->value[0]);
          if(!ammo||ammo->item_type!=ITEM_AMMO||ammo->value[3]!=o->value[3]) {
            fprintf(stderr,"WORLD ERROR missile/ammunition mismatch: object=%ld ammo=%ld\n",o->vnum,o->value[0]);errors++;
          }
        }
      }
      for(ROOM_INDEX_DATA *r=room_index_hash[h];r;r=r->next) {
        rc++; if(dump)printf("{\"kind\":\"room\",\"vnum\":%ld}\n",r->vnum);
        for(ROOMTEXT_DATA *t=r->roomtext;t;t=t->next) {
          if(dump)printf("{\"kind\":\"trigger\",\"room\":%ld,\"type\":%ld,\"power\":%ld,\"mob\":%ld}\n",r->vnum,t->type,t->power,t->mob);
          if(t->mob)need('M',t->mob);
          switch(t->type%RT_RETURN) {
            case RT_OBJECT:need('O',t->power);break;
            case RT_MOBILE:need('M',t->power);break;
            case RT_TELEPORT:case RT_PORTAL:case RT_MOVE_LIFT:need('R',t->power);break;
            case RT_SPELL:spell(-r->vnum,t->power);break;
          }
        }
        if(r->trap) {
          TRAP_DATA *t=r->trap;
          if(t->spellsn>0)spell(-r->vnum,t->spellsn);
          if(t->movevnum>0)need('R',t->movevnum);
          if(t->summonvnum>0)need('M',t->summonvnum);
          if(dump)printf("{\"kind\":\"trap\",\"room\":%ld,\"spell\":%ld,\"move\":%ld,\"summon\":%ld}\n",r->vnum,t->spellsn,t->movevnum,t->summonvnum);
        }
        for(int d=0;d<6;d++) if(r->exit[d]) {
          EXIT_DATA *e=r->exit[d];ec++;
          if(e->vnum>0&&!e->to_room){dangling++;fprintf(stderr,"WORLD REVIEW dangling exit room=%ld dir=%d target=%ld\n",r->vnum,d,e->vnum);}
          if(e->key>0&&!get_obj_index(e->key)){missing_keys++;fprintf(stderr,"WORLD REVIEW missing exit key room=%ld key=%ld\n",r->vnum,e->key);}
          if(e->to_room && e->to_room->exit[rev_dir[d]] &&
             e->to_room->exit[rev_dir[d]]->to_room!=r)asymmetric++;
          if(dump)printf("{\"kind\":\"exit\",\"room\":%ld,\"dir\":%d,\"target\":%ld,\"key\":%ld,\"flags\":%ld}\n",r->vnum,d,e->vnum,e->key,e->rs_flags);
        }
      }
    }
    for(AREA_DATA *a=area_first;a;a=a->next) {ac++;for(RESET_DATA *r=a->reset_first;r;r=r->next) {
      resets++;
      if(r->command=='P') {
        OBJ_INDEX_DATA *container=get_obj_index(r->arg3);
        if(!container || container->item_type!=ITEM_CONTAINER){fprintf(stderr,"WORLD ERROR put-reset non-container: %ld\n",r->arg3);errors++;}
      }
      if(r->command=='E' && (r->arg3<0 || r->arg3>=MAX_WEAR)){fprintf(stderr,"WORLD ERROR invalid equip slot: %ld\n",r->arg3);errors++;}
      if(dump)printf("{\"kind\":\"reset\",\"command\":\"%c\",\"args\":[%ld,%ld,%ld]}\n",r->command,r->arg1,r->arg2,r->arg3);
    }}
    for(SHOP_DATA *s=shop_first;s;s=s->next) {
      if(s->open_hour<0 || s->close_hour>23 || s->open_hour>s->close_hour ||
         s->profit_sell<0 || s->profit_buy<s->profit_sell) {
        fprintf(stderr,"WORLD ERROR shop hours/margins: keeper=%ld\n",s->keeper);errors++;
      }
      if(dump)printf("{\"kind\":\"shop\",\"keeper\":%ld,\"buy\":%ld,\"sell\":%ld,\"open\":%ld,\"close\":%ld}\n",s->keeper,s->profit_buy,s->profit_sell,s->open_hour,s->close_hour);
    }
    printf("{\"kind\":\"summary\",\"areas\":%ld,\"rooms\":%ld,\"objects\":%ld,\"mobs\":%ld,\"exits\":%ld,\"resets\":%ld,\"item_spell_slots\":%ld,\"errors\":%d,\"dangling_exits\":%d,\"missing_keys\":%d,\"asymmetric_links\":%d}\n",ac,rc,oc,nc,ec,resets,spell_slots,errors,dangling,missing_keys,asymmetric);
    fflush(stdout);
    door_roundtrip();
    assert(fclose(fpReserve)==0 && chdir(cwd)==0);
    for(size_t i=0;i<6;i++) {
      snprintf(source,sizeof(source),"%s/%s",scratch,dirs[i]);DIR *directory=opendir(source);assert(directory);
      struct dirent *entry;
      while((entry=readdir(directory)))if(strcmp(entry->d_name,".")&&strcmp(entry->d_name,"..")){
        snprintf(target,sizeof(target),"%s/%s",source,entry->d_name);assert(unlink(target)==0);
      }
      assert(closedir(directory)==0 && rmdir(source)==0);
    }
    assert(rmdir(scratch)==0);
    return errors || dangling || missing_keys ? EXIT_FAILURE : EXIT_SUCCESS;
}
