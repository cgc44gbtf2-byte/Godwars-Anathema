#if defined(macintosh)
#include <types.h>
#else
#include <sys/types.h>
#endif
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "merc.h"
#include "progress_metrics.h"
#include "file_number.h"
#include "file_string.h"
#include "file_word.h"
#include "atomic_save.h"
#include <limits.h>
#include <unistd.h>

LEADER_BOARD leader_board;
static bool leaderboard_load_failed;
static bool leaderboard_save_pending;
static time_t leaderboard_retry_after;

void do_expmod( CHAR_DATA *ch, char *argument )
{
    char buf[MSL];
    char arg1[MIL];
    char arg2[MIL];
    int mod, time, bit;

    argument = one_argument(argument, arg1);
    argument = one_argument(argument, arg2);

    if (arg1[0] == '\0' || arg2[0] == '\0')
    {
	if (IS_SET(exp_info.bit, EXP_RANDOM))
	    sprintf(buf, "Current Expmod: Random%% for %li minutes.\n\r", exp_info.time / 60);
	else if (IS_SET(exp_info.bit, EXP_DEGRADE))
	    sprintf(buf, "Current Expmod: %li%% for %li minutes.  Degrading.\n\r", exp_info.exp, exp_info.time / 60);
	else
	    sprintf(buf, "Current Expmod: %li%% for %li minutes.\n\r", exp_info.exp, exp_info.time / 60);
	stc(buf, ch);
	stc("Syntax: expmod <amount> <time>.\n\r", ch);
	stc(" Amount: 0 - 250, in %.\n\r", ch);
	stc(" Time: 0 - 300, in minutes.  0 is Permanent.\n\r", ch);
	stc("Syntax: expmod random <time>.  Randomly gives between 101 and 200% on each kill.\n\r", ch);
	stc("Syntax: expmod <amount> degrade.  Starts at amount, and goes down 25% every 15 minutes.\n\r", ch);
        return;
    }

    if (!str_cmp(arg1, "random"))
    {
	mod = 101;
	time = atoi(arg2);
	bit = EXP_RANDOM;
    }
    else if (!str_cmp(arg2, "degrade"))
    {
	mod = atoi(arg1);
	if (mod < 100)
	{
	    stc("You can't set a degrading penalty.\n\r", ch);
	    return;
	}
	time = 15;
	bit = EXP_DEGRADE;
    }
    else
    {
	mod = atoi(arg1);
	time = atoi(arg2);
	bit = 0;
    }

    if (mod < 0 || mod > 250 || time < 0 || time > 300)
    {
	do_expmod(ch, "");
	return;
    }

    if (!IS_ST(ch) && mod != 100 && (time == 0 || time > 120))
    {
	stc("Only Storytellers may activate expmod forever.\n\r", ch);
	return;
    }

    if (mod == 100)
    {
	sprintf(buf, "#r%s#n has removed the expmod.", ch->name);
	time = 0;
	bit = 0;
    }
    else if (bit == EXP_RANDOM)
    {
	if (time == 0) sprintf(buf, "#r%s#n has activated Random%% exp forever!", ch->name);
	else sprintf(buf, "#r%s#n has activated Random%% exp for %i minutes.", ch->name, time);
    }
    else if (bit == EXP_DEGRADE) sprintf(buf, "#r%s#n has activated a %i%% degrading exp.", ch->name, mod);
    else if (time == 0) sprintf(buf, "#r%s#n has activated %i%% exp forever!", ch->name, mod);
    else sprintf(buf, "#r%s#n has activated %i%% exp for %i minutes.", ch->name, mod, time);
    do_info(ch, buf);

    exp_info.bit = bit;
    exp_info.exp = mod;
    exp_info.time = time * 60;
    return;
}

