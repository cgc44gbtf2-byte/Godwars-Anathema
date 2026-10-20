"""Counted reversal of the isolated checked-tilde reader and finger wiring.

The catalogs still compare complete older modules to their original Git blobs.
New functions are separately exercised by real-engine and pure parser tests.
"""
from inventory_records_source import without_inventory_records_stage

def once(text,new,old,count=1):
    assert text.count(new)==count,(repr(new[:100]),text.count(new),count)
    return text.replace(new,old)

def without_string_readers_stage(text):
    text=without_inventory_records_stage(text)
    if '#include "file_string.h"\n' not in text:return text
    text=once(text,'#include "file_string.h"\n','')
    if 'char *fread_string( FILE *fp )' in text:
        start=text.index('/* Session records use checked input.')
        end=text.index('    return FILE_STRING_OK;\n}\n',start)+len('    return FILE_STRING_OK;\n}\n')
        assert text[end]=='\n'
        end+=1
        text=once(text,text[start:end],'')
    elif 'void fread_tiny_obj(' in text:
        start=text.index('static char *saved_string(FILE *fp,bool *ok)')
        end=text.index('bool load_char_obj(',start)
        text=once(text,text[start:end],'')
        text=once(text,'#define fread_string(fp) saved_string((fp),&numbers_ok)\n','')
        text=once(text,'#undef fread_string\n','')
        text=once(text,'    if(strlen(ch->pcdata->title)>=sizeof buf-1){\n'
                  '        numbers_ok=FALSE;fMatch=TRUE;break;\n'
                  '    }\n','')
        text=once(text,'char *f_createtime\t= str_empty;','char *f_createtime\t= 0;')
        for field in ('f_createtime','f_email','f_marriage','f_lasthost','f_lasttime','f_title'):
            text=once(text,f'finger_string(fp,&numbers_ok,{field})','fread_string( fp )')
        text=once(text,'            if(!numbers_ok){fMatch=TRUE;break;}\n','')
        text=once(text,'if(!drop_out) numbers_ok=FALSE;\n','')
        text=once(text,'    if(victim==NULL){\n'
                  '        free_string(f_title);free_string(f_lasthost);free_string(f_lasttime);\n'
                  '        free_string(f_createtime);free_string(f_marriage);free_string(f_email);\n'
                  '    }\n','')
        for prefix in ('" %s", f_title','"%s%s%s.\\n\\r", f_name',
                       '"Last connected from %s at %s.\\n\\r", f_lasthost',
                       '"Last connected at %s.\\n\\r", f_lasttime'):
            text=once(text,'snprintf( buf, sizeof buf, '+prefix,'sprintf( buf, '+prefix)
        text=once(text,'snprintf(buf, sizeof buf, "Create Time: %s.\\n\\r", f_createtime)',
                        'sprintf(buf, "Create Time: %s.\\n\\r", f_createtime)')
        for prefix in ('"is married to %s.\\n\\r",f_marriage',
                       '"is engaged to %s.\\n\\r",f_marriage',
                       '"Email: %s\\n\\r", f_email'):
            text=once(text,'snprintf(buf,sizeof buf,'+prefix,'sprintf(buf,'+prefix)
    return text
