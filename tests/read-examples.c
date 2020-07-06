#include <yaml.h>

#include <stdlib.h>
#include <stdio.h>
#include <dirent.h>
#include <string.h>

#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>

/*
// What does this program do? It looks for all files ending with ".yaml" in 
// the examples directory, and attempts to parse them to the end. If everything
// goes well, then nothing is printed out. However, if an error is found, then
// the program prints out as much diagnostics as possible: the filename, the 
// problem, the offset, and so on.
//
// This program was written because libyaml lacked a nice quick way for testers
// to add their test case YAML files to the repository itself - as compared to
// pyyaml, which makes it extremely easy. The libyaml source came with its own
// examples directory, but that appeared to be unused. So let's use it.
//
// Easy way to run this (and other tests) on any Linuxy-POSIXy system:
//
// tests/run-all-tests.sh
*/



const char * reldir = "../examples/";

int main(void)
{
    DIR *dir;
    struct dirent *fileitem;
    dir = opendir(reldir);
    char filename[60];
    if (dir) {
        while ((fileitem = readdir(dir)) != NULL) {
            char *dotpos = strrchr(fileitem->d_name, '.');
            if (dotpos && !strcmp(dotpos, ".yaml")) {
                yaml_parser_t parser;
                yaml_event_t event;
                int done = 0;
                strcpy(filename, reldir);
                strcat(filename, fileitem->d_name); 
              
                yaml_parser_initialize(&parser);
                FILE *input = fopen(filename, "rb");
                yaml_parser_set_input_file(&parser, input);
                while (!done) {
                    if (!yaml_parser_parse(&parser, &event)) {
                        printf("Filename: %s.\n", fileitem->d_name);
                        printf("Error: %d.\n", (int)parser.problem);
                        printf("Problem: %s.\n", parser.problem);
                        printf("Offset: %d.\n", (int)parser.problem_offset);
                        printf("Error: %d.\n", (int)parser.problem_value);
                        printf("Problem mark index: %d.\n", (int)parser.problem_mark.index);
                        printf("Problem mark line: %d.\n", (int)parser.problem_mark.line);
                        printf("Problem mark column: %d.\n", (int)parser.problem_mark.column);
                        printf("Context: %s.\n", parser.context);
                        printf("Context mark index: %d.\n", (int)parser.context_mark.index);
                        printf("Context mark line: %d.\n", (int)parser.context_mark.line);
                        printf("Context mark column: %d.\n", (int)parser.context_mark.column);
                        yaml_parser_delete(&parser);
                        fclose(input);
                        closedir(dir);
                        return(1);
                    }
                    done = (event.type == YAML_STREAM_END_EVENT);
                    yaml_event_delete(&event);
                }
                yaml_parser_delete(&parser);
                fclose(input);
            }
        }
        closedir(dir);
        return(0);
    }
    else { 
        printf("The examples directory doesn't exist, or has other issues.\n");
        return(1);
    }
}