void do_godstrike( CHAR_DATA *ch, char *argument )
{
    char arg1[MIL];
    char arg2[MIL];
    char buf[MIL];
    CHAR_DATA *victim;
    long amount;
    argument = one_argument(argument, arg1);
    argument = one_argument(argument, arg2);

    if (arg1[0] == '\0' || arg2[0] == '\0')
    {
	stc("Syntax: Godstrike <target> <voltage>\n\r", ch);
	return;
    }

    if ((victim = get_char_world(ch, arg1)) == NULL)
    {
	stc("That person is not playing.\n\r", ch);
	return;
    }

    if (!IS_NPC(victim) && victim->level >= ch->level)
    {
	stc("You failed.\n\r", ch);
	return;
    }

    if (!str_cmp(arg2, "all")) amount = victim->hit - 1;
    else amount = atoi(arg2);

    if (amount <= 0)
    {
	stc("You have to strike with something.\n\r", ch);
	return;
    }

    victim->hit -= amount;
    sprintf(buf, "You godstrike %s for %li.\n\r", victim->name, amount);
    stc(buf, ch);
    sprintf(buf, "%s godstrikes you for %li.\n\r", ch->name, amount);
    stc(buf, victim);
    sprintf(buf, "#r%s#n calls down a %li volt #LGod #CStrike#n on #r%s#n.", ch->name, amount, victim->name);
    do_info(ch, buf);

    if (victim->hit < 1)
    {
	if (IS_NPC(victim)) 
	{
	    sprintf(buf, "#r%s#n has been slain by #r%s#n!", victim->short_descr, ch->name); 
	    do_info(ch, buf); 
	}
	raw_kill(victim, ch);
    } 
    return; 
}

void do_setgift( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA *victim;
    char arg1[MIL];
    char arg2[MIL];
    char arg3[MIL];
    int gift = -1, value;

    argument = one_argument(argument, arg1);
    argument = one_argument(argument, arg2);
    argument = one_argument(argument, arg3);

    if (arg1[0] == '\0' || arg2[0] == '\0' || arg3[0] == '\0')
    {
	stc("Syntax : setgift [victim] [gift/all] [level]\n\r", ch);
	stc("  Gift being one of:\n\r", ch);
	stc("  Homid, Lupus, or Metis\n\r", ch);
	stc("  Ragabash, Theurge, Philodox, Galliard, or Ahroun.\n\r", ch);
	stc("  Blackfuries, Bonegnawers, Childrenofgaia, Fianna, Getofffenris,\n\r", ch);
	stc("  Glasswalkers, Redtalons, Shadowlords, Silentstriders, Silverfangs,\n\r", ch);
	stc("  Stargazers, Uktena, or Wendigos.\n\r", ch);
	return;
    }

    if ((victim = get_char_world(ch, arg1)) == NULL)
    {
	stc("They aren't here.\n\r", ch);
	return;
    }

    if (!IS_WEREWOLF(victim))
    {
	stc("You may only set the gifts of a Werewolf.\n\r", ch);
	return;
    }

    if (!str_cmp(arg2, "all"))
    {
	value = atoi(arg3);

	if (value < 0 || value > 5)
	{
	    stc("Range is 0 - 5.\n\r", ch);
	    return;
	}

	for(gift = 0; gift <= BREED_MAX; gift++) victim->pcdata->breed[gift] = value;
	for(gift = 0; gift <= AUSPICE_MAX; gift++) victim->pcdata->auspice[gift] = value;
	for(gift = 0; gift <= TRIBE_MAX; gift++) victim->pcdata->tribes[gift] = value;

	stc("Done.\n\r", ch);
	return;
    }

    if (!str_cmp(arg2, "lupus")) gift = BREED_LUPUS;
    else if (!str_cmp(arg2, "homid")) gift = BREED_HOMID;
    else if (!str_cmp(arg2, "metis")) gift = BREED_METIS;

    if (gift >= 0)
    {
	value = atoi(arg3);

	if (value < 0 || value > 5)
	{
	    stc("Range is 0 - 5.\n\r", ch);
	    return;
	}

	victim->pcdata->breed[gift] = value;
	stc("Done.\n\r", ch);
	return;
    }

    if (!str_cmp(arg2, "ragabash")) gift = AUSPICE_RAGABASH;
    else if (!str_cmp(arg2, "theurge")) gift = AUSPICE_THEURGE;
    else if (!str_cmp(arg2, "philodox")) gift = AUSPICE_PHILODOX;
    else if (!str_cmp(arg2, "galliard")) gift = AUSPICE_GALLIARD;
    else if (!str_cmp(arg2, "ahroun")) gift = AUSPICE_AHROUN;

    if (gift >= 0)
    {
	value = atoi(arg3);

	if (value < 0 || value > 5)
	{
	    stc("Range is 0 - 5.\n\r", ch);
	    return;
	}

	victim->pcdata->auspice[gift] = value;
	stc("Done.\n\r", ch);
	return;
    }

    if (!str_cmp(arg2, "blackfuries")) gift = TRIBE_BLACK_FURIES;
    else if (!str_cmp(arg2, "bonegnawers" )) gift = TRIBE_BONE_GNAWERS;
    else if (!str_cmp(arg2, "childrenofgaia")) gift = TRIBE_CHILDREN_OF_GAIA;
    else if (!str_cmp(arg2, "fianna")) gift = TRIBE_FIANNA;
    else if (!str_cmp(arg2, "getoffenris")) gift = TRIBE_GET_OF_FENRIS;
    else if (!str_cmp(arg2, "glasswalkers")) gift = TRIBE_GLASS_WALKERS;   
    else if (!str_cmp(arg2, "redtalons")) gift = TRIBE_RED_TALONS;   
    else if (!str_cmp(arg2, "shadowlords")) gift = TRIBE_SHADOW_LORDS;
    else if (!str_cmp(arg2, "silentstriders")) gift = TRIBE_SILENT_STRIDERS;
    else if (!str_cmp(arg2, "silverfangs")) gift = TRIBE_SILVER_FANGS;
    else if (!str_cmp(arg2, "stargazers")) gift = TRIBE_STARGAZERS;
    else if (!str_cmp(arg2, "uktena")) gift = TRIBE_UKTENA;      
    else if (!str_cmp(arg2, "wendigos")) gift = TRIBE_WENDIGOS;   

    if (gift >= 0)
    {
	value = atoi(arg3);

	if (value < 0 || value > 5)
	{
	    stc("Range is 0 - 5.\n\r", ch);
	    return;
	}

	victim->pcdata->tribes[gift] = value;
	stc("Done.\n\r", ch);
	return;
    }

    do_setgift(ch, "");
    return;
}

