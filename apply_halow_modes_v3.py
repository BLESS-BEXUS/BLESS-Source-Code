#!/usr/bin/env python3
"""VERSION 3 (2026-10-14). Apply runtime AP/GROUP selector to BLESS firmware from repository root.
Run: python apply_halow_modes.py
Only commits to local working tree. Backups not necessary: use git diff/revert.
"""
from pathlib import Path
import re, sys
ROOT = Path.cwd()
files = ['Core/Inc/txhalow.h','Core/Src/txhalow.c','Core/Inc/elink_cmd.h','Core/Src/elink_cmd.c','Core/Src/main.c']
print("BLESS HaLow mode patch VERSION 3")
original = {}
for path in files:
    fp = ROOT / path
    if not fp.is_file(): sys.exit(f'ERROR: Missing {path}. Run from BLESS-Source-Code root.')
    original[path] = fp.read_text(encoding='utf-8')
modified = dict(original)
def sub(path, old, new, count=1):
    s=modified[path]
    n=s.count(old)
    if n != count: raise RuntimeError(f'{path}: expected {count} occurrences, found {n}: {old[:65]!r}')
    modified[path]=s.replace(old,new,count)
def regex(path, pattern, replacement, count=1):
    s,n=re.subn(pattern,lambda match: replacement(match) if callable(replacement) else replacement,modified[path],count=count,flags=re.M)
    if n!=count:raise RuntimeError(f'{path}: pattern not found/multiple: {pattern[:75]}')
    modified[path]=s
