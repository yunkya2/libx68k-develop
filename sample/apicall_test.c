/*
 * libdos/libiocs API call build test.
 *
 * This source is built in three modes by sample/Makefile:
 *   - calls to library functions
 *   - calls to inline functions
 *   - calls to inline functions with inlining disabled
 *
 * The calls cover different argument and return-value layouts.  They are kept
 * behind an argc value that is not used in normal execution because this is a
 * build test, not a test that should alter the machine state when launched.
 */

#include <x68k/dos.h>
#include <x68k/iocs.h>

static int exercise_apis(void)
{
    static const char message[] = "libx68k API call test\r\n";
    struct iocs_time uptime;
    int result = 0;

    /* libdos: no argument, word, split-byte, pointer, and mixed arguments. */
    result ^= _dos_vernum();
    _dos_putchar('A');
    result ^= _dos_drvctrl(-1, 0);
    _dos_print(message);
    result ^= _dos_write(1, message, sizeof(message) - 1);

    /* libiocs: basic, split-register, multiple-register, and struct return. */
    result ^= _iocs_b_keysns();
    result ^= _iocs_b_putc('I');
    result ^= _iocs_hsvtorgb(0, 0, 0);
    result ^= _iocs_sp_regst(0, 0, 0, 0, 0, 0);
    uptime = _iocs_ontime();
    result ^= uptime.sec ^ uptime.day;

    return result;
}

int main(int argc, char **argv)
{
    (void)argv;
    return (argc == 12345) ? exercise_apis() : 0;
}