void do_vamparmor( CHAR_DATA *ch, char *argument )
{
    OBJ_INDEX_DATA *pObjIndex;
    OBJ_DATA *obj;
    char arg[MIL];
    int vnum = 0;

    argument = one_argument(argument, arg);

    if (!IS_VAMPIRE(ch))
    {
	stc("Huh?\n\r", ch);
	return;
    }

    if (arg[0] == '\0')
    {
	stc("Please specify which piece of vampire armor you wish to make:\n\r Mark, Trenchcoat, or Vest.\n\r", ch);
	return;
    }

    if (ch->primal < 100 || ch->pcdata->quest < 400)
    {
	stc("It costs 100 points of primal and 400 quest points to create vampire equipment.\n\r", ch);
	return;
    }

    if (!str_cmp(arg,"trenchcoat")) vnum = 29631;
    else if (!str_cmp(arg,"vest")) vnum = 29632;
    else if (!str_cmp(arg,"mark")) vnum = 29630;
    else
    {
	do_vamparmor(ch, "");
	return;
    }
    
    if (vnum == 0 || (pObjIndex = get_obj_index(vnum)) == NULL)
    {
	stc("Missing object, please inform a IMP.\n\r", ch);
	return;
    }

    obj = create_object(pObjIndex, 50);
    obj_to_char(obj, ch);
    ch->primal -= 100;
    ch->pcdata->quest -= 400;

    free_string(obj->questmaker);
    obj->questmaker = str_dup(ch->name);

    free_string(obj->questowner);
    obj->questowner = str_dup(ch->name);

    act("$p appears in your hands.", ch, obj, NULL, TO_CHAR);
    act("$p appears in $n's hands.", ch, obj, NULL, TO_ROOM);
    return;
}

