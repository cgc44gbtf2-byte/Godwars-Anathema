"""Counted reversal of only the checked-word stage; old pins stay unchanged."""
def once(text,new,old,count=1):
    assert text.count(new)==count,(repr(new[:90]),text.count(new),count)
    return text.replace(new,old)

OLD_WORD=r"""char *fread_word( FILE *fp )
{
    static char word[MAX_INPUT_LENGTH];
    char *pword;
    int cEnd, c;

    do
    {
\tcEnd = getc( fp );
    }
    while ( isspace( cEnd ) );

    if (cEnd == EOF)
    {
        word[0] = '\0';
        return word;
    }

    if ( cEnd == '\'' || cEnd == '"' )
    {
\tpword   = word;
    }
    else
    {
\tword[0] = cEnd;
\tpword   = word+1;
\tcEnd    = ' ';
    }

    for ( ; pword < word + MAX_INPUT_LENGTH; pword++ )
    {
c = getc(fp);
if (c == EOF)
{
    if (cEnd != ' ')
    {
        bug("Fread_word: unterminated quote.", 0);
        exit(1);
    }
    *pword = '\0';
    return word;
}
*pword = (char)c;
if ( cEnd == ' ' ? isspace(c) : c == cEnd )
\t{
\t    if ( cEnd == ' ' )
\t\tungetc( *pword, fp );
\t    *pword = '\0';
\t    return word;
\t}
    }

    bug( "Fread_word: word too long.", 0 );
    exit( 1 );
    return NULL;
}
""".replace(r"\t","\t")
NEW_WORD="""char *fread_word( FILE *fp )
{
    static char word[MAX_INPUT_LENGTH];
    enum file_word_status status=fread_word_checked(fp,word,sizeof word);
    if (status==FILE_WORD_OK || status==FILE_WORD_EOF) return word;
    bug( "Fread_word: invalid, overlong or unreadable word.", 0 );
    exit( 1 );
    return NULL;
}
"""
SAVED_WORD="""/* Session reads must never use the boot parser's process-exit policy. */
static char *saved_word(FILE *fp, bool *ok)
{
    static char word[MAX_INPUT_LENGTH];
    if (!*ok) { word[0]='\\0'; return word; }
    if (fread_word_checked(fp,word,sizeof word)!=FILE_WORD_OK) *ok=FALSE;
    return word;
}

"""
def without_word_readers_stage(text):
    if '#include "file_word.h"\n' not in text:return text
    text=once(text,'#include "file_word.h"\n','')
    if 'char *fread_word( FILE *fp )' in text:
        return once(text,NEW_WORD,OLD_WORD)
    if 'void fread_tiny_obj(' in text:
        text=once(text,SAVED_WORD,'')
        text=once(text,'#define fread_word(fp) saved_word((fp),&numbers_ok)\n','')
        text=once(text,'#undef fread_word\n','')
        text=once(text,
                  '            bool section_ok=TRUE;\n'
                  '            word=saved_word(fp,&section_ok);\n'
                  '            if (!section_ok) { ch->pcdata->player_load_failed=1; break; }',
                  '\t    word = fread_word( fp );',count=2)
        for diagnostic in ('# not found.','bad section.'):
            text=once(text,
                      '                ch->pcdata->player_load_failed=1;\n'
                      f'\t\tbug( "Load_char_obj: {diagnostic}", 0 );',
                      f'\t\tbug( "Load_char_obj: {diagnostic}", 0 );',count=2)
        text=once(text,'        word=fread_word(fp);\n'
                  '        if (!numbers_ok) { continue; }',
                  '\tword   = feof( fp ) ? "End" : fread_word( fp );',count=3)
        text=once(text,'    while ( !drop_out && numbers_ok )\n\t    {\n'
                  '\tword   = feof( fp ) ? "End" : fread_word( fp );',
                  '    while ( !drop_out && numbers_ok )\n\t    {\n'
                  '\t\tword   = feof( fp ) ? "End" : fread_word( fp );')
    return text
