"""Counted reversal of the inventory-only structural reader checks."""
def without_inventory_records_stage(text):
    from admin_config_source import without_admin_config_stage
    text=without_admin_config_stage(text)
    from file_numbers_source import without_hybrid_conversion_backend
    text=without_hybrid_conversion_backend(text)
    marker="/* Publish only a complete record."
    if marker not in text:return text
    def once(new,old):
        nonlocal text
        assert text.count(new)==1,(new[:100],text.count(new))
        text=text.replace(new,old)
    once("    fVnum\t\t= FALSE;","    fVnum\t\t= TRUE;")
    once("if ( !fNest || !fVnum || obj->pIndexData==NULL ||\n"
         "     (iNest>0 && rgObjNest[iNest-1]==NULL) )",
         "if ( !fNest || !fVnum || obj->pIndexData==NULL )")
    once("\t\t    /* Publish only a complete record. An ascending/root entry\n"
         "\t\t     * invalidates every cached descendant of the previous path. */\n"
         "\t\t    for (unsigned n=(unsigned)iNest+1;n<MAX_NEST;n++) rgObjNest[n]=NULL;\n"
         "\t\t    rgObjNest[iNest]=obj;\n","")
    once("\t\t    if ( iNest == 0 )",
         "\t\t    if ( iNest == 0 || rgObjNest[iNest] == NULL )")
    once("\t\tif ( fNest || iNest < 0 || iNest >= MAX_NEST )",
         "\t\tif ( iNest < 0 || iNest >= MAX_NEST )")
    once("\t\t    fNest = TRUE;",
         "\t\t    rgObjNest[iNest] = obj;\n\t\t    fNest = TRUE;")
    once("\t\tif (fVnum) { numbers_ok=FALSE; break; }\n","")
    once("    if ( !numbers_ok || iNest < 0 || iNest >= MAX_NEST ||\n"
         "         (iNest>0 && rgObjNest[iNest-1]==NULL) )",
         "    if ( iNest < 0 || iNest >= MAX_NEST )")
    once("\tfNest = TRUE;","\trgObjNest[iNest] = obj;\n\tfNest = TRUE;")
    once("    for (unsigned n=(unsigned)iNest+1;n<MAX_NEST;n++) rgObjNest[n]=NULL;\n"
         "    rgObjNest[iNest]=obj;\n    if ( iNest == 0 )",
         "    if ( iNest == 0 || rgObjNest[iNest] == NULL )")
    return text