void do_ninjaarmor( CHAR_DATA *ch, char *argument )
{
    OBJ_INDEX_DATA *pObjIndex;
    OBJ_DATA *obj;
    char arg[MIL];
    long vnum = 0;

    argument = one_argument(argument, arg);

    if (!IS_NINJA(ch))
    {
	stc("Huh?\n\r", ch);
	return;
    }

    if (get_ninja(ch, NINJA_CHIKYU) < 1)
    {
	stc("You need Chikyu 1 to create ninja gear.\n\r", ch);
	return;
    }

    if (arg[0] == '\0')
    {
	stc("Please specify which piece of ninja gear you wish to make:\n\r Belt Boots Gi Cloak.\n\r", ch);
	return;
    }

    if (ch->primal < 100 || ch->pcdata->quest < 400)
    {
	stc("It costs 100 points of primal and 400 quest points to create ninja equipment.\n\r", ch);
	return;
    }

    if (!str_cmp(arg, "belt")) vnum = 29600;
    else if (!str_cmp(arg, "boots")) vnum = 29601;
    else if (!str_cmp(arg, "gi")) vnum = 29602;
    else if (!str_cmp(arg, "cloak")) vnum = 29603;
    else
    {
	do_ninjaarmor(ch, "");
	return;
    }
    
    if (vnum == 0 || (pObjIndex = get_obj_index(vnum)) == NULL)
    {
	stc("Missing object, please inform a IMP.\n\r", ch);
	return;
    }
    obj = create_object(pObjIndex, 50);
    obj_to_char(obj, ch);
    ch->primal -= 100;
    ch->pcdata->quest -= 400;

    free_string(obj->questmaker);
    obj->questmaker = str_dup(ch->name);
    free_string(obj->questowner);
    obj->questowner = str_dup(ch->name);
    
    act("$p appears in your hands.", ch, obj, NULL, TO_CHAR);
    act("$p appears in $n's hands.", ch, obj, NULL, TO_ROOM);
    return;
}

void do_wolfarmor( CHAR_DATA *ch, char *argument )
{
    OBJ_INDEX_DATA *pObjIndex;
    OBJ_DATA *obj;
    char arg[MIL];
    long vnum = 0;

    argument = one_argument(argument, arg);

    if (!IS_WEREWOLF(ch))
    {
	stc("Huh?\n\r", ch);
	return;
    }

    if (arg[0] == '\0')
    {
	stc("Please specify which piece of armor you wish to make:\n\r Vest, Mask, or Talisman\n\r", ch);
	return;
    }

    if (ch->primal < 100 || ch->pcdata->quest < 400)
    {
	stc("It costs 100 points of primal and 400 quest points to create wolf equipment.\n\r", ch);
	return;
    }

    if (!str_cmp(arg,"vest")) vnum = 29640;
    else if (!str_cmp(arg,"mask")) vnum = 29641;
    else if (!str_cmp(arg,"talisman")) vnum = 29642;
    else
    {
        send_to_char("Choose Vest, Mask, or Talisman.\n\r", ch);
        return;
    }
    
    if (vnum == 0 || (pObjIndex = get_obj_index(vnum)) == NULL)
    {
	stc("Missing object, please inform the staff.\n\r", ch);
	return;
    }
    obj = create_object(pObjIndex, 50);
    obj_to_char(obj, ch);
    ch->primal -= 100;
    ch->pcdata->quest -= 400;

    free_string(obj->questmaker);
    obj->questmaker = str_dup(ch->name);
    free_string(obj->questowner);
    obj->questowner = str_dup(ch->name);

    act("$p appears in your hands.", ch, obj, NULL, TO_CHAR);
    act("$p appears in $n's hands.", ch, obj, NULL, TO_ROOM);
    return;
}

void do_randomload(CHAR_DATA *ch, char *argument)
{
    ROOM_INDEX_DATA *pRoomIndex = NULL;
    OBJ_DATA *obj;
    int room;

    if (get_obj_index(atoi(argument)) == NULL)
    {
	stc("That object doesn't exist!\n\r", ch);
	return;
    }

    for( ; ; )
    {
	room = number_range(100, 16000);

	pRoomIndex = get_room_index(room);
	if (pRoomIndex != NULL && !IS_SET(pRoomIndex->room_flags, ROOM_SAFE)
	 && !IS_SET(pRoomIndex->room_flags, ROOM_SOLITARY)
	 && !IS_SET(pRoomIndex->room_flags, ROOM_PRIVATE))
	    break;
    }

    obj = create_object(get_obj_index(atoi(argument)), 0);
    obj_to_room(obj, pRoomIndex);

    act("$p randomly loaded to a room on the mud.\n\r", ch, obj, NULL, TO_CHAR);
    return;
}

