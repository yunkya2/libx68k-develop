/*
 * sbrk()
 */

#include <x68k/dos.h>
#include <unistd.h>
#include <errno.h>

//#define SBRK_DEBUG

#ifdef SBRK_DEBUG
#include <x68k/iocs_inline.h>
#endif

int __doserr2errno(int error);

#ifdef SBRK_DEBUG
static void puthex(int x)
{
    char buf[9];
    for (int i = 0; i < 8; i++) {
        int d = (x >> ((7 - i) * 4)) & 0xf;
        buf[i] = (d < 10) ? ('0' + d) : ('A' + d - 10);
    }
    buf[8] = '\0';
    _iocs_b_print(buf + 2);
}

static void put_counter(char digits[11])
{
  char *p = digits;

  while (*p == '0' && p[1] != '\0')
    p++;
  _iocs_b_print(p);

  for (int i = 9; i >= 0; i--) {
    if (digits[i] < '9') {
      digits[i]++;
      break;
    }
    digits[i] = '0';
  }
}

static int follows_jsr(uintptr_t address, uintptr_t text_start)
{
  uint16_t opcode;

  if ((address & 1) != 0)
    return 0;

  if (address >= text_start + 2) {
    opcode = *(const uint16_t *)(address - 2);
    if ((opcode & 0xfff8) == 0x4e90)       /* jsr (An) */
      return 1;
  }

  if (address >= text_start + 4) {
    opcode = *(const uint16_t *)(address - 4);
    if ((opcode & 0xfff8) == 0x4ea8 ||     /* jsr (d16,An) */
        (opcode & 0xfff8) == 0x4eb0 ||     /* jsr (d8,An,Xn) */
        opcode == 0x4eb8 ||                /* jsr (xxx).W */
        opcode == 0x4eba ||                /* jsr (d16,PC) */
        opcode == 0x4ebb)                  /* jsr (d8,PC,Xn) */
      return 1;
  }

  if (address >= text_start + 6 &&
      *(const uint16_t *)(address - 6) == 0x4eb9) /* jsr (xxx).L */
    return 1;

  return 0;
}
#endif

void *sbrk(ptrdiff_t incr)
{
  extern char *_HSTA, *_HEND, *_HMAX;     /* Set by linker.  */
  static char *heap_end;
  char *new_heap_end;
  char *prev_heap_end;
#ifdef SBRK_DEBUG
  extern char *_PSTA, *_SEND;
  extern char _etext;
  uintptr_t stack_marker;
  uintptr_t *scan = &stack_marker;
  uintptr_t *stack_end = (uintptr_t *)_SEND;
  uintptr_t text_start = (uintptr_t)_PSTA;
  uintptr_t text_end = (uintptr_t)&_etext;
  int skip_count = 4;
  char caller_number[11];

  caller_number[0] = '0';
  caller_number[1] = '0';
  caller_number[2] = '0';
  caller_number[3] = '0';
  caller_number[4] = '0';
  caller_number[5] = '0';
  caller_number[6] = '0';
  caller_number[7] = '0';
  caller_number[8] = '0';
  caller_number[9] = '2';
  caller_number[10] = '\0';

  static int first_call = 1;
  if (first_call) {
    first_call = 0;
    _iocs_b_print("sbrk called: _HSTA=");
    puthex((int)_HSTA);
    _iocs_b_print(" _HEND=");
    puthex((int)_HEND);
    _iocs_b_print("\r\n");
  }
#endif

  if (heap_end == 0) {
    if (_HSTA == 0) {             /* in the case of "-nostartfiles" */
      extern char _end;
      extern int _heap_size;
      _HSTA = &_end;
      _HEND = _HMAX = _HSTA + _heap_size;
    }
    heap_end = _HSTA;
  }
 
  prev_heap_end = heap_end;
  new_heap_end = heap_end + incr;

#ifdef SBRK_DEBUG
  _iocs_b_putc('*');
  puthex((int)incr);
  _iocs_b_putc(':');
  puthex((int)prev_heap_end);
  _iocs_b_putc(':');
  puthex((int)new_heap_end);
  while (scan < stack_end) {
    uintptr_t candidate = *scan++;
    uintptr_t caller_offset;

    if (candidate < text_start || candidate >= text_end ||
        !follows_jsr(candidate, text_start))
      continue;
    if (skip_count > 0) {
      skip_count--;
      continue;
    }

    caller_offset = candidate - text_start;
    if (caller_offset == 0)
      break;
    _iocs_b_print(" caller");
    put_counter(caller_number);
    _iocs_b_putc('=');
    puthex((int)caller_offset);
  }
#endif

  if (new_heap_end > _HEND) {
    char *new_block_end;
    extern char *_PSP;

    /* Extend the memory block for heap */

    new_block_end = (char *)(((uint32_t)new_heap_end + 0x3fff) & ~0x3fff);
    if (_PSP == 0 || new_block_end > _HMAX ||
        _dos_setblock(_PSP, (uint32_t)new_block_end - (uint32_t)_PSP) < 0) {
#ifdef SBRK_DEBUG
      _iocs_b_putc('!');
#endif
      errno = ENOMEM;
#ifdef SBRK_DEBUG
      _iocs_b_putc('x');
#endif
      return (void *)-1;
    }

    _HEND = new_block_end;
  }

  heap_end = new_heap_end;

#ifdef SBRK_DEBUG
  _iocs_b_putc('#');
  _iocs_b_putc('\r');
  _iocs_b_putc('\n');
#endif

  return (void *)prev_heap_end;
}
