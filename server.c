// server.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>

#include "cache.h"

#define PORT 8080
#define BACKLOG 5
#define BUF_SIZE 2048

int main() {
    int server_fd = -1, client_fd = -1;
    struct sockaddr_in address;
    socklen_t addrlen = sizeof(address);
    char buffer[BUF_SIZE];

    // Инициализация кеша
    LRUCache *cache = init_cache();
    if (!cache) {
        fprintf(stderr, "Ошибка: не удалось инициализировать кэш\n");
        return 1;
    }

    // Создаём серверный сокет
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket");
        return 1;
    }

    // Устанавливаем опцию reuseaddr, чтобы можно было быстро перезапускать сервер
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind");
        close(server_fd);
        return 1;
    }

    if (listen(server_fd, BACKLOG) < 0) {
        perror("listen");
        close(server_fd);
        return 1;
    }

    printf("🌐 Сервер запущен и слушает порт %d\n", PORT);

    while (1) {
        client_fd = accept(server_fd, (struct sockaddr *)&address, &addrlen);
        if (client_fd < 0) {
            perror("accept");
            // не выходим, пробуем снова
            continue;
        }

        // Читаем сообщение от клиента (один запрос)
        ssize_t r = recv(client_fd, buffer, BUF_SIZE - 1, 0);
        if (r < 0) {
            perror("recv");
            close(client_fd);
            continue;
        }
        buffer[r] = '\0';

        printf("📩 Получено от клиента: %s\n", buffer);

        // Демосценарий: если сообщение содержит "INVALIDATE", мы вторым вызовом поставим другие права
        unsigned char xor_key = 0xAA;
        unsigned int rights_first = 0x1F;
        unsigned int rights_second = 0x1F;

        // Первый put — просто добавление/обновление
        put_block(cache, "client_block", buffer, (size_t)r + 1, xor_key, rights_first);

        CacheBlock *b = get_block(cache, "client_block");
        if (b) {
            printf("💾 Сервер: данные в кеше: %s\n", b->data);
        }

        // Если клиент просит инвалидировать (демо), делаем второй put с изменёнными правами
        if (strstr(buffer, "INVALIDATE") != NULL) {
            // ставим другие права — это вызовет инвалидизацию по нашей логике put_block
            rights_second = 0x0F;
            sleep(1); // небольшая пауза, чтобы видеть последовательность
            put_block(cache, "client_block", buffer, (size_t)r + 1, xor_key, rights_second);

            // проверим, доступен ли блок
            CacheBlock *b2 = get_block(cache, "client_block");
            if (!b2) {
                printf("🔎 Сервер: блок client_block недоступен в кеше (инвалидирован)\n");
            } else {
                printf("🔎 Сервер: после смены прав блок всё ещё в кеше: %s\n", b2->data);
            }
        }

        // Отправляем ответ клиенту
        const char *resp = "OK: server cached data";
        send(client_fd, resp, strlen(resp), 0);

        close(client_fd);
    }

    // никогда не дойдём сюда в нормальном цикле, но для чистоты:
    free_cache(cache);
    close(server_fd);
    return 0;
}