void do_nuke( CHAR_DATA *ch, char *argument)
{
    CHAR_DATA *victim;
    char strsave[MIL];
    char strback[MIL];
    char arg[MIL];
    char buf[MSL];

    one_argument(argument, arg);

    if (arg[0] == '\0')
    {
	stc("Syntax:nuke <character name> \n\r", ch);
	return;
    }

    if ((victim = get_char_world(ch, arg)) == NULL)
    {
	stc("They are not logged on.\n\r", ch);
	return;
    }

    if (IS_NPC(victim))
    {
	stc("This command only works on players.\n\r", ch);
	return;
    }

    sprintf(strsave, "%s%s", PLAYER_DIR, capitalize(victim->name));
    sprintf(strback, "%s%s", BACKUP_DIR, capitalize(victim->name));
    stop_fighting(victim, TRUE);
    sprintf(buf, "#r%s#n has been nuked by #r%s#n", victim->name, ch->name);
    do_info(ch, buf);
    do_quit(victim, "");
    unlink(strsave);
    unlink(strback);
    return;
}

void do_prepfor( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA *vch;
    char buf[MSL];

    sprintf(buf, "Prepare for #G%s!#n", argument);
    do_info(ch, buf);
    do_restore(ch, "all");
    for (vch = char_list; vch != NULL; vch = vch->next)
    {
	if (IS_NPC(vch) || vch->desc == NULL) continue;
	if (vch->mkill < 5) vch->mkill = 5;
	if (vch->level < LEV_AWA) vch->level = LEV_AWA;
	do_peace(vch, "");
	do_stand(vch, "");
	do_call(vch, "all");
	do_save(vch, "");
    }
    return;
}

void do_clearreply(CHAR_DATA *ch, char *argument)
{
    CHAR_DATA *vch;
    bool donesomething = FALSE;

    for (vch = char_list; vch; vch = vch->next)
    {
	if (vch->level >= LEV_IMP) continue;
	if (vch->reply == ch)
	{
	    sprintf(log_buf, "%s no longer replies to you.\n\r", vch->name);
	    stc(log_buf, ch);
	    donesomething = TRUE;
	    vch->reply = NULL;
	}
    }

    if (!donesomething)
    {
	stc("Nobody was replying to you.\n\r", ch);
	return;
    }

    return;
}

void mageupkeep(CHAR_DATA *ch)
{
    char buf[MSL];

    stc("--------------------------------------------------------------------------------\n\r", ch);
    stc("                               #C(< #cMage Powers #C>)#n\n\r", ch);
    stc("--------------------------------------------------------------------------------\n\r", ch);
     
    if (effective_holylight(ch)) stc("Your mental control makes you more aware of your surroundings.\n\r",ch);
    if (effective_shadowsight(ch)) stc("You can locate nearby spirits.\n\r", ch);
    if (effective_nightsight(ch)) stc("Your mastery of energy allows you to sense heat patterns in the dark.\n\r", ch);

    if (ch->spheres[MLIF] >= 4)
    {
	sprintf(buf, "You are affected by better body for: %li.\n\r", ch->better_body_timer);
	stc(buf, ch);
    }

    if (ch->spheres[MTIM] >= 4)
    {
	sprintf(buf, "Your contingency is: %s.\n\r", ch->contingency);
	stc(buf, ch);
    }

    if (ch->spheres[MTIM] >= 5)
    {
	sprintf(buf, "You can use your powers of time again in: %li.\n\r", ch->doublecast_timer);
	stc(buf, ch);
    }

    if (IS_XTRA(ch, XTRA_HOLY_SMOTE)) stc("You are calling upon a holy sword to smite your enemies.\n\r", ch);

    if (IS_XTRA(ch, XTRA_HEARTS_BLOOD)) stc("You can channel your life's blood into your spells.\n\r", ch);

    if (ch->paradox_ward > 0)
    {
	sprintf(buf, "You are affected by a paradox ward with %li quintessence in it.\n\r", ch->paradox_ward);
	stc(buf, ch);
    }

    if (ch->beginners_luck > 0)
    {
	sprintf(buf, "You are affected by beginner's luck for %li attacks.\n\r", ch->beginners_luck);
	stc(buf, ch);
    }

    if (IS_AFFECTED(ch, AFF_POLYMORPH))
    {
	sprintf(buf, "You are %s.\n\r", ch->morph);
	stc(buf, ch);
    }

    if (IS_AFFECTED(ch, AFF_SHADOWPLANE)) stc("You are in the Shadowplane.\n\r", ch);
    if (IS_MORE(ch, MORE_SHIELDED)) stc("Your mental prowess helps you hide your location from others.\n\r", ch);

    if(ch->spheres[MPRI]>0)
    {
	sprintf(buf,"\n\rYou have %li quintessence stored in within your body.\n\rYou have %li quintessence stored within your Avatar.\n\r",
	ch->quint[BODY],ch->quint[AVATAR]);
	stc(buf,ch);
    }

    stc("--------------------------------------------------------------------------------\n\r", ch);
    return;
}


