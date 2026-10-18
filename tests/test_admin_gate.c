/* Historical eligibility proof, no real player/account data. */
#include <assert.h>
#include <stdio.h>
#include "../Genesis/src/merc.h"

int main(void)
{
    /* Both names were permitted, and the old list matcher accepted them.
     * The vulnerable command must not be registered or linked anymore. */
    assert(check_parse_name("Akuma"));
    assert(check_parse_name("Shane"));
    assert(is_name("Akuma", "Akuma Chaos Dude"));
    assert(is_name("Shane", "Shane Corruption"));
    assert(LEV_SLE < LEV_IMP && LEV_IMP < LEV_ST);
    puts("historical name escalation prerequisites confirmed; command removed");
    return 0;
}
