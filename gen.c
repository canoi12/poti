#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>

#if !WIN32
#include <dirent.h>
#endif

char header_name[256] = "GEN_H";
FILE *fp;
int arg;
char path[1000];

#define HEADER_COMMAND 0
#define TARGET_COMMAND 1

static int process_args(int argc, char **argv);

static int set_target_file(char **buf);
static int set_header_name(char **buf);
static int process_files(int argc, char **buf);

static int write_folder(const char *path);
static int write_file(const char *filename);
static int write_header(void);

static struct {
    char *name;
    int(*function)(char **buf);
} commands[] = {
    { "-n", set_header_name },
    { "-t", set_target_file },
};



int main(int argc, char **argv) {
    fp = stdout;
    return process_args(argc, argv);
#if 0
#ifdef _WIN32
    fopen_s(&fp, "poti.h", "w");
#else
    fp = fopen("poti.h", "w");
#endif
#endif
#if 0
    fprintf(fp, "#ifndef POTI_H\n");
    fprintf(fp, "#define POTI_H\n");
    fprintf(fp, "\n");
    new_file("font.ttf");
    new_file("boot.lua");
    fprintf(fp, "\n");
    fprintf(fp, "#endif\n");
    fclose(fp);
#endif
}

int process_args(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stdout, "usage: ./gen [-n header_name -t target_file] file1.txt file2.bin ...\n");
        return 1;
    }
    fprintf(stdout, "Embedding files: ");
    arg = 1;
    while (arg < argc) {
        char *c = argv[arg];
        if (*c == '-') {
            switch (c[1]) {
                case 'n': commands[HEADER_COMMAND].function(argv); break;
                case 't': commands[TARGET_COMMAND].function(argv); break;
                default: break;
            }
        } else {
            fprintf(stdout, "%s ", argv[arg]);
            process_files(argc, argv);
        }
        arg++;
    }
    fprintf(stdout, "\n");

    return 0;
}

int set_target_file(char **buf) {
    arg++;
    char *filename = buf[arg];
    FILE *f = fopen(filename, "w");
    if (f == NULL) {
        fprintf(stderr, "Failed to create target file: %s\n", filename);
        exit(EXIT_FAILURE);
    }
    fp = f;
    return 1;
}

int set_header_name(char **buf) {
    arg++;
    char *name = buf[arg];
    sprintf(header_name, "%s", name);
    return 1;
}

int process_files(int argc, char **buf) {
    fprintf(fp, "#ifndef %s\n", header_name);
    fprintf(fp, "#define %s\n", header_name);
    fprintf(fp, "\n");
    char *f = buf[arg];
    if (*f == '-') arg++;
    f = buf[arg];
    while (arg < argc) {
        write_file(buf[arg]);
        arg++;
    }

    fprintf(fp, "\n");
    fprintf(fp, "#endif /* %s */\n", header_name);
    return 1;
}

int write_file(const char *filename) {
    int filename_len = strlen(filename);
#if WIN32
    char var_name[1024];
#else
    char var_name[filename_len + 1];
#endif
    memcpy(var_name, filename, filename_len);
    var_name[filename_len] = '\0';

    char *p = var_name;
    while (strchr(p, '/')) p = strchr(p, '/') + 1;
    if (*p == '/' || *p == '\\') p++;
    char* pp;
    while((pp = strchr(p, '.')) || (pp = strchr(p, '/')))
        *pp = '_';

    FILE *f = fopen(filename, "rb");
    if (f == NULL) {
        return 0;
    }
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    long bytes_remaining = size;
    char rd_buf;
    fprintf(fp, "const char _%s[] = {\n", p);
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
    if (strstr(filename, ".lua")) {
	fprintf(fp, ", 0");
	size++;
    }
    fprintf(fp, "\n};\n");
    fprintf(fp, "const long _%s_size = %ld;\n", p, size);
    fclose(f);
    return 1;
}

int write_header(void) {

}