try:
    h,c,eh,ec,m=files
    sub(h,'#define HALOW_GROUP_MODE 1', '''/* GROUP is optional; boot in NORMAL (AP) mode. */
typedef enum {
    HALOW_MODE_NORMAL = 0U,
    HALOW_MODE_GROUP = 1U
} HaLowMode_t;
#define HALOW_INITIAL_MODE HALOW_MODE_NORMAL''')
    regex(h,r'#if HALOW_GROUP_MODE\n#define HALOW_MODE "GROUP"\n#else\n#define HALOW_MODE "AP"\n','')
    regex(h,r'(#define HALOW_PSK[^\n]*\n)#endif\n',lambda match:match.group(1))
    sub(h,'uint8_t Config_HaLow(UART_HandleTypeDef *huart);','uint8_t Config_HaLow(UART_HandleTypeDef *huart, HaLowMode_t mode);')

    # Only single UART owner thread calls Tx_HaLow_Packet and Config_HaLow.
    sub(c,'#include "txhalow.h"','#include "txhalow.h"\nextern volatile HaLowMode_t halow_active_mode;')
    regex(c,r'(?s)#if[ \t]+HALOW_GROUP_MODE[ \t]*\r?\n.*?#else.*?#endif[ \t\r\n]*memcpy\(eth_hdr\.dest_mac,\s*mac_dest,\s*6\);', '''    if (halow_active_mode == HALOW_MODE_GROUP) {
        const uint8_t dest[6] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
        memcpy(eth_hdr.dest_mac, dest, sizeof(dest));
    } else {
        const uint8_t dest[6] = {0x82,0x59,0x13,0x89,0x90,0xC0};
        memcpy(eth_hdr.dest_mac, dest, sizeof(dest));
    }''')
    # Remove dead commented-out alternative header code (does not affect behavior).
    regex(c,r'(?s)/\*\s*EthernetHeader eth_hdr = \{0\};.*?eth_hdr\.eth_type = 0x0000;\*/\s*','')
    sub(c,'uint8_t Config_HaLow(UART_HandleTypeDef *huart)','uint8_t Config_HaLow(UART_HandleTypeDef *huart, HaLowMode_t mode)')
    sub(c,'HaLow_Send_Config_Command(huart, "MODE", HALOW_MODE)','HaLow_Send_Config_Command(huart, "MODE", mode == HALOW_MODE_GROUP ? "GROUP" : "AP")')
    if modified[c].count('#if !HALOW_GROUP_MODE') != 2: raise RuntimeError('Expected two GROUP preprocessor sections')
    regex(c,r'(?s)#if !HALOW_GROUP_MODE(.*?)#endif',lambda z: 'if (mode == HALOW_MODE_NORMAL) {' + z.group(1) + '} ',count=2)

    sub(eh,'#include "main.h"','#include "main.h"\n#include "txhalow.h"')
    sub(eh,'#define ELINK_RF_SILENCE_PAYLOAD_SIZE      0U','#define ELINK_RF_SILENCE_PAYLOAD_SIZE      0U\n#define ELINK_SET_HALOW_MODE_PAYLOAD_SIZE   1U\n#define ELINK_CMD_SET_HALOW_MODE           0x24U')
    sub(eh,'extern volatile uint8_t flag_halow_connect;','extern volatile uint8_t flag_halow_connect;\nextern volatile HaLowMode_t requested_halow_mode;\nextern volatile uint8_t flag_halow_mode_change;')
    sub(eh,'void CMD_HALOW_CONNECT();','void CMD_HALOW_CONNECT();\nvoid CMD_SET_HALOW_MODE(uint8_t mode);')
    sub(ec,'    // Data Management Commands','''    case ELINK_CMD_SET_HALOW_MODE:
        if (command->payload_size != ELINK_SET_HALOW_MODE_PAYLOAD_SIZE)
            return ELINK_CMD_ERR_PAYLOAD;
        if (command->payload[0] > (uint8_t)HALOW_MODE_GROUP)
            return ELINK_CMD_ERR_RANGE;
        CMD_SET_HALOW_MODE(command->payload[0]);
        break;

    // Data Management Commands''')
    sub(ec,'void CMD_SET_TX_POWER(uint8_t tx_power){','''void CMD_SET_HALOW_MODE(uint8_t mode){
    requested_halow_mode = (HaLowMode_t)mode;
    flag_halow_mode_change = 1U;
}
void CMD_SET_TX_POWER(uint8_t tx_power){''')
    sub(m,'volatile uint8_t flag_halow_connect = 0U;','''volatile uint8_t flag_halow_connect = 0U;
volatile HaLowMode_t halow_active_mode = HALOW_INITIAL_MODE;
volatile HaLowMode_t requested_halow_mode = HALOW_INITIAL_MODE;
volatile uint8_t flag_halow_mode_change = 0U;''')
    sub(m,'if(Config_HaLow(&hlpuart1)){','if(Config_HaLow(&hlpuart1, halow_active_mode)){')
    regex(m,r'\t\t  if\(flag_halow_connect\)\{\n\t\t\t  flag_halow_connect = 0U;\n\t\t\t  if\(Config_HaLow\(&hlpuart1\)\)\{\n\t\t\t\t  osEventFlagsSet\(InitFlags, FLAG_HALOW_OK\);\n\t\t\t  \}\n\t\t  \}', '''          if (flag_halow_mode_change || flag_halow_connect) {
              HaLowMode_t target = requested_halow_mode;
              flag_halow_mode_change = 0U;
              flag_halow_connect = 0U;
              /* Do not transmit until the new AT configuration finishes. */
              if (Config_HaLow(&hlpuart1, target)) {
                  halow_active_mode = target;
                  memset(&rf_data_cache, 0, sizeof(rf_data_cache));
                  osEventFlagsSet(InitFlags, FLAG_HALOW_OK);
                  printf("HaLow mode active: %s\\r\\n",
                      target == HALOW_MODE_GROUP ? "GROUP" : "AP");
              } else {
                  /* Partially applied commands may leave radio in unknown mode.
                     Recover with ELINK_CMD_HALOW_CONNECT (0x20). */
                  printf("HaLow mode reconfiguration failed; retry via E-Link\\r\\n");
              }
          }''')
    assert 'HALOW_GROUP_MODE' not in modified[c]
    assert 'HALOW_GROUP_MODE' not in modified[h]
except Exception as ex:
    sys.exit(f'NO FILES CHANGED. Patch incompatible with checked-out version: {ex}')
for p in files:
    (ROOT/p).write_text(modified[p],encoding='utf-8')
print('Patched 5 files. Default: AP. ELINK command 0x24: payload 0x00 AP, 0x01 GROUP.')
print('Review: git diff; compile and test on ground before flight.')
