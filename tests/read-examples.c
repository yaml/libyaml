#include <yaml.h>

#include <stdlib.h>
#include <stdio.h>
#include <dirent.h>
#include <string.h>

#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
// tests/run-all-tests.sh

const char * reldir = "../examples/";

int
main(void)
{
    DIR *dir;
    struct dirent *fileitem;
    dir = opendir(reldir);
    char filename[60];
    if (dir) {
        while ((fileitem = readdir(dir)) != NULL) {
            char *dotpos = strrchr(fileitem->d_name, '.');
            if (dotpos && !strcmp(dotpos, ".yaml")) {
                printf("%s\n", fileitem->d_name);
                yaml_parser_t parser;
                yaml_event_t event;
                int done = 0;
                strcpy(filename, reldir);
                strcat(filename, fileitem->d_name); 
              
       //         yaml_parser_initialize(&parser);
                FILE *input = fopen(filename, "rb");
              //  yaml_parser_set_input_file(&parser, input);
              //  printf("%s\n", fileitem->d_name);
                fclose(input);
            }
        }
        closedir(dir);
        return(0);
    }
    else { 
        printf("THESE ARE THE EXAMPLE FILES!\n");
        return(1);
    }
}