void load_leaderboard()
{
    FILE *fp;
    LEADER_BOARD staged={0};
    char **names[]={&staged.bestpk_name,&staged.worstpk_name,
        &staged.pk_name,&staged.pd_name,&staged.mk_name,&staged.md_name,
        &staged.tt_name,&staged.qc_name};
    long *numbers[]={&staged.bestpk_number,&staged.worstpk_number,
        &staged.pk_number,&staged.pd_number,&staged.mk_number,&staged.md_number,
        &staged.tt_number,&staged.qc_number};
    char **old_names[]={&leader_board.bestpk_name,&leader_board.worstpk_name,
        &leader_board.pk_name,&leader_board.pd_name,&leader_board.mk_name,
        &leader_board.md_name,&leader_board.tt_name,&leader_board.qc_name};
    char text[MAX_INPUT_LENGTH],extra[2];
    unsigned i;
    if ((fp = fopen("../area/leader.txt", "r")) == NULL)
    {
        if(errno==ENOENT){
            log_string("Error: leader.txt not found!");
            do_leaderclear(NULL, "");
            return;
        }
        goto failed;
    }
    for(i=0;i<8;i++){
        if(fread_tilde_checked(fp,text,sizeof text)!=FILE_STRING_OK)goto failed;
        *names[i]=str_dup(text);
        if(fread_number_checked(fp,numbers[i])!=FILE_NUMBER_OK)goto failed;
    }
    if(fread_word_checked(fp,extra,sizeof extra)!=FILE_WORD_EOF)goto failed;
    if(fclose(fp)!=0){fp=NULL;goto failed;}
    for(i=0;i<8;i++)free_string(*old_names[i]);
    leader_board=staged;
    leaderboard_load_failed=FALSE;
    leaderboard_save_pending=FALSE;
    leaderboard_retry_after=0;
    return;
failed:
    if(fp)fclose(fp);
    for(i=0;i<8;i++){
        free_string(*names[i]);
        if(!*old_names[i])*old_names[i]=str_dup("Noone");
    }
    leaderboard_load_failed=TRUE;
    log_string("load_leaderboard: rejected file; scores preserved, saving disabled until repair/reload or explicit leaderclear");
}

/* Atomic leaderboard writer scope. */
static int write_leaderboard(FILE *fp,void *context)
{
    LEADER_BOARD *board=context;
    const char *names[]={board->bestpk_name,board->worstpk_name,board->pk_name,
        board->pd_name,board->mk_name,board->md_name,board->tt_name,board->qc_name};
    const long numbers[]={board->bestpk_number,board->worstpk_number,board->pk_number,
        board->pd_number,board->mk_number,board->md_number,board->tt_number,board->qc_number};
    for(unsigned i=0;i<8;i++){
        if(!names[i] || strchr(names[i],'~') || strlen(names[i])>=MAX_INPUT_LENGTH){
            errno=EINVAL;
            return -1;
        }
        if(fprintf(fp,"%s~\n%li\n",names[i],numbers[i])<0)return -1;
    }
    return 0;
}

void save_leaderboard()
{
    struct atomic_save_error error;
    enum atomic_save_result result;
    if(leaderboard_load_failed)return;
    result=atomic_save_leaderboard("../area",write_leaderboard,&leader_board,&error);
    leaderboard_save_pending=result!=ATOMIC_SAVE_OK;
    if(leaderboard_save_pending){
        char message[512];
        leaderboard_retry_after=current_time>LONG_MAX-30 ? LONG_MAX : current_time+30;
        snprintf(message,sizeof message,"Leaderboard save: %s (error %d; cleanup %d)",
            error.stage?error.stage:"unknown",error.number,error.cleanup_number);
        log_string(message);
    }
}

