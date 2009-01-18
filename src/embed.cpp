// A simple program to create resources.cpp
// Usage: embed [input1 internalname1 input2...]

#include <stdio.h>
#include <vector>
#include <string>
#include <utility>

typedef std::pair< std::string, int > file;
typedef std::vector< file > fvec;

int main(int argc, char** argv)
{
    puts("// Generated file - do not edit");
    puts("");
    puts("#include <cstring>");
    puts("#include \"resources.h\"");
    fvec names;
    for (int i = 1, j = 1; i < argc; i += 2, ++j) {
        FILE* f = fopen(argv[i], "rb");
        if (f) {
            int len = 0, c;
            printf("\nstatic const unsigned char resource_%d_buffer[] = {", j);
            while ((c = getc(f)) != EOF) {
                if (len) putchar(',');

                if (len++ % 16 == 0) printf("\n\t");else putchar(' ');

                printf("%3d", c);
            }
            fclose(f);
            puts("\n};");
            names.push_back(file(argv[i + 1], len));
        }
    }

    printf("\nstatic const InternalResource resource_list[] = {");
    int i = 1;
    for (fvec::const_iterator ni = names.begin(); ni != names.end(); ++ni, ++i) {
        printf("\n\t{ \"%s\", resource_%d_buffer, %d },",
            ni->first.c_str(), i, ni->second);
    }

    puts("\n\t{ 0, 0, 0 }\n};");
    puts("");
    puts("const InternalResource*");
    puts("getResource(const char* filename)");
    puts("{");
    puts("\tfor (const InternalResource* rv = resource_list; rv->buffer; ++rv) {");
    puts("\t\tif (strcmp(rv->filename, filename) == 0) return rv;");
    puts("\t}");
    puts("\treturn NULL;");
    puts("}");
}
