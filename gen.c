#include <stdio.h>
#include <stdlib.h>
#include <string.h>

FILE *fp;

static int new_file(const char *filename) {
    int filename_len = strlen(filename);
    char var_name[filename_len + 1];
    memcpy(var_name, filename, filename_len);
    var_name[filename_len] = '\0';

    char *p;
    while((p = strchr(var_name, '.')) || (p = strchr(var_name, '/')))
        *p = '_';

    FILE *f = fopen(filename, "rb");
    if (f == NULL) {
        return 0;
    }
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    long bytes_remaining = size;
    char rd_buf;
    fprintf(fp, "const char _%s[] = {\n", var_name);
    while(bytes_remaining > 0) {
        fread(&rd_buf, 1, 1, f);
        fprintf(fp, "%d", rd_buf);
        if (bytes_remaining > 1)
            fprintf(fp, ", ");
        int diff = size - bytes_remaining;
        if (diff != 0 && diff % 32 == 0)
            fprintf(fp, "\n");
        bytes_remaining--;
    }
    fprintf(fp, "\n};\n");
    fprintf(fp, "const long _%s_size = %ld;\n", var_name, size);
    fclose(f);
    return 1;
}

int main(int argc, char **argv) {
#ifdef _WIN32
    fopen_s(&fp, "poti.h", "w");
#else
    fp = fopen("poti.h", "w");
#endif

    fprintf(fp, "#ifndef POTI_H\n");
    fprintf(fp, "#define POTI_H\n");
    fprintf(fp, "\n");
    new_file("5x5.ttf");
    new_file("boot.lua");
    fprintf(fp, "\n");
    fprintf(fp, "#endif\n");
    fclose(fp);

    // fopen_s(&fp, "5x5.ttf", "rb");
    // fseek(fp, 0, SEEK_END);
    // long size = ftell(fp);
    // fseek(fp, 0, SEEK_SET);

    // char *buffer = (char *)malloc(size);
    // fread(buffer, size, 1, fp);
    // fclose(fp);

    
    // //fprintf(fp, "struct { unsigned char *data; long size; } font_5x5_ttf = { .data = {");
    // fprintf(fp, "unsigned char font_5x5_ttf[] = {");
    // long bytes_remaining = size;
    // unsigned char *p = buffer;
    // while (bytes_remaining)
    // {
    //     fprintf(fp, "%d", *p);
    //     p++;
    //     bytes_remaining--;
    //     if (bytes_remaining)
    //         fprintf(fp, ", ");
    //     if (bytes_remaining % 32 == 0)
    //         fprintf(fp, "\n");
    // }
    // fprintf(fp, "};\n");
    // fprintf(fp, "long font_5x5_ttf_size = %ld;\n", size);
    // fclose(fp);
    // free(buffer);

    return 0;
}