void retry_leaderboard_save(void)
{
    if(leaderboard_save_pending && !leaderboard_load_failed
        && current_time>=leaderboard_retry_after)
        save_leaderboard();
}
/* End atomic leaderboard writer scope. */
void do_leader( CHAR_DATA *ch, char *argument )
{
    char buf[MSL];

    stc("#l-==--==#r**#l==--==#r**#l==--==#r**#l== #RLEADER BOARD #l==#r**#l==--==#r**#l==--==#r**#l==--==#r**#l==#n\n\r\n\r",ch);

    stc("   #cMr. Best PK'er   #L--->    ", ch);
    sprintf(buf, "#C%-13s", leader_board.bestpk_name);
    stc(buf, ch);
    sprintf(buf, " #cwith a #C%li #cPK score#n\n\r",leader_board.bestpk_number);
    stc(buf, ch);

    stc("   #cMr. Deadly       #L--->    ", ch);
    sprintf(buf, "#C%-13s", leader_board.pk_name);
    stc(buf, ch);
    sprintf(buf, " #cwith #C%li #cpkills#n\n\r", leader_board.pk_number);
    stc(buf, ch);

    stc("   #cMr. Pathetic     #L--->    ", ch);
    sprintf(buf, "#C%-13s", leader_board.pd_name);
    stc(buf, ch);
    sprintf(buf, " #cwith #C%li #cpdeaths#n\n\r", leader_board.pd_number);
    stc(buf, ch);

    stc("   #cMr. Slayer       #L--->    ", ch);
    sprintf(buf, "#C%-13s", leader_board.mk_name);
    stc(buf, ch);
    sprintf(buf, " #cwith #C%li #cmkills#n\n\r", leader_board.mk_number);
    stc(buf, ch);

    stc("   #cMr. Wimpy        #L--->    ", ch);
    sprintf(buf, "#C%-13s", leader_board.md_name);
    stc(buf, ch);
    sprintf(buf, " #cwith #C%li #cmdeaths#n\n\r", leader_board.md_number);
    stc(buf, ch);

    stc("   #cMr. No life      #L--->    ", ch);
    sprintf(buf, "#C%-13s", leader_board.tt_name);
    stc(buf, ch);
    sprintf(buf, " #cwith #C%li #cHours played#n\n\r", leader_board.tt_number);
    stc(buf, ch);

    stc("   #cMr. Questor      #L--->    ", ch);
    sprintf(buf, "#C%-13s", leader_board.qc_name);
    stc(buf, ch);
    sprintf(buf, " #cwith #C%li #cQuests completed#n\n\r", leader_board.qc_number);
    stc(buf, ch);

    stc("\n\r#l-==#r**#l==--==#r**#l==--==#r**#l==--==#r**#l==--==#r**#l==--==#r**#l==--==#r**#l==--==#r**#l==--==#r**#l==#n\n\r",ch);
    return;
}

/* Checked leaderboard metrics scope: leave other counters/eligibility alone. */
void check_leaderboard( CHAR_DATA *ch, char *argument )
{
    bool changed = FALSE;
    long ratio, hours;
    bool have_ratio, have_hours;
    char *imm = "Akuma Chaos Dude Shane Corruption"; /* Only needed for Imp+ */

    if (ch == NULL || IS_NPC(ch) || ch->pcdata == NULL || ch->name == NULL) return;
    if (is_name(ch->name, imm) || IS_BUI(ch)) return; 
    have_ratio = get_ratio_checked(ch, &ratio);
    have_hours = get_hours_checked(ch, &hours);
    if (have_ratio && ratio > leader_board.bestpk_number)
    {
leader_board.bestpk_number = ratio;
	free_string(leader_board.bestpk_name);
	leader_board.bestpk_name = str_dup(ch->name);
	changed = TRUE;
    }
    if (have_ratio && ratio < leader_board.worstpk_number)
    {
leader_board.worstpk_number = ratio;
	free_string(leader_board.worstpk_name);
	leader_board.worstpk_name = str_dup(ch->name);
	changed = TRUE;
    }
    if (ch->mdeath > leader_board.md_number)
    {
	leader_board.md_number = ch->mdeath;
	free_string(leader_board.md_name);
	leader_board.md_name = str_dup(ch->name);
	changed = TRUE;
    }
    if (ch->mkill > leader_board.mk_number)
    {
	leader_board.mk_number = ch->mkill;
	free_string(leader_board.mk_name);
	leader_board.mk_name = str_dup(ch->name);
	changed = TRUE;
    }
    if (ch->pkill > leader_board.pk_number)
    {
	leader_board.pk_number = ch->pkill;
	free_string(leader_board.pk_name);
	leader_board.pk_name = str_dup(ch->name);
	changed = TRUE;
    }
    if (ch->pdeath > leader_board.pd_number)
    {
	leader_board.pd_number = ch->pdeath;
	free_string(leader_board.pd_name);
	leader_board.pd_name = str_dup(ch->name);
	changed = TRUE;
    }
    if (ch->pcdata->score[SCORE_NUM_QUEST] > leader_board.qc_number)
    {
	leader_board.qc_number = ch->pcdata->score[SCORE_NUM_QUEST];
	free_string(leader_board.qc_name);
	leader_board.qc_name = str_dup(ch->name);
	changed = TRUE;
    }
    if (have_hours && hours > leader_board.tt_number)
    {
leader_board.tt_number = hours;
	free_string(leader_board.tt_name);
	leader_board.tt_name = str_dup(ch->name);
	changed = TRUE;
    }
    if (changed && (!leaderboard_save_pending || current_time>=leaderboard_retry_after))
        save_leaderboard();
    return;
}

