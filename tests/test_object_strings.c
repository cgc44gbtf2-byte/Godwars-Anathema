/* Real factory/extraction: OLC strings are mutable, boot strings are shared. */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "merc.h"

extern char *string_space,*top_string;
extern bool fBootDb;
extern void init_mm(void);

static char *intern(const char *value)
{
    FILE *stream=tmpfile();assert(stream);
    fprintf(stream,"%s~",value);rewind(stream);
    fBootDb=TRUE;char *text=fread_string(stream);fBootDb=FALSE;
    assert(fclose(stream)==0);return text;
}

static void check(int shared)
{
    OBJ_INDEX_DATA prototype={0};prototype.vnum=43200;prototype.item_type=ITEM_TRASH;
    char **source[]={&prototype.name,&prototype.short_descr,&prototype.description,
        &prototype.chpoweron,&prototype.chpoweroff,&prototype.chpoweruse,
        &prototype.victpoweron,&prototype.victpoweroff,&prototype.victpoweruse};
    for(size_t i=0;i<9;i++)*source[i]=shared?intern("Shared template text"):str_dup("Mutable template text");
    OBJ_DATA *obj=create_object(&prototype,0);
    char **dest[]={&obj->name,&obj->short_descr,&obj->description,
        &obj->chpoweron,&obj->chpoweroff,&obj->chpoweruse,
        &obj->victpoweron,&obj->victpoweroff,&obj->victpoweruse};
    for(size_t i=0;i<9;i++) {
        assert(!strcmp(*source[i],*dest[i]));
        assert(shared?*source[i]==*dest[i]:*source[i]!=*dest[i]);
        /* Repeated live OLC edits must not invalidate an existing object. */
        free_string(*source[i]);*source[i]=str_dup("Edited template");
        assert(!strcmp(*dest[i],shared?"Shared template text":"Mutable template text"));
    }
    extract_obj(obj);
    for(size_t i=0;i<9;i++) {
        assert(!strcmp(*source[i],"Edited template"));
        free_string(*source[i]);*source[i]=NULL;
    }
    assert(prototype.count==0&&!object_list);
}

static void partial(void)
{
    OBJ_INDEX_DATA prototype={0};prototype.vnum=43201;prototype.item_type=ITEM_TRASH;
    prototype.chpoweron=str_dup("One supplied message");
    OBJ_DATA *obj=create_object(&prototype,0);
    assert(obj->name&&!obj->name[0]&&obj->short_descr&&!obj->short_descr[0]&&
           obj->description&&!obj->description[0]);
    assert(!strcmp(obj->chpoweron,"One supplied message"));
    assert(!strcmp(obj->chpoweroff,"(null)")&&!strcmp(obj->chpoweruse,"(null)")&&
           !strcmp(obj->victpoweron,"(null)")&&!strcmp(obj->victpoweroff,"(null)")&&
           !strcmp(obj->victpoweruse,"(null)"));
    free_string(prototype.chpoweron);
    assert(!strcmp(obj->chpoweron,"One supplied message"));
    extract_obj(obj);assert(!object_list&&prototype.count==0);
}

int main(void)
{
    string_space=malloc(1024*1024);assert(string_space);top_string=string_space;*top_string='\0';
    init_mm();fBootDb=FALSE;
    for(int i=0;i<128;i++){check(0);check(1);partial();}
    puts("object strings:128 mutable/shared/partial template cycles;9-field ownership, live OLC edits and extraction passed");
    return 0;
}
