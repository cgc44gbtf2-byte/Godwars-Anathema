"""Reverse only this release's bounded factory and reserve/reader changes.

Older source pins still compare their original modules, not a refreshed SHA.
Each replacement is counted so an unreviewed neighboring edit fails closed.
"""
from word_readers_source import without_word_readers_stage

def once(text, new, old, count=1):
    assert text.count(new)==count,(repr(new[:100]),text.count(new),count)
    return text.replace(new,old)

def without_reader_fields_stage(text):
    from werewolf_source import without_werewolf_stage
    text=without_werewolf_stage(text)
    if "void do_scry(" in text:
        from scry_source import without_scry_stage
        text=without_scry_stage(text)
    from aux_readers_source import without_aux_readers_stage
    text=without_aux_readers_stage(text)
    text=without_word_readers_stage(text)
    if '#include "file_io.h"\n' not in text:return text
    text=once(text,'#include "file_io.h"\n','')
    if 'OBJ_DATA *create_object(' in text:
        text=once(text,'#include "object_numbers.h"\n','')
        text=once(text,
            '    /* Keep the three RNG draws in legacy order, but multiply as long. */\n'
            '    long cost_base=number_fuzzy(10);\n'
            '    long cost_first=number_fuzzy(object_fuzzy_input(level));\n'
            '    long cost_second=number_fuzzy(object_fuzzy_input(level));\n'
            '    obj->cost=object_cost_bounded(cost_base,cost_first,cost_second);',
            '    obj->cost\t\t= number_fuzzy( 10 )\n'
            '\t\t\t* number_fuzzy( level ) * number_fuzzy( level );')
        text=once(text,'number_fuzzy(number_fuzzy(object_fuzzy_input(obj->value[0])))',
                  'number_fuzzy( number_fuzzy( obj->value[0] ) )')
        text=once(text,'number_fuzzy(object_fuzzy_input(obj->value[0]))',
                  'number_fuzzy( obj->value[0] )',count=2)
        text=once(text,'number_fuzzy(object_fuzzy_input(obj->value[1]))',
                  'number_fuzzy( obj->value[1] )')
        text=once(text,'    case 0:  if(number>INT_MIN) number -= 1; break;\n'
                  '    case 3:  if(number<INT_MAX) number += 1; break;',
                  '    case 0:  number -= 1; break;\n    case 3:  number += 1; break;')
        text=once(text,'    file_reserve_close();\n    if ( ( fp = fopen( file, "a" ) )',
                  '    fflush( fpReserve );\n    fclose( fpReserve );\n'
                  '    if ( ( fp = fopen( file, "a" ) )')
        text=once(text,'    file_reserve_open();\n    return;\n}',
                  '    fpReserve = fopen( NULL_FILE, "r" );\n    return;\n}')
        text=once(text,'    log_string( buf );\n\n    file_reserve_open();',
                  '    log_string( buf );\n\n'
                  '    if ( fpReserve != NULL )\n    {\n'
                  '        fflush( fpReserve );\n        fclose( fpReserve );\n'
                  '    }\n    fpReserve = fopen( NULL_FILE, "r" );')
    elif 'void fread_tiny_obj(' in text:
        text=once(text,'#include <errno.h>\n','')
        text=once(text,'    file_reserve_close();\n    result = atomic_save(',
                  '    if (fpReserve != NULL)\n    {\n'
                  '        fclose(fpReserve);\n        fpReserve = NULL;\n'
                  '    }\n    result = atomic_save(')
        text=once(text,'    file_reserve_open();\n    if (fpReserve == NULL)',
                  '    fpReserve = fopen( NULL_FILE, "r" );\n    if (fpReserve == NULL)')
        text=once(text,'    file_reserve_close();\n    sprintf( strsave, "%s%s", PLAYER_DIR, capitalize( name ) );',
                  '    fflush( fpReserve );\n    fclose( fpReserve );\n'
                  '    sprintf( strsave, "%s%s", PLAYER_DIR, capitalize( name ) );',
                  count=2)
        text=once(text,
                  '        if(ferror(fp)) ch->pcdata->player_load_failed=1;\n'
                  '        if(fclose(fp)!=0) ch->pcdata->player_load_failed=1;\n'
                  '    }\n    else if(errno!=ENOENT) {\n'
                  '        /* An inaccessible existing account must not become a new signup. */\n'
                  '        found=TRUE;ch->pcdata->player_load_failed=1;\n'
                  '    }\n    file_reserve_open();',
                  '\tfflush( fp );\n\tfclose( fp );\n    }\n\n'
                  '    fpReserve = fopen( NULL_FILE, "r" );')
        text=once(text,
                  '        if(ferror(fp)) ch->pcdata->player_load_failed=1;\n'
                  '        if(fclose(fp)!=0) ch->pcdata->player_load_failed=1;\n'
                  '    }\n    else if(errno!=ENOENT) {\n'
                  '        found=TRUE;ch->pcdata->player_load_failed=1;\n'
                  '    }\n    file_reserve_open();',
                  '\tfflush( fp );\n\tfclose( fp );\n    }\n\n'
                  '    fpReserve = fopen( NULL_FILE, "r" );')
        text=once(text,
                  'file_reserve_close();\n\tsprintf( strsave, "%s%s", PLAYER_DIR, capitalize(argument) );',
                  '\tfflush( fpReserve );\n\tfclose( fpReserve );\n'
                  '\tsprintf( strsave, "%s%s", PLAYER_DIR, capitalize(argument) );')
        text=once(text,
                  '\t\tsend_to_char("Their playerfile has been corrupted.\\n\\r",ch);\n'
                  '\t\tfclose( fp );\nfile_reserve_open();',
                  '\t\tsend_to_char("Their playerfile has been corrupted.\\n\\r",ch);\n'
                  '\t\tfflush( fp );\n\t\tfclose( fp );\n'
                  '\t\tfpReserve = fopen( NULL_FILE, "r" );')
        text=once(text,
                  '\telse\n\t{\n    file_reserve_open();\n'
                  '    send_to_char("Their playerfile could not be opened.\\n\\r",ch);\n'
                  '    return;\n\t}\n'
                  'if(ferror(fp)) numbers_ok=FALSE;\n'
                  'if(fclose(fp)!=0) numbers_ok=FALSE;\n'
                  'file_reserve_open();',
                  '\telse\n\t{\n\t    bug( "Do_finger: fopen", 0 );\n'
                  '\t    perror( strsave );\n\t}\n'
                  '\tfflush( fp );\n\tfclose( fp );\n'
                  '\tfpReserve = fopen( NULL_FILE, "r" );')
        text=once(text,'Their playerfile could not be read safely.',
                  'Their playerfile has invalid numeric data.')
    elif 'bool char_exists(' in text:
        text=once(text,'        file_reserve_close();',
                  '        fclose(fpReserve);')
        text=once(text,'   file_reserve_open();',
                  '   fpReserve = fopen( NULL_FILE, "r" );')
    elif 'bool valid_name(' in text:
        text=once(text,'#include <errno.h>\n','')
        text=once(text,
                  '\t\t/* Failed reads are not proof a name is available. */\n'
                  '\t\tvalid = errno==ENOENT && check_parse_name(name);',
                  '\t\tvalid = check_parse_name(name);')
        text=once(text,'file_reserve_close();','\tfclose(fpReserve);')
        text=once(text,'   file_reserve_open();',
                  '   fpReserve = fopen( NULL_FILE, "r" );')
    else:
        assert 'social_table' in text and 'ban_list' in text
        text=once(text,'      file_reserve_close();','      fclose(fpReserve);')
        text=once(text,'      file_reserve_open();',
                  '      fpReserve = fopen(NULL_FILE, "r");')
    return text
