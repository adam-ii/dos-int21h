#include <stdio.h> 
#include <stdarg.h>
#include <io.h>
#include <dos.h>
#include <fcntl.h>

char output_buf[4096];
char* output_ptr = output_buf;

void save_output(const char* format, ...)
{
    int output_remain = output_buf + sizeof(output_buf) - output_ptr;
    int written;
    va_list arglist;

    va_start(arglist, format);
    written = _vsnprintf(output_ptr, output_remain, format, arglist);
    va_end(arglist);
    output_ptr += written;
}

void flush_output()
{
    if (output_ptr != output_buf)
    {
        puts(output_buf);
    }
    output_ptr = output_buf;
}

void (__interrupt __far *prev_int_21)();

void __interrupt __far handle_int_21()
{
    unsigned char reg_ah, reg_al;
    unsigned int reg_bx, reg_cx, reg_dx, reg_ds;

    _asm {
        mov reg_ah, ah
        mov reg_al, al
        mov reg_bx, bx
        mov reg_cx, cx
        mov reg_dx, dx
        mov reg_ds, ds
    }

    save_output("\nInt 21h AH = %.2x: ", reg_ah);

    switch (reg_ah)
    {
        case 0x25:
            // DOS 1+ - SET INTERRUPT VECTOR
            save_output("SET INTERRUPT VECTOR\n");
            save_output("  %.2x         AL = interrupt number\n", reg_al);
            save_output("  %.4x:%.4x  DS:DX -> new interrupt handler\n", reg_ds, reg_dx);
            break;

        case 0x3d:
            // DOS 2+ - OPEN - OPEN EXISTING FILE
            save_output("OPEN EXISTING FILE\n");
            save_output("  %.2x         AL = access and sharing modes\n", reg_al);
            save_output("  %.4x:%.4x  DS:DX -> ASCIZ filename [%s]\n", reg_ds, reg_dx, (const char*)MK_FP(reg_ds, reg_dx));
            break;

        case 0x3e:
            // DOS 2+ - CLOSE - CLOSE FILE
            save_output("CLOSE FILE\n");
            save_output("  %.4x       BX = file handle\n", reg_dx);
            break;

        case 0x3f:
            // DOS 2+ - READ - READ FROM FILE OR DEVICE
            save_output("READ FROM FILE OR DEVICE\n");
            save_output("  %.4x       BX = file handle\n", reg_bx);
            save_output("  %.4x       CX = number of bytes to read\n", reg_cx);
            save_output("  %.4x:%.4x  DS:DX -> buffer for data\n", reg_ds, reg_dx);
            break;

        case 0x42:
            // DOS 2+ - LSEEK - SET CURRENT FILE POSITION
            save_output("SET CURRENT FILE POSITION\n");
            save_output("  %.2x         AL = origin of move\n", reg_al);
            save_output("  %.4x       BX = file handle\n", reg_bx);
            save_output("  %.4x:%.4x  CX:DX = (signed) offset from origin of new file position\n", reg_cx, reg_dx);
            break;

        case 0x44:
            if (reg_al == 0x00)
            {
                // DOS 2+ - IOCTL - GET DEVICE INFORMATION
                save_output("GET DEVICE INFORMATION\n");
                save_output("  %.4x   BX = handle\n");
                break;
            }
            // fall through

        default:
            save_output("Unhandled, AL = %.2x\n", reg_al);
            break;
    }

    _chain_intr(prev_int_21);
}

void set_int_21()
{
    prev_int_21 = _dos_getvect(0x21);
    _dos_setvect(0x21, handle_int_21);
    // save_output("_dos_setvect(0x21) was %Fp, now %Fp\n", prev_int_21, _dos_getvect(0x21));
}

void restore_int_21()
{
    save_output("\n_dos_setvect(0x21, %Fp)\n", prev_int_21);
    _dos_setvect(0x21, prev_int_21);
}

void test_io(const char* fname)
{
    int fd;

    set_int_21();

    save_output("\nopen(%s) (%Fp)\n", fname, fname);
    fd = open(fname, O_RDONLY | O_TEXT);
    if (fd >= 0)
    {
        char buffer[64];

        save_output("\nread() fd=%d buffer=%Fp size=%d\n", fd, buffer, sizeof(buffer));
        read(fd, buffer, sizeof(buffer));

        save_output("\ntell() fd=%d\n", fd);
        tell(fd);

        save_output("\nclose() fd=%d\n", fd);
        close(fd);
    }

    restore_int_21();

    flush_output();
}

void test_stdio(const char* fname)
{
    FILE* f;

    set_int_21();

    save_output("\nfopen(%s) (%Fp)\n", fname, fname);
    f = fopen(fname, "r");
    if (f)
    {
        char buffer[64];

        save_output("\nfread() buffer=%Fp size=%d\n", buffer, sizeof(buffer));
        fread(buffer, sizeof(buffer), 1, f);

        save_output("\nftell()\n");
        ftell(f);

        save_output("\nfclose()\n");
        fclose(f);
    }

    restore_int_21();

    flush_output();
}

int main(int argc, char* argv[])
{
    save_output("***** io.h functions *****\n");
    test_io(argv[0]);

    save_output("***** stdio.h functions *****\n");
    test_stdio(argv[0]);
    return 0;
}
