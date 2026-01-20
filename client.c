#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "cache.h"

#define PORT 8080

int main() {
    int sock = 0;
    struct sockaddr_in serv_addr;
    char buffer[1024] = {0};
    char *hello = "Привет, сервер! Клиент подключился.";

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Ошибка при создании сокета");
        return 1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        perror("Неверный адрес или адрес не поддерживается");
        return 1;
    }

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Ошибка подключения");
        return 1;
    }

    send(sock, hello, strlen(hello), 0);
    printf("Сообщение отправлено серверу\n");

    read(sock, buffer, sizeof(buffer));
    printf("Ответ от сервера: %s\n", buffer);

    // Работа с кэшем
    LRUCache *cache = init_cache();

    char *data1 = "Привет, кэш!";
    put_block(cache, "block1", data1, strlen(data1) + 1, 0x1F, 0x10);

    // второй вызов с другими правами — вызовет инвалидирование
    char *data2 = "Обновлённый блок!";
    put_block(cache, "block1", data2, strlen(data2) + 1, 0x1F, 0x20);

    free_cache(cache);

    close(sock);
    return 0;
}
