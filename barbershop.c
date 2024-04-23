#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <unistd.h>

void barber(sem_t *chair, sem_t *queue) {
    // Парикмахера будят и он встает с кресла.
    if (sem_wait(chair) == -1) {
        perror("barber->sem_wait");
        exit(EXIT_FAILURE);
    }

    printf("Парикмахер проснулся, начал свою работу.\n");

    // Парикмахер стрижет всех посетителей, чтобы сесть в кресло и уснуть.
    while (1) {
        int queue_count;
        if (sem_getvalue(queue, &queue_count) == -1) {
            perror("visitors->queue->sem_getvalue");
            exit(EXIT_FAILURE);
        }
        if (queue_count == 0) {
            break;
        }
    }

    // Парикмахер сел в кресло и уснул.
    if (sem_post(chair) == -1) {
        perror("chair->sem_wait");
        exit(EXIT_FAILURE);
    }

    printf("Парикмахер закончил свою работу, уснул.\n");
}

void visitors_queue(sem_t *chair, sem_t *queue, int num, int count) {
    // Пришедшие посетители будят парикмахера в кресле, если он спит.
    int someone_in_chair;
    while (1) {
        if (sem_getvalue(chair, &someone_in_chair) == -1) {
            perror("visitors->chair->sem_getvalue");
            exit(EXIT_FAILURE);
        }
        if (someone_in_chair == 0) {
            break;
        }
    }

    // someone_in_chair == 0 => Кресло свободно.
    // someone_in_chair == 1 => Кресло занято.

    // Посетитель ожидает свое место в очереди, чтобы постричься.
    while (1) {
        int queue_count;
        if (sem_getvalue(queue, &queue_count) == -1) {
            perror("visitors->queue->sem_getvalue");
            exit(EXIT_FAILURE);
        }
        if (queue_count == count - num + 1) {
            break;
        }
    }

    // Посетитель садится в кресло.
    if (sem_post(chair) == -1) {
        perror("visitors->sem_post->chair");
        exit(EXIT_FAILURE);
    }
    printf("Посетитель %d сел в кресло!\n", num);


    // Посетитель стрижется.
    sleep(1);

    // Посетитель постригся - встает с кресла и уходит, уступая мето следующему в очереди.
    if (sem_wait(chair) == -1) {
        perror("visitors->sem_wait->queue");
        exit(EXIT_FAILURE);
    }
    printf("Посетитель %d пострижен!\n", num);
    // Очередь продвигается.
    if (sem_wait(queue) == -1) {
        perror("visitors->sem_wait->queue");
        exit(EXIT_FAILURE);
    }

    exit(EXIT_SUCCESS);
}

int main(int argc, char *argv[]) {
    // Объявление переменных.
    pid_t process;
    sem_t *chair;
    sem_t *queue;

    // Проверка входных аргументов.
    if (argc < 2) {
        perror("Запуск программы: \"Исполняемый файл\" \"Количество посетителей\".");
        exit(EXIT_FAILURE);
    }

    // Создание семафора в разделяемой памяти, характеризующего кресло у парикмахера и очередь к нему.
    char sem_name1[] = "Chair";
    char sem_name2[] = "Queue";
    if ((chair = sem_open(sem_name1, O_CREAT, 0666, 1)) == -1) {
        perror("sem_open");
        exit(EXIT_FAILURE);
    }
    if ((queue = sem_open(sem_name2, O_CREAT, 0666, atoi(argv[1]))) == -1) {
        perror("sem_open");
        exit(EXIT_FAILURE);
    }

    // Разделяем процесс на парикмахера (родительский) и очередь (дочерний) процессы.
    if ((process = fork()) == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    } else if (process == 0) {
        // Создание необходимого количества посетителей.
        int i = 1;
        int number = atoi(argv[1]);
        for (int i = 1; i < number + 1; ++i) {
        	if ((process = fork()) == -1) {
                perror("visitor->fork");
                exit(EXIT_FAILURE);
            } else if (process == 0) {
                // Дочерний процесс иллюстрирует работу одного посетителя.
                visitors_queue(chair, queue, i, atoi(argv[1]));
                return 0;
            }
        }
    } else {
        // Родительский процесс иллюстрирует работу парикмахера.
        barber(chair, queue);
    }

    return 0;
}
