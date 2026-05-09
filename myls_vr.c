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

# define MLEN 512 // максимальная длина промежуточных массивов

// 1. Функция для расшифровки прав доступа типа данных mode_t
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

// 2. Функция для вывода подробной информации о файле
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

// 3. Функция отделения имени от пути файла
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

// 4. Функция для вывода информации о вложенных файлах (папках)
// В качестве аргументов принимает путь к папке и массив, хранящий флаги для настройки вывода
void ls_Folder(const char *path, const int *f) {
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

int main(int argc, char* argv[])
{
    char* path_def = getcwd(NULL, MLEN);    // Путь по умолчанию
    int flags[3] = {0};     // массив для хранения флагов команды

    // Обработка аргументов функцией getopt
    int opt;
    while ((opt = getopt(argc, argv, "lrh")) != -1) {
        if (opt == 'l') flags[0] = 1;
        if (opt == 'r') flags[1] = 1;
        if (opt == 'h') flags[2] = 1;
    }

    // Вывод информации о файлах (папках) из текущей директории
    if (argc == optind) {
        ls_Folder(path_def, flags);
        printf("\n");
        return 0;;
    }

    // Формирование списка файлов и папок для вывода из аргументов функции main
    char **listFF = NULL;
    if (optind < argc)
    {
        listFF = (char** )malloc((argc - optind) * sizeof(char* ));
        for (size_t i = optind; i < argc; i++)
        {
            listFF[i - optind] = (char* )malloc(MLEN * sizeof(char));
            strncpy(listFF[i - optind], argv[i], 510);
        }
    }
    if (flags[1]==0)
        qsort(listFF, (argc - optind), sizeof(char* ), compare);
    else
        qsort(listFF, (argc - optind), sizeof(char* ), compare_r);

    // отбрасываем несуществующие файлы
    for (size_t i = 0; i < (argc - optind); i++)
    {
        char path[MLEN]="";
        strcat(path, path_def);
        strcat(path, "/");
        strcat(path, listFF[i]);
        struct stat st;
        if (stat(path, &st) < 0 && stat(listFF[i], &st) < 0)
        {
            printf("cannot access %s: No such file or directory \n", listFF[i]);
            free(listFF[i]);
            listFF[i] = NULL;
        }
    }
    printf("\n");

    // Выводим все файлы
    for (size_t i = 0; i < (argc - optind); i++)
    {
        if (listFF[i] == NULL)
            continue;
        struct stat st;
        int crit = stat(listFF[i], &st);
        if (crit != -1 && S_ISDIR(st.st_mode)==0) { // Случай когда в качестве аргумента передано путь к файлу (включая его имя)
            char loc_name[MLEN];
            getName(listFF[i], loc_name);
            printInfo(loc_name, st, flags);
            free(listFF[i]);
            listFF[i]=NULL;
            continue;
        }
        char path[MLEN]="";
        strcat(path, path_def);
        strcat(path, "/");
        strcat(path, listFF[i]);
        crit = stat(path, &st);
        if (crit != -1 && S_ISDIR(st.st_mode) == 0) { // Случай когда в качестве аргумента передано только имя файла
            printInfo(listFF[i], st, flags);
            free(listFF[i]);
            listFF[i]=NULL;
        }
    }
    printf("\n");

    // выводим все папки
    for (size_t i = 0; i < (argc - optind); i++)
    {
        if (listFF[i]==NULL)
            continue;
        printf("%s:\n", listFF[i]);
        char path[MLEN]="";
        strcat(path, path_def);
        strcat(path, "/");
        strcat(path, listFF[i]);
        struct stat st;
        int crit = stat(path, &st);
        if (crit != -1)
        {
            ls_Folder(path, flags);
            free(listFF[i]);
            listFF[i]=NULL;
            continue;
        }
        crit = stat(listFF[i], &st);
        if (crit!=-1)
        {
            ls_Folder(listFF[i], flags);
            free(listFF[i]);
            listFF[i]=NULL;
        }
    }
    free(listFF);
    return 0;
}