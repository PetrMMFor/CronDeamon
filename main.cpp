#include <iostream>
#include <fstream>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/date_time.hpp>
#include <vector>
#include <string>
#include <ctime>
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

//используется для экономии места
using b_ptime = boost::posix_time::ptime;

//Структура используемая для возврата значений
//времени и его периода
struct tmp_time
{
    int time = -1;
    int period = -1;
};

//Функция преобразования char в int
tmp_time extractNumeric(const std::string& c_strTask)
{
    tmp_time ext_time;
    //Проверка что строка не пустая
    if(!c_strTask.empty())
    {
        //Пробуем выполнить преобразование
        try
        {
            //если размер строки больше двух и первый символ "*", а второй "/"
            //то необходимо отделить период
            if((c_strTask.size() > 2) && (c_strTask.at(0) == '*') && (c_strTask.at(1) == '/'))
            {
                std::string newStringforInt = c_strTask.substr(2);
                ext_time.period = boost::lexical_cast<unsigned>(newStringforInt);
                ext_time.time = 0;
                return ext_time;
            }
            //если только "*" то используем время = 0, а период = 1
            //т.к. 0 по модулю 1 = 0
            if(c_strTask.at(0) == '*' && c_strTask.size() == 1)
            {
                ext_time.time = 0;
                ext_time.period = 1;
                return ext_time;
            }
            //если условия не выполнены то в строке лежит число
            ext_time.time = boost::lexical_cast<unsigned>(c_strTask);
            return ext_time;
        }
        //если не удалось выполнить преобразование
        //а именно, в строке лежит не число
        //то кидаем исключение
        catch(const boost::bad_lexical_cast&)
        { std::cout << "Error in type casting string -> int" << std::endl;}
    }
    return ext_time;
}
// Структура времени и его модуля(периода)
struct timeTask
{
    unsigned int minute;
    unsigned int hour;
    unsigned int day;
    int min_period;
    int hour_period;
    int day_period;
    timeTask():
        minute(0),
        hour(0),
        day(0),
        min_period(1),
        hour_period(1),
        day_period(1)
    {}
};
//Класс в котором хронится время считанное из конфига
//и сам скрипт
//Так же описанны методы преобразования char в int
class Task
{
public:
    timeTask t_time;
    std::string cmd_comand;
    void extractMinutesVal(const std::string &c_strTask)
    {
        tmp_time ext_time = extractNumeric(c_strTask);
        t_time.minute = ext_time.time;
        t_time.min_period = ext_time.period;
    }
    void extractHoursVal(const std::string &c_strTask)
    {
        tmp_time ext_time = extractNumeric(c_strTask);
        t_time.hour = ext_time.time;
        t_time.hour_period = ext_time.period;
    }
    void extractDaysVal(const std::string &c_strTask)
    {
        tmp_time ext_time = extractNumeric(c_strTask);
        t_time.day = ext_time.time;
        t_time.day_period = ext_time.period;
    }
    void extractCmdVal(const std::string &c_strTask)
    {
        cmd_comand = c_strTask;
    }
};

//Функция выполнения скрипта по времени
//выделяем из элемента  nowTime время и дату
void checkTimeAndExec(const b_ptime &nowTime, Task &tmp_Task, int sec_tmp)
{
        b_ptime::time_duration_type c_time = nowTime.time_of_day();
        b_ptime::date_type c_dat = nowTime.date();
        //Проверяем правльность времени по конфигу и правильность секунд
        //Если все верно выполняем скрипт
        if((c_time.minutes() % tmp_Task.t_time.min_period == 0)
                && (c_time.seconds() == sec_tmp))
        {
            system(tmp_Task.cmd_comand.c_str());
        }
        sleep(1);
};

int main()
{
    //Расположение конфига
    std::fstream in_file("/home/petr/cronDeamon/IN.txt");

    using vec_str = std::vector<std::string>;

    pid_t pid, sid;

    pid = fork();
    if(pid < 0)
    {
        exit(EXIT_FAILURE);
    }
    if(pid > 0)
    {
        exit(EXIT_SUCCESS);
    }

    umask(0);

    sid = setsid();
    if(sid < 0)
    {
        exit(EXIT_FAILURE);
    }

    if((chdir("/")) < 0)
    {
        exit(EXIT_FAILURE);
    }

    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    std::string tmp_string;
    vec_str tmp;
    //Разивание конфига на минуты / часы / дни / скрипт
    while(std::getline(in_file, tmp_string, ' '))
    {
        if(tmp_string[0])
        {
            //Тут нужна проверка символ перевода
            //коретки на следующую строку, под Linux
            //Не понятно как делать,это необходимо для считывания
            //только одной строки для cron демона
            tmp.push_back(tmp_string);
        }
    }
    //Проверка на правильность конфига
    if(tmp.size() != 4)
    {
        std::cout << "Error, not the right format\n"
                  << "Right format is: '* * * /script or 12 */2 * /script'"
                  << std::endl;

    }

    Task cronDeamon;
    //Преобразование строковой переменной в целочисленную
    cronDeamon.extractMinutesVal(tmp[0]);
    cronDeamon.extractHoursVal(tmp[1]);
    cronDeamon.extractDaysVal(tmp[2]);
    cronDeamon.extractCmdVal(tmp[3]);

    /*Тут начинает работать демон*/

    //условие на выполнение скрипта каждую минуту
    if(cronDeamon.t_time.min_period
            == cronDeamon.t_time.hour_period
            == cronDeamon.t_time.day_period
            == 1)
    {
        bool flag = false;
        while(1)
        {
            const b_ptime nowTime = boost::posix_time::second_clock::local_time();
            b_ptime::time_duration_type c_time = nowTime.time_of_day();
            b_ptime::date_type c_dat = nowTime.date();
            int tmp_sec;

            if(!flag)
            {
                tmp_sec = c_time.seconds();
                flag = true;
            }

            checkTimeAndExec(nowTime, cronDeamon, tmp_sec);

        }

    }

    in_file.close();
    exit(EXIT_SUCCESS);
    return 0;
}
