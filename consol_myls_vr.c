#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>
#include <time.h>
#include <sys/types.h>
#include <pwd.h>
#include <grp.h>
//#include <stdio.h>

# define MAX_LEN_STR 1024 // максимальная длина строки ввода
# define MLEN 512 // максимальная длина промежуточных массивов

// 1. Функция получает аргументы команды из входящей строки. Первым аргументом хранит консольную команду
// В качестве аргументов принимает массив указателей для хранения аргументов программы и указатель на буфер для хранения строки ввода
// Возвращает количество аргументов команды
int Get_argv_ (char** argv_, char* str_)
{
    int num_arg_ = 0;
    char* token = strtok(str_, " ");

    while (token != NULL)
    {
        argv_[num_arg_] = token;
        token = strtok(NULL, " ");
        num_arg_++;
    }
    return num_arg_;
}

// 2 Функция для расшифровки прав доступа типа данных mode_t
// В качестве аргументов принимает переменную mode_t и строку для хранения информации о правах доступа
void modeToStr (mode_t m, char* str)
{
    // тип файла
    if (S_ISREG(m)!=0) str[0] = '-'; // файл
    if (S_ISBLK(m)!=0) str[0] = 'b'; // блочные файлы
    if (S_ISCHR(m)!=0) str[0] = 'c'; // символьный файл
    if (S_ISDIR(m)!=0) str[0] = 'd'; // папка
    if (S_ISFIFO(m)!=0) str[0] = 'p'; // FIFO специальный файл, или канал
    //if (S_ISLNK(m)!=0) str[0] = 'l';

    //для владельца файла
    if ((m & S_IRUSR)) str[1] = 'r'; else str[1] = '-'; // бит права чтения
    if ((m & S_IWUSR)) str[2] = 'w'; else str[2] = '-'; // бит записи
    if ((m & S_IXUSR)) str[3] = 'x'; else str[3] = '-'; // бит права
    //для владельца группы
    if ((m & S_IRGRP)) str[4] = 'r'; else str[4] = '-'; // бит права чтения
    if ((m & S_IWGRP)) str[5] = 'w'; else str[5] = '-'; // бит записи
    if ((m & S_IXGRP)) str[6] = 'x'; else str[6] = '-'; // бит права
    //для других пользователей
    if ((m & S_IROTH)) str[7] = 'r'; else str[7] = '-'; // бит права чтения
    if ((m & S_IWOTH)) str[8] = 'w'; else str[8] = '-'; // бит записи
    if ((m & S_IXOTH)) str[9] = 'x'; else str[9] = '-'; // бит права
    str[10] = '\0';
};

// 3. Функция для вывода подробной информации о файле
// В качестве аргументов принимает строку с именем, структуру stat (из <sys/stat.h>), содержащую информацию о файле,
//      а также массив, хранящий флаги для настройки вывода
void printInfo (char *name, struct stat st, const int f[3])
{
    if (f[0] == 0)
    {
        printf("%s ", name);
        return;
    }

    // права доступа
    char rights [11];
    modeToStr(st.st_mode, rights);
    printf("%s\t",rights);

    // количество жестких ссылок на объект.
    printf("%lu\t", st.st_nlink);

    // имя пользователя
    struct passwd   *pw;
    struct group    *gr;
    if ((pw = getpwuid(st.st_uid)))
        printf("%s\t", pw->pw_name);
    else
        printf("%hd\t", st.st_uid);
    // имя группы
    if ((gr = getgrgid(st.st_gid)))
        printf("%s\t", gr->gr_name);
    else
        printf("%hd\t", st.st_gid);

    // размер файла
    unsigned int sizef_ = st.st_size;
    const char postfix[] = {' ','K','M','G','T'};
    if (f[2] == 0)
        printf("%u\t", sizef_);
    else {
        int c = 0;
        while (sizef_ > 1000) {
            sizef_ /= 1024;
            c++;
        }
        printf("%u%c\t", sizef_, postfix[c]);
    }
    // время изменения файла
    struct tm* data = localtime(&st.st_mtime);
    const char *mouth[] = { "jan","feb","mar","apr","may","jun","jul","aug","sep","oct","nov","dec" };
    if (data->tm_year != 2026-1900)
        printf("%s\t%d\t%d\t", mouth[data->tm_mon], data->tm_mday, data->tm_year+1900);
    else
        printf("%s\t%d\t%d:%d\t", mouth[data->tm_mon], data->tm_mday, data->tm_hour, data->tm_min);

    // имя файла
    printf("%s\t", name);
    printf("\n");
};

