"""Reverse only the reviewed numeric-reader edits for older native-source pins."""
def replace_once(text,old,new="",count=1):
    assert text.count(old)==count,(old,text.count(old),count)
    return text.replace(old,new)

def without_file_number_db(text):
    text=text.replace('#include "file_number.h"\n',"",1)
    start=text.index("long fread_number(")
    stop=text.index("char *fread_string(",start)
    return text[:start]+"/* checked numeric reader scope */\n\n"+text[stop:]

def without_file_number_save(text):
    if "static long saved_number(" not in text:return text
    text=replace_once(text,'#include "file_number.h"\n')
    start=text.index("/* Numeric player-reader scope:")
    stop=text.index("/* Keep the persisted format intact.",start)
    text=text[:start]+text[stop:]
    start=text.index("/* This object has not been published to object_list")
    stop=text.index("void fread_obj(",start)
    text=text[:start]+text[stop:]
    text=replace_once(text,"    bool numbers_ok=TRUE;\n",count=4)
    text=replace_once(text,"\tfor ( ; ; )\n\t{\n    if (ch->pcdata->player_load_failed) break;\n",
                      "\tfor ( ; ; )\n\t{\n",count=2)
    text=replace_once(text,"if (!numbers_ok) { ch->pcdata->player_load_failed=1; return; }\n")
    text=replace_once(text,
        "if (!numbers_ok || num<0 || num>1024) {\n"
        "    ch->pcdata->player_load_failed=1; return;\n}\n"
        "for ( ; num && numbers_ok; num--)",
        "\t\tfor ( ; num; num--)")
    text=replace_once(text,
        "long vnum=fread_number(fp);\n"
        "if (!numbers_ok || vnum<0) { numbers_ok=FALSE; break; }\n"
        "ch->in_room = get_room_index(vnum);",
        "\t\tch->in_room = get_room_index( fread_number( fp ) );")
    text=replace_once(text,
        "long vnum=fread_number(fp);\n"
        "if (!numbers_ok || vnum<0) { numbers_ok=FALSE; break; }\n"
        "ch->pIndexData = get_mob_index(vnum);",
        "\t\tch->pIndexData = get_mob_index( fread_number( fp ) );")
    text=replace_once(text,
        "if (!numbers_ok) {\n"
        "    ch->pcdata->player_load_failed=1;discard_saved_object(obj);return;\n}\n")
    text=replace_once(text,
        "if ( !fNest || !fVnum || obj->pIndexData==NULL )\n\t\t{\n"
        "    ch->pcdata->player_load_failed=1;\n    discard_saved_object(obj);\n",
        "\t\tif ( !fNest || !fVnum )\n\t\t{\n"
        '\t\t    bug( "Fread_obj: incomplete object.", 0 );\n'
        "\t\t    free_string( obj->name        );\n"
        "\t\t    free_string( obj->description );\n"
        "\t\t    free_string( obj->short_descr );\n"
        "\t\t    obj->next = obj_free;\n\t\t    obj_free  = obj;\n")
    text=replace_once(text,
        "if ( iNest < 0 || iNest >= MAX_NEST )\n\t\t{\n    numbers_ok=FALSE;",
        'if ( iNest < 0 || iNest >= MAX_NEST )\n\t\t{\n\t\t    bug( "Fread_obj: bad nest %li.", iNest );')
    text=replace_once(text,"if (!numbers_ok) break;\n")
    text=replace_once(text,"if (!numbers_ok || vnum<0) { numbers_ok=FALSE; break; }\n")
    text=replace_once(text,"\nnumbers_ok=FALSE;\n",
        '\n\t\t\tbug( "Fread_obj: bad vnum %li.", vnum );\n')
    text=replace_once(text,
        'if (!numbers_ok) continue;\nif ( !fMatch )\n\t{\n\t    sprintf(buf,',
        '\tif ( !fMatch )\n\t{\n\t    sprintf(buf,')
    text=replace_once(text,"if (!numbers_ok) continue;\nif ( !fMatch )","\tif ( !fMatch )")
    text=replace_once(text,
        "    if (!numbers_ok || vnum<0 || (prototype=get_obj_index(vnum))==NULL) {\n"
        "        ch->pcdata->player_load_failed=1;return;\n    }\n")
    text=replace_once(text,"    OBJ_INDEX_DATA *prototype;\n")
    text=replace_once(text,"create_object( prototype, level )","create_object( get_obj_index( vnum ), level )")
    text=replace_once(text,"\nch->pcdata->player_load_failed=1;\n\treturn;\n",
        '\n\tbug( "Fread_tiny_obj: bad vnum %li.", vnum );\n\treturn;\n')
    text=replace_once(text,
        "    if ( iNest < 0 || iNest >= MAX_NEST )\n    {\n"
        "ch->pcdata->player_load_failed=1;extract_obj(obj);return;",
        '    if ( iNest < 0 || iNest >= MAX_NEST )\n    {\n'
        '\tbug( "Fread_tiny_obj: bad nest %li.", iNest );')
    text=replace_once(text,
        "    if (!numbers_ok) {\n"
        "        for (unsigned n=0;n<MAX_NEST;n++) if(rgObjNest[n]==obj) rgObjNest[n]=NULL;\n"
        "        ch->pcdata->player_load_failed=1;extract_obj(obj);return;\n    }\n")
    text=replace_once(text,"    while ( !drop_out && numbers_ok )","\t    while ( !drop_out )")
    for var in ("f_title","f_lasthost","f_lasttime","f_marriage","f_email"):
        text=replace_once(text,"    char *"+var+"\t= str_empty;",
                         "    char *"+var+'\t= "";')
    text=replace_once(text,
        "    if (!numbers_ok) {\n"
        "        free_string(f_title);free_string(f_lasthost);free_string(f_lasttime);\n"
        "        free_string(f_createtime);free_string(f_marriage);free_string(f_email);\n"
        '        send_to_char("Their playerfile has invalid numeric data.\\n\\r",ch);\n'
        "        return;\n    }\n")
    return replace_once(text,"#undef fread_number\n")
