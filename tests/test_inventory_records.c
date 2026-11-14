/* Inventory topology in a disposable world, never live player files. */
#define FILE_WORDS_FIXTURE_ONLY
#include "test_file_words.c"
#include <signal.h>
#include <unistd.h>
static unsigned inventory_rows;
static uint64_t inventory_walk(OBJ_DATA *list,OBJ_DATA *parent,unsigned depth,
                               unsigned *count,uint64_t total)
{
    for(OBJ_DATA *obj=list;obj;obj=obj->next_content){
        assert(++*count<=12&&depth<8&&obj->in_obj==parent);
        assert(parent||obj->carried_by);
        char row[512];
        int n=snprintf(row,sizeof row,"%u:%s:%ld:%ld:%ld:%ld:%ld:%ld:%ld",
            depth,obj->name,obj->pIndexData->vnum,obj->level,obj->cost,
            obj->weight,obj->item_type,obj->value[0],obj->value[3]);
        assert(n>0&&(size_t)n<sizeof row);total=hash(total,row,(size_t)n);
        for(AFFECT_DATA *p=obj->affected;p;p=p->next){
            long fields[]={p->duration,p->modifier,p->location};
            total=hash(total,fields,sizeof fields);
        }
        for(EXTRA_DESCR_DATA *p=obj->extra_descr;p;p=p->next){
            total=hash(total,p->keyword,strlen(p->keyword));
            total=hash(total,p->description,strlen(p->description));
        }
        total=inventory_walk(obj->contains,obj,depth+1,count,total);
    }
    return total;
}
static uint64_t inventory_snapshot(CHAR_DATA *ch,int dump,uint64_t total)
{
    unsigned count=0;
    uint64_t value=inventory_walk(ch->carrying,NULL,0,&count,
                                  UINT64_C(1469598103934665603));
    value=hash(value,&count,sizeof count);
    if(dump)printf("%u,%u,%llu\n",inventory_rows,count,(unsigned long long)value);
    inventory_rows++;return hash(total,&value,sizeof value);
}
static void inventory_object(FILE *fp,unsigned depth,unsigned node,int tiny)
{
    if(tiny){
        assert(fprintf(fp,"#O\n900 %u %u -1 %u 0 0 0 0 2 7 11 17\n",
                       node+1,depth,node+101)>0);return;
    }
    assert(fprintf(fp,"#OBJECT\nNest %u\nVnum 900\n"
        "Name initial~\nName item%u~\nShortDescr an item~\nDescription An item.~\n"
        "ItemType %d\nWeight 1\nLevel %u\nCost 0\nCost %u\n"
        "Values 2 7 11 17\n",depth,node,ITEM_TRASH,node+1,node+101)>0);
    if(node==0)assert(fputs("Affect 1 2 3\nAffect 4 5 6\n"
        "ExtraDescr first~ first value~\nExtraDescr second~ second value~\n",fp)>=0);
    assert(fputs("End\n",fp)>=0);
}
static FILE *inventory_begin(void)
{
    FILE *fp=fopen("../player/Numfixture","w");assert(fp);
    assert(fputs("#PLAYER\nName Numfixture~\nLevel 2\nEnd\n",fp)>=0);return fp;
}
static void inventory_end(FILE *fp)
{assert(fputs("#END\n",fp)>=0);assert(fclose(fp)==0);}
static uint64_t inventory_native(int dump)
{
    const unsigned shapes[][7]={{1,0},{2,0,0},{2,0,1},{3,0,1,2},
        {3,0,1,1},{4,0,1,0,1},{5,0,1,2,1,2},{6,0,1,2,0,1,2}};
    uint64_t total=UINT64_C(1469598103934665603);inventory_rows=0;
    for(unsigned shape=0;shape<8;shape++)
    for(unsigned mask=0;mask<(1u<<shapes[shape][0]);mask++){
        FILE *fp=inventory_begin();
        for(unsigned n=0;n<shapes[shape][0];n++)
            inventory_object(fp,shapes[shape][n+1],n,(mask>>n)&1);
        inventory_end(fp);CHAR_DATA *ch=read_player(0);
        assert(!ch->pcdata->player_load_failed);
        total=inventory_snapshot(ch,dump,total);ch->in_room=&room;
        assert(save_char_obj_checked(ch));drop_player(ch);
        assert(!object_list&&!index_data.count);
        ch=read_player(0);assert(!ch->pcdata->player_load_failed);
        total=inventory_snapshot(ch,dump,total);drop_player(ch);
        assert(!object_list&&!index_data.count);
        ch=read_player(1);assert(!ch->pcdata->player_load_failed&&!ch->carrying);
        drop_player(ch);
    }
    assert(inventory_rows==276);
    if(!dump)printf("INVENTORY_NATIVE_ROWS=%u HASH=%llu\n",inventory_rows,
                    (unsigned long long)total);
    return total;
}
static void inventory_bad(unsigned kind)
{
    FILE *fp=inventory_begin();
    if(kind==2||kind==3){
        inventory_object(fp,0,0,kind==3);
        inventory_object(fp,1,1,kind==3);
        inventory_object(fp,0,2,kind==3);
    }
    if(kind==1||kind==3){
        assert(fprintf(fp,"#O\n900 1 %u -1 0 0 0 0 0 0 0 0 0\n",
                       kind==1?1:2)>0);
    }else{
        assert(fputs("#OBJECT\n",fp)>=0);
        if(kind!=8)assert(fputs("Vnum 900\n",fp)>=0);
        if(kind==6)assert(fputs("Vnum 900\n",fp)>=0);
        if(kind==4||kind==5)assert(fputs("Nest 0\n",fp)>=0);
        long nest=kind==0?1:kind==2?2:kind==5?1:kind==9?-1:kind==10?100:0;
        assert(fprintf(fp,"Nest %ld\nEnd\n",nest)>0);
    }
    inventory_end(fp);
    char before[4096],after[sizeof before];fp=fopen("../player/Numfixture","r");assert(fp);
    size_t n=fread(before,1,sizeof before,fp);assert(feof(fp)&&!ferror(fp));fclose(fp);
    alarm(3);CHAR_DATA *ch=read_player(0);alarm(0);
    assert(ch->pcdata->player_load_failed);
    assert(!save_char_obj_checked(ch));fp=fopen("../player/Numfixture","r");assert(fp);
    size_t m=fread(after,1,sizeof after,fp);assert(feof(fp)&&!ferror(fp));fclose(fp);
    assert(n==m&&!memcmp(before,after,n));drop_player(ch);
    assert(!object_list&&!index_data.count);
    player_fixture("Gold 23");ch=read_player(0);
    assert(!ch->pcdata->player_load_failed&&ch->gold==23);drop_player(ch);
}
int main(int argc,char **argv)
{
    fixture_begin();
    if(argc>2&&!strcmp(argv[1],"--boundary"))inventory_bad((unsigned)atoi(argv[2]));
    else {
        inventory_native(argc>1&&!strcmp(argv[1],"--native-dump"));
        if(argc==1){
            const unsigned bad[]={0,1,2,3,4,5,6,8,9,10};
            for(unsigned i=0;i<sizeof bad/sizeof *bad;i++)inventory_bad(bad[i]);
            puts("Inventory records: native full/compact trees, repeated fields, save/reload and structural rejection/recovery passed.");
        }
    }
    assert(fclose(fpReserve)==0);fpReserve=NULL;board_names(0);
    unlink("../player/Numfixture");assert(rmdir("../player")==0);
    char_list=NULL;descriptor_list=NULL;
    room_index_hash[ROOM_VNUM_TEMPLE%MAX_KEY_HASH]=NULL;
    obj_index_hash[900%MAX_KEY_HASH]=NULL;sandbox_end();return 0;
}
