#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>

/*Родительский процесс создает дочерний процесс. Первой строчкой пользователь в консоль
родительского процесса пишет имя файла, которое будет передано при создании дочернего
процесса. Родительский и дочерний процесс должны быть представлены разными программами.
Родительский процесс передает команды пользователя через pipe1, который связан с
стандартным входным потоком дочернего процесса. Дочерний процесс принеобходимости
передает данные в родительский процесс через pipe2. Результаты своей работы дочерний
процесс пишет в созданный им файл. Допускается просто открыть файл и писать туда, не
перенаправляя стандартный поток вывода

4 вариант) Пользователь вводит команды вида: «число число число<endline>». Далее эти числа
передаются от родительского процесса в дочерний. Дочерний процесс производит деление
первого числа, на последующие, а результат выводит в файл. Если происходит деление на 0, то
тогда дочерний и родительский процесс завершают свою работу. Проверка деления на 0 должна
осуществляться на стороне дочернего процесса. Числа имеют тип float. Количество чисел может
быть произвольным.*/

int main(void) {
    char filename[4096];
    ssize_t n;

    write(1, "Enter filename: ", 16);
    n = read(0, filename, sizeof(filename) - 1);
    if (n <= 0) _exit(1);
    if (filename[n-1] == '\n') n--;
    filename[n] = '\0';

    int pipe1[2], pipe2[2];
    if (pipe(pipe1) < 0 || pipe(pipe2) < 0) {
        write(2, "error: creating pipes\n", 22);
        _exit(2);
    }

    pid_t child = fork();
    if (child < 0) {
        write(2, "error: fork\n", 12);
        _exit(3);
    }

    if (child == 0) {
        close(pipe1[1]);
        close(pipe2[0]);

        if (dup2(pipe1[0], 0) < 0) { write(2, "error: dup2 stdin\n", 18); _exit(4); }
        if (dup2(pipe2[1], 1) < 0) { write(2, "error: dup2 stdout\n", 19); _exit(5); }

        close(pipe1[0]);
        close(pipe2[1]);

        char *args[] = {"./child", filename, NULL};
        extern char **environ;
        execve(args[0], args, environ);

        write(2, "execve failed\n", 14);
        _exit(6);
    }

    close(pipe1[0]);
    close(pipe2[1]);

    char buf[4096];
    char status = 0;
    const char *msg = "Input line <num num num>:\n";

    write(1, msg, strlen(msg));
    while ((n = read(0, buf, sizeof(buf) - 1)) > 0) {
        buf[n] = '\0';

        int ws;
        if (waitpid(child, &ws, WNOHANG) == child) break;

        if (write(pipe1[1], buf, n) < 0) break;

        if (read(pipe2[0], &status, 1) <= 0) { status = '2'; break; }
        if (status == '0' || status == '2') break;

        write(1, msg, strlen(msg));
    }

    close(pipe1[1]);
    close(pipe2[0]);

    waitpid(child, NULL, 0);

    if (status == '2') _exit(10);
    return 0;
}