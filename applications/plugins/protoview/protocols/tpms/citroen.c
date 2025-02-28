/* Citroen TPMS. Usually 443.92 Mhz FSK.
 *
 * Preamble of ~14 high/low 52 us pulses
 * Sync of high 100us pulse then 50us low
 * Then Manchester bits, 10 bytes total.
 * Simple XOR checksum. */

#include "../../app.h"

static bool decode(uint8_t *bits, uint32_t numbytes, uint32_t numbits, ProtoViewMsgInfo *info) {

    /* We consider a preamble of 17 symbols. They are more, but the decoding
     * is more likely to happen if we don't pretend to receive from the
     * very start of the message. */
    uint32_t sync_len = 17;
    const char *sync_pattern = "10101010101010110";
    if (numbits-sync_len < 8*10) return false; /* Expect 10 bytes. */

    uint64_t off = bitmap_seek_bits(bits,numbytes,0,numbits,sync_pattern);
    if (off == BITMAP_SEEK_NOT_FOUND) return false;
    FURI_LOG_E(TAG, "Renault TPMS preamble+sync found");

    off += sync_len; /* Skip preamble + sync. */

    uint8_t raw[10];
    uint32_t decoded =
        convert_from_line_code(raw,sizeof(raw),bits,numbytes,off,
            "01","10"); /* Manchester. */
    FURI_LOG_E(TAG, "Citroen TPMS decoded bits: %lu", decoded);

    if (decoded < 8*10) return false; /* Require the full 10 bytes. */

    /* Check the CRC. It's a simple XOR of bytes 1-9, the first byte
     * is not included. The meaning of the first byte is unknown and
     * we don't display it. */
    uint8_t crc = 0;
    for (int j = 1; j < 10; j++) crc ^= raw[j];
    if (crc != 0) return false; /* Require sane checksum. */

    int repeat = raw[5] & 0xf;
    float kpa = (float)raw[6]*1.364;
    int temp = raw[7]-50;
    int battery = raw[8]; /* This may be the battery. It's not clear. */

    snprintf(info->name,sizeof(info->name),"%s","Citroen TPMS");
    snprintf(info->raw,sizeof(info->raw),
        "%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X",
        raw[0],raw[1],raw[2],raw[3],raw[4],raw[5],
        raw[6],raw[7],raw[8],raw[9]);
    snprintf(info->info1,sizeof(info->info1),"Tire ID %02X%02X%02X%02X",
        raw[1],raw[2],raw[3],raw[4]);
    snprintf(info->info2,sizeof(info->info2),"Pressure %.2f kpa", (double)kpa);
    snprintf(info->info3,sizeof(info->info3),"Temperature %d C", temp);
    snprintf(info->info4,sizeof(info->info4),"Repeat %d, Bat %d", repeat, battery);
    return true;
}

ProtoViewDecoder CitroenTPMSDecoder = {
    "Citroen TPMS", decode
};
