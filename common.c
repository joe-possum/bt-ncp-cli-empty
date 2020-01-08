void parse_address(const char *fmt,bd_addr *address) {
  char buf[3];
  int octet;
  for(uint8 i = 0; i < 6; i++) {
    memcpy(buf,&fmt[3*i],2);
    buf[2] = 0;
    sscanf(buf,"%02x",&octet);
    address->addr[5-i] = octet;
  }
}

uint8 ad_flags(uint8 *buffer, uint flags) {
  if(0 == flags) return 0;
  buffer[0] = 2;
  buffer[1] = 1;
  buffer[2] = flags;
  return 3;
}

uint8 ad_name(uint8 *buffer, char *name) {
  uint8 len = strlen(name);
  buffer[0] = len + 1;
  buffer[1] = 9;
  memcpy(&buffer[2],(uint8*)&name[0],len);
  return len + 2;
}

uint8 ad_manufacturer(uint8 *buffer, uint8 len, uint8 *data) {
  buffer[0] = len + 3;
  buffer[1] = 0xff;
  buffer[2] = 0xff;
  buffer[3] = 0xff;
  memcpy(&buffer[4],data,len);
  return len + 4;
}

bool ad_match_local_name(uint8 len, uint8 *data, char *name) {
  uint8 *end = data + len;
  while(data < end) {
    uint8 elen = *data++;
    uint8 type = data++;
    if(--elen == strlen(name) && (0x09 == type)) {
      if(memcmp(data,name,elen)) return 1;
      break;
    }
    data  += elen;
  }
  return 0;
}