/* Shared PK-score metrics scope: preserve legacy operation order/rounding. */
bool get_ratio_checked(CHAR_DATA *ch, long *ratio)
{
    long sum, denominator, numerator, result;
    if (ch == NULL || ratio == NULL) return FALSE;
    if (IS_NPC(ch)) { *ratio = 0; return TRUE; }
    if (ch->pdeath == 0) {
        if (__builtin_mul_overflow(100L, ch->pkill, &result)) return FALSE;
    } else {
        if (__builtin_add_overflow(ch->pkill, ch->pdeath, &sum) || sum == 0 ||
            __builtin_mul_overflow(sum, sum, &denominator) ||
            __builtin_mul_overflow(100L, ch->pkill, &numerator) ||
            __builtin_mul_overflow(numerator, ch->pkill, &numerator) ||
            __builtin_mul_overflow(ch->pkill, numerator / denominator, &result)) return FALSE;
    }
    *ratio = result;
    return TRUE;
}

long get_ratio(CHAR_DATA *ch)
{
    long ratio = 0;
    (void)get_ratio_checked(ch, &ratio);
    return ratio;
}

void do_leaderclear(CHAR_DATA *ch, char *argument )
{
    /* This existing explicit reset intentionally replaces the stored scores. */
    leaderboard_load_failed=FALSE;
    free_string(leader_board.bestpk_name);
    leader_board.bestpk_name = str_dup("Noone");
    leader_board.bestpk_number = 0;
    free_string(leader_board.worstpk_name);
    leader_board.worstpk_name = str_dup("Noone");
    leader_board.worstpk_number = 0;
    free_string(leader_board.pk_name);
    leader_board.pk_name = str_dup("Noone");
    leader_board.pk_number = 0;
    free_string(leader_board.pd_name);
    leader_board.pd_name = str_dup("Noone");
    leader_board.pd_number = 0;
    free_string(leader_board.md_name);
    leader_board.md_name = str_dup("Noone");
    leader_board.md_number = 0;
    free_string(leader_board.mk_name);
    leader_board.mk_name = str_dup("Noone");
    leader_board.mk_number = 0;
    free_string(leader_board.qc_name);
    leader_board.qc_name = str_dup("Noone");
    leader_board.qc_number = 0;
    free_string(leader_board.tt_name);
    leader_board.tt_name = str_dup("Noone");
    leader_board.tt_number = 0;
    save_leaderboard();
    if (ch != NULL) stc(leaderboard_save_pending ?
        "Leader board reset in memory; disk save is pending.\n\r" :
        "Leader board cleared.\n\r", ch);
    return;
}


/*
 * Use this instead of strlen() when checking the size of playermade
 * buffers, ie in do_description, note writing and questing messages on eq
 * (do_quest, the setting of other-remove and the rest)
 */
long strlen2(const char *s)
{
  long i, b, count=0;

  if (s[0] == '\0') return 0;
  b = strlen(s);
  for (i = 0; i < b; i++)
  {
    if (s[i] == '#') count++;
  }
  return (b + 7 * count);
}


void do_blah( CHAR_DATA *ch, char *argument )
{
    char msg[]={78, 111, 119, 32, 72, 105, 114, 105, 114, 105, 110, 103, 0};

    stc(msg, ch);
    return;
}
