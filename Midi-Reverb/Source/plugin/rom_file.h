/* SPDX-License-Identifier: GPL-3.0-only */
enum { ROM_BANKS = 3, ROM_MAX = 32768 };
static const struct rom_bank {
    const char *name, *file, *path;
    unsigned size, crc, programs, first;
} banks[ROM_BANKS] = {
    {"midiverb i", "midiverb.bin", "/yaffs2/mod/mv1rom/midiverb.bin", 16384, 0xde8ca203u, 63, 1},
    {"midifex", "midifex.bin", "/yaffs2/mod/mv1rom/midifex.bin", 16384, 0x098cc1b4u, 64, 1},
    {"midiverb ii", "midiverb2.bin", "/yaffs2/mod/mv1rom/midiverb2.bin", 32768, 0x2dd24b56u, 100, 0},
};
static volatile unsigned rom_ready, rom_revision;

static int transfer_rom(const struct rom_bank *bank, const char *path, int output)
{
    int fd = fw_yaffs_open(path, 0, 0);
    if (fd < 0) return 0;
    unsigned at = 0, crc = ~0u;
    unsigned char bytes[256] __attribute__((aligned(32)));
    int count;
    while ((count = fw_yaffs_read(fd, bytes, sizeof bytes)) > 0) {
        if ((unsigned)count > sizeof bytes || at + (unsigned)count > bank->size) break;
        at += (unsigned)count;
        for (int i = 0; i < count; ++i) {
            crc ^= bytes[i];
            for (int bit = 0; bit < 8; ++bit)
                crc = (crc >> 1) ^ (0xedb88320u & (0u - (crc & 1)));
        }
        if (output >= 0 && fw_yaffs_write(output, bytes, (unsigned)count) != count) break;
    }
    int closed = fw_yaffs_close(fd);
    return count == 0 && at == bank->size && ~crc == bank->crc && closed == 0;
}

static int same_name(const char *a, const char *b)
{
    while (*a && *a == *b) { ++a; ++b; }
    return *a == *b;
}

static int accept_rom(const char *name, const char *incoming)
{
    unsigned i = 0;
    while (i < ROM_BANKS && !same_name(name, banks[i].file)) ++i;
    if (i == ROM_BANKS) return 0;
    const struct rom_bank *bank = &banks[i];
    if (!transfer_rom(bank, incoming, -1)) return 0;
    if (transfer_rom(bank, bank->path, -1)) return 1;
    if (opus_flash_headroom() < (int)bank->size) return 0;
    fw_yaffs_mkdir("/yaffs2/mod/mv1rom", 0x4180);
    int fd = fw_yaffs_open(bank->path, 2 | 0x40 | 0x200, 0x180);
    if (fd < 0) return 0;
    int valid = transfer_rom(bank, incoming, fd);
    int closed = fw_yaffs_close(fd);
    if (valid && closed == 0) return 1;
    fw_yaffs_unlink(bank->path);
    return 0;
}

static void rom_changed(void)
{
    unsigned valid = 0;
    for (unsigned i = 0; i < ROM_BANKS; ++i)
        if (transfer_rom(&banks[i], banks[i].path, -1)) valid |= 1u << i;
    rom_ready = valid;
    ++rom_revision;
}

static const struct opus_folder rom_folder = { "mv1rom", ".bin", ROM_MAX, accept_rom, rom_changed };
