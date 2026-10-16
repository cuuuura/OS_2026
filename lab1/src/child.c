#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <math.h>

#define EPS 1e-6f

int ftostr(float x, char * res) {
    int len = 0;
    int start = 0;
    if (fabsf(x) < EPS) {
        res[len++] = '0';
        res[len++] = '.';
        for (int k = 0; k < 6; k++) res[len++] = '0';
    }
    else {
        if (x < 0) {
            res[len++] = '-';
            x = -x;
            start = 1;
        }

        int int_part = (int)x;
        float float_part = x - int_part + 5e-7f;
        if (int_part == 0) {
            res[len++] = '0';
        }
        else {
            while (int_part > 0) {
                res[len++] = '0' + int_part % 10;
                int_part /= 10;
            }
        
            int t;
            int count = len - start;
            int left, right;
            for(int j = 0; j < count / 2; j++) {
            left = start + j;
            right = len - 1 - j;
                t = res[left];
            res[left] = res[right];
            res[right] = t;
            }
        }
        res[len++] = '.';

        float_part *= 10;
        int_part = float_part / 1;
        if (int_part > 9) int_part = 9;
        float_part -= int_part;
        for (int k = 0; k < 6; k++) {
            res[len++] = int_part + '0';
            float_part *= 10;
            int_part = float_part / 1;
            if (int_part > 9) int_part = 9;
            float_part -= int_part;
        }
    }

    res[len] = '\0';
    return len;
}
 
int main(int argc, char *argv[]) {
    if (argc < 2) { write(2, "No filename\n", 12); _exit(1); }

    int fd = open(argv[1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) { write(2, "Can't open file\n", 16); _exit(2); }

    ssize_t n;
    char *token;
    char buf[4096], str_result[64];
    float other, result;
    int len;

    while ((n = read(0, buf, sizeof(buf) - 1)) > 0) {
        buf[n] = '\0';

        token = strtok(buf, " \t\n");
        if (token == NULL) { write(1, "1", 1); continue; }

        result = strtof(token, NULL);

        while ((token = strtok(NULL, " \t\n")) != NULL) {
            other = strtof(token, NULL);
            if (fabsf(other) < EPS) {
                write(1, "2", 1);
                close(fd);
                _exit(10);
            }
            result /= other;
        }

        len = ftostr(result, str_result);
        write(fd, str_result, len);
        write(fd, "\n", 1);
        
        write(1, "1", 1);
    }
    
    write(1, "0", 1);
    close(fd);
    return 0;
}