// 4. Функция отделения имени от пути файла
// В качестве аргументов принимает путь к файлу (папке) и строку для хранения имени
void getName(const char *path, char *name)
{
    const size_t len = strlen(path);
    size_t i = len - 2;
    for ( ; i < -1; i--)
        if (path[i] == '/' || path[i] == '\\') break;
    for (size_t t = 1; t < (len - i); t++)
        name[t - 1] = path[t + i];
    name[len - i - 1] = '\0';
}

// 5. Функция для вывода информации о вложенных файлах (папках)
// В качестве аргументов принимает путь к папке и массив, хранящий флаги для настройки вывода
void ls_Folder(const char *path, int *f) {
    int count = 0; // Количество файлов(папок) в папке
    char p[MLEN]; // Строка для хранения пути вложенных файлов(папок)
    long int Total_ = 0; // Общее количество блоков, занимаемых файлами
    struct dirent *d;

    DIR *dh = opendir(path);
    while (readdir(dh) != NULL)
        count++;

    char** list_names = (char**)malloc((count) * sizeof(char*)); // Буфер для хранения имен вложенных файлов (папок)
    struct stat* list_info_struct_ = (struct stat*)malloc(count * sizeof(struct stat)); // Буфер для хранения struct stat вложенных файлов (папок)

    rewinddir(dh);
    for (size_t i = 0; i < count; i++) // Формирование списка вложенных файлов(папок)
    {
        d = readdir(dh);
        list_names[i] = (char*)malloc(260 * sizeof(char));
        strncpy(list_names[i], d->d_name, 260);
        strcpy(p, path);
        strcat(p, "/");
        strcat(p, list_names[i]);
        stat(p, &list_info_struct_[i]);
        Total_ += list_info_struct_[i].st_blocks/(long int)2;
    }
    closedir(dh);

    // вывода информации о вложенных папках(файлах)
    if (f[0] == 1)
        printf("total %lu\n", Total_);
    if (f[1] == 0)
        for (size_t i = 0; i < count; i++)
            printInfo(list_names[i], list_info_struct_[i], f);
    else
        for (int i = count - 1; i > -1; i--)
            printInfo(list_names[i], list_info_struct_[i], f);

    for (size_t i = 0; i < count; i++)
        free(list_names[i]);
    free(list_names);
    free(list_info_struct_);
}

// Функция для сортировки строк qsort
int compare(const void * str1, const void * str2)
{
    return strcmp(*(char**)str1, *(char**)str2);
}
int compare_r(const void * str1, const void * str2)
{
    return -strcmp(*(char**)str1, *(char**)str2);
}

// Функция реализующая команду смены директории cd

