/* BG stack headers */
#include "bg_types.h"

void parse_address(const char *fmt,bd_addr *address);
uint8 ad_flags(uint8 *buffer, uint flags);
uint8 ad_name(uint8 *buffer, char *name);
uint8 ad_manufacturer(uint8 *buffer, uint8 len, uint8 *data);
bool ad_match_local_name(uint8 len, uint8 *data, char *name);
