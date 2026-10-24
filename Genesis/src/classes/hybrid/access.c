/* Effective access, not raw storage or permission to activate a shared form. */
#include <sys/types.h>
#include <stdio.h>
#include "../../merc.h"
#include "access.h"
#include "player.h"

const HYBRID_STATE *hybrid_live_secondary(const CHAR_DATA *ch)
{
    if (!ch || IS_NPC(ch) || !ch->pcdata ||
        ch->pcdata->hybrid_present != 1 || !hybrid_player_save_valid(ch))
        return NULL;
    const HYBRID_STATE *s=&ch->pcdata->hybrid;
    /* Old records remain unconfigured, not guessed into a free native build. */
    if ((s->secondary_class==CLASS_VAMPIRE ||
         s->secondary_class==CLASS_MAGE ||
         s->secondary_class==CLASS_DRAGON) && !s->secondary_build)
        return NULL;
    if (s->secondary_class==CLASS_VAMPIRE &&
        (s->secondary_generation<1 || s->secondary_blood[BLOOD_POOL]<1))
        return NULL;
    if (s->secondary_class==CLASS_DRAGON && s->secondary_generation<1)
        return NULL;
    if (s->secondary_class==CLASS_WEREWOLF &&
        s->secondary_progression.wolf_rank<1) return NULL;
    if (s->secondary_class==CLASS_MAGE &&
        s->secondary_progression.arete<1) return NULL;
    return s;
}

int hybrid_power_parent(const CHAR_DATA *ch,long parent)
{
    if (!ch || IS_NPC(ch) || !ch->pcdata || !hybrid_parent_name(parent)) return 0;
    if (ch->class==parent) return 1;
    const HYBRID_STATE *s=hybrid_live_secondary(ch);
    return s && s->secondary_class==parent;
}

static int discipline(size_t i)
{
    switch (i) {
    case DISC_ANIMALISM:case DISC_AUSPEX:case DISC_CELERITY:
    case DISC_CHIMERSTRY:case DISC_DOMINATE:
    case DISC_FORTITUDE:case DISC_NECROMANCY:
    case DISC_OBFUSCATE:case DISC_OBTENEBRATION:case DISC_POTENCE:
    case DISC_PRESENCE:case DISC_PROTEAN:case DISC_QUIETUS:
    case DISC_SERPENTIS:case DISC_THAUMATURGY:case DISC_VICISSITUDE:
        return 1;
    default:return 0;
    }
}

long hybrid_power_rank(CHAR_DATA *ch, long parent,
                       enum hybrid_training_tree tree, size_t i)
{
    if (!ch || IS_NPC(ch) || !ch->pcdata) return 0;
    /* Validate before even a primary getter (some log bad indexes). */
    int valid=0;
    switch (tree) {
    case HYBRID_TRAIN_BREED:valid=parent==CLASS_WEREWOLF&&i<3;break;
    case HYBRID_TRAIN_AUSPICE:valid=parent==CLASS_WEREWOLF&&i<5;break;
    case HYBRID_TRAIN_TRIBE:valid=parent==CLASS_WEREWOLF&&i<=TRIBE_WENDIGOS;break;
    case HYBRID_TRAIN_DISCIPLINE:valid=parent==CLASS_VAMPIRE&&discipline(i);break;
    case HYBRID_TRAIN_SPHERE:valid=parent==CLASS_MAGE&&i<9;break;
    case HYBRID_TRAIN_ELEMENT:valid=parent==CLASS_ELEM&&i>=ELEM_FIRE&&i<=ELEM_INFIN;break;
    case HYBRID_TRAIN_COLOR:valid=parent==CLASS_SORCERER&&i<6;break;
    case HYBRID_TRAIN_HIGHLAND:valid=parent==CLASS_HIGHLANDER&&i<4;break;
    case HYBRID_TRAIN_NINJA:valid=parent==CLASS_NINJA&&i<3;break;
    case HYBRID_TRAIN_MONK:valid=parent==CLASS_MONK&&i<3;break;
    default:break;
    }
    if (!valid) return 0;
    if (ch->class==parent) {
        switch (tree) {
        case HYBRID_TRAIN_BREED:return get_breed(ch,i);
        case HYBRID_TRAIN_AUSPICE:return get_auspice(ch,i);
        case HYBRID_TRAIN_TRIBE:return get_tribe(ch,i);
        case HYBRID_TRAIN_DISCIPLINE:return get_disc(ch,i);
        case HYBRID_TRAIN_SPHERE:return ch->spheres[i];
        case HYBRID_TRAIN_ELEMENT:return ch->pcdata->powers[i];
        case HYBRID_TRAIN_COLOR:return get_color(ch,i);
        case HYBRID_TRAIN_HIGHLAND:return get_highland(ch,i);
        case HYBRID_TRAIN_NINJA:return get_ninja(ch,i);
        case HYBRID_TRAIN_MONK:return get_monk(ch,i);
        default:return 0;
        }
    }
    const HYBRID_STATE *s=hybrid_live_secondary(ch);
    long rank=0;
    if (!s || s->secondary_class!=parent ||
        !hybrid_parent_rank_read(ch,s,parent,tree,i,&rank)) return 0;
    return rank>0?rank:0; /* Unknown/first-learning markers never grant a power. */
}

int hybrid_power_drow(CHAR_DATA *ch, long power)
{
    const long learned=DROW_ACID|DROW_WEB|DROW_DARK|DROW_FIRE|DROW_BITE|
        DROW_DUALWIELD|DROW_BFIGHT|DROW_SPIDER|DROW_EYES|DROW_READ|DROW_NIGHT|
        DROW_SIGHT|DROW_PIMM|DROW_SKIP|DROW_ATTUNE|DROW_MIGHT|DROW_TOUGH;
    if (!ch || IS_NPC(ch) || !ch->pcdata || power<=0 ||
        (power&(power-1)) || (power&~learned)) return 0;
    if (IS_DROW(ch)) return get_drow(ch,power)!=0;
    const HYBRID_STATE *s=hybrid_live_secondary(ch);
    return s && s->secondary_class==CLASS_DROW &&
        (s->secondary_progression.drow[DROW_POWERS]&power)!=0;
}

int hybrid_power_dragon(CHAR_DATA *ch, long skill)
{
    const long learned=DRAG_FANGS|DRAG_NIGHT|DRAG_CLAWS|DRAG_HORNS|DRAG_WINGS|
        DRAG_BREATH|DRAG_TAIL|DRAG_TRUESIGHT|DRAG_DUST|DRAG_MAGIC|DRAG_SCRY|
        DRAG_READAURA|DRAG_SHIELD;
    if (!ch || IS_NPC(ch) || !ch->pcdata || skill<=0 ||
        (skill&(skill-1)) || (skill&~learned)) return 0;
    if (IS_DRAGON(ch)) return IS_DRAGAFF(ch,skill)!=0;
    const HYBRID_STATE *s=hybrid_live_secondary(ch);
    return s && s->secondary_class==CLASS_DRAGON &&
        (s->secondary_progression.dragon_skills&skill)!=0;
}