int main()
{
    // INFO
    printf("-----------WELCOME-----------\n");
    printf("available commands:\n");
    printf("Usage: ls [OPTION]... [FILE]... \t List information about the FILEs (the current directory by default)\n");
    printf("Usage: exit                     \t End the program\n");
    printf("-----------------------------\n");

    char* path_def = getcwd(NULL, MLEN);    // Путь по умолчанию
    char console_str[MAX_LEN_STR];  // буфер для хранения строки ввода
    char* Arg_comm[64];     // Массив указателей для хранения аргументов программы
    int num_arg;    // количество аргументов команды
    bool flag = true;   // флаг завершения цикла - выхода из программы

    while (flag)
    {
        console_str[0] = '\0';  // обнуление буфера строки ввода
        int flags[3] = {0};     // массив для хранения флагов команды
        optind = 0; optarg = NULL;            // обнуление глобальных переменных для функции getopt

        // Ввод
        printf("%s>>> ", path_def) ;     // Приглашения для ввода
        fgets(console_str, MAX_LEN_STR, stdin);     // Чтение строки ввода
        console_str[strcspn(console_str, "\n")] = '\0';     // Удаление \n
        if (strlen(console_str) == 0)  // Проверка ввод пустой строки
            continue;
        fflush(stdout);     // Сброс буфера вывода

        num_arg = Get_argv_(Arg_comm, console_str); // Получения массива аргументов

        // Проверка доступных команд: exit, cd, ls
        if ( strcmp(Arg_comm[0], "exit") == 0 )
        {
            flag = false;
            printf("The program is completed!\n");
            break;
        }
        //if ( strcmp(Arg_comm[0], "cd") == 0 ) {} // not implemented
        if ( strcmp(Arg_comm[0], "ls") == 0 ) {}
        else
        {
            printf("Command '%s' not found\n", Arg_comm[0]) ;
            continue;
        }

        // Обработка аргументов функцией getopt
        int opt;
        if (num_arg < 2) // Проверка необходима чтобы в функцию не попали пустые аргументы
        {
            opt = -1 ;
            optind = 1;
        }
        else
            opt = getopt(num_arg, Arg_comm, "lrh");
        while (opt != -1) {
            if (opt == 'l') flags[0] = 1;
            if (opt == 'r') flags[1] = 1;
            if (opt == 'h') flags[2] = 1;
            opt = getopt(num_arg, Arg_comm, "lrh");
        }
        //for (int i = optind; i < num_arg; i++) printf("\t%s\n", Arg_comm[i]);

        // вывод информации о файлах (папках) из текущей директории
        if (num_arg == optind) {
            ls_Folder(path_def, flags);
            printf("\n");
            continue;
        }

        // вывод информации о файлах (папках) переданных в аргументах
        if (flags[1] == 0)
            qsort(&Arg_comm[optind], num_arg-optind, sizeof(char*), compare); // сортировка аргументов в прямом порядке
        else
            qsort(&Arg_comm[optind], num_arg-optind, sizeof(char*), compare_r); // сортировка аргументов в обратном порядке

        // отбрасываем несуществующие файлы
        for (int i = optind; i < num_arg; i++)
        {
            char path[MLEN]="";
            strcat(path, path_def);
            strcat(path, "/");
            strcat(path, Arg_comm[i]);
            struct stat st;
            if (stat(path, &st) < 0 && stat(Arg_comm[i], &st) < 0)
            {
                printf("cannot access %s: No such file or directory \n", Arg_comm[i]);
                Arg_comm[i] = NULL;
            }
        }
        printf("\n");

        // Выводим все файлы
        for (size_t i = optind; i < num_arg; i++)
        {
            if (Arg_comm[i] == NULL)
                continue;
            struct stat st;
            int crit = stat(Arg_comm[i], &st);
            if (crit != -1 && S_ISDIR(st.st_mode)==0) { // Случай когда в качестве аргумента передано путь к файлу (включая его имя)
                char loc_name[MLEN];
                getName(Arg_comm[i], loc_name);
                printInfo(loc_name, st, flags);
                Arg_comm[i]=NULL;
                continue;
            }
            char path[MLEN]="";
            strcat(path, path_def);
            strcat(path, "/");
            strcat(path, Arg_comm[i]);
            crit = stat(path, &st);
            if (crit != -1 && S_ISDIR(st.st_mode) == 0) { // Случай когда в качестве аргумента передано только имя файла
                printInfo(Arg_comm[i], st, flags);
                Arg_comm[i]=NULL;
            }
        }
        printf("\n");

        // выводим все папки
        for (size_t i = optind; i < num_arg; i++)
        {
            if (Arg_comm[i]==NULL) continue;
            printf("%s:\n", Arg_comm[i]);
            char path[MLEN]="";
            strcat(path, path_def);
            strcat(path, "/");
            strcat(path, Arg_comm[i]);
            struct stat st;
            int crit = stat(path, &st);
            if (crit != -1)
            {
                ls_Folder(path, flags);
                Arg_comm[i]=NULL;
                continue;
            }
            crit = stat(Arg_comm[i], &st);
            if (crit!=-1)
            {
                char name[MLEN];
                getName(Arg_comm[i], name);
                ls_Folder(Arg_comm[i], flags);
                Arg_comm[i]=NULL;
            }
        }
    }
    return 0;
}