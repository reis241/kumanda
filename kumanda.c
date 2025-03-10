#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>

#pragma comment(lib, "Ws2_32.lib")

#define BUFFER_SIZE 1024

// IPv4 adresini döndüren fonksiyon
char* get_ipv4_address() {
    FILE *fp;
    char buffer[BUFFER_SIZE];
    static char ipv4_address[BUFFER_SIZE]; // Statik dizi, fonksiyonun dönüşünden sonra da değerini korur
    ipv4_address[0] = '\0'; // Boş string ile başlat

    // Windows'ta `popen` yerine `_popen` kullan
    fp = _popen("ipconfig", "r");
    if (fp == NULL) {
        return NULL;
    }

    // Satır satır oku ve IPv4 adresini bul
    while (fgets(buffer, BUFFER_SIZE, fp) != NULL) {
        if (strstr(buffer, "IPv4 Address") || strstr(buffer, "IPv4 Adresi")) {
            char *colon_pos = strchr(buffer, ':'); // ':' karakterini bul
            if (colon_pos != NULL) {
                strcpy(ipv4_address, colon_pos + 2); // IP adresini kopyala (boşluğu atla)
                ipv4_address[strcspn(ipv4_address, "\r\n")] = '\0'; // Yeni satır karakterlerini temizle
                break;
            }
        }
    }

    // Komut çıktısını kapat
    _pclose(fp);

    return ipv4_address[0] ? ipv4_address : NULL; // Eğer adres bulunduysa döndür, yoksa NULL döndür
}

// Klavye tuşuna basan fonksiyon
void press_key(WORD key) {
    INPUT input;
    input.type = INPUT_KEYBOARD;
    input.ki.wScan = 0;
    input.ki.time = 0;
    input.ki.dwExtraInfo = 0;

    input.ki.wVk = key;
    input.ki.dwFlags = 0;
    SendInput(1, &input, sizeof(INPUT));
    Sleep(100);
    input.ki.dwFlags = KEYEVENTF_KEYUP;
    SendInput(1, &input, sizeof(INPUT));
}

// QR kod oluşturucu fonksiyon
void generate_qr_code(const char* ip, int port) {
    if (!ip) {
        printf("IPv4 adresi alınamadı!\n");
        return;
    }

    char url[BUFFER_SIZE];
    snprintf(url, sizeof(url), "https://quickchart.io/qr?text=%s", ip);

    printf("QR Kod URL: %s\n", url);
    
    ShellExecute(NULL, "open", url, NULL, NULL, SW_SHOWNORMAL); 
}

int main() {
    WSADATA wsaData;
    SOCKET server_fd, new_socket;
    struct sockaddr_in address;
    char buffer[1024] = {0};

    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        printf("WSAStartup failed.\n");
        return 1;
    }

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == INVALID_SOCKET) {
        printf("Socket creation failed.\n");
        WSACleanup();
        return 1;
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(8080);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) == SOCKET_ERROR) {
        printf("Bind failed.\n");
        closesocket(server_fd);
        WSACleanup();
        return 1;
    }

    if (listen(server_fd, 3) == SOCKET_ERROR) {
        printf("Listen failed.\n");
        closesocket(server_fd);
        WSACleanup();
        return 1;
    }
    
    char *ip = get_ipv4_address();
    if (ip) {
        printf("Server Listening on IPv4: %s:8080\n", ip);
        generate_qr_code(ip, 8080); // QR kodunu oluştur ve tarayıcıda aç
    } else {
        printf("IPv4 address not found!\n");
    }

    int addrlen = sizeof(address);

    while (1) {
        new_socket = accept(server_fd, (struct sockaddr *)&address, &addrlen);
        if (new_socket == INVALID_SOCKET) {
            printf("Accept failed.\n");
            closesocket(server_fd);
            WSACleanup();
            return 1;
        }

        memset(buffer, 0, sizeof(buffer));
        int valread = recv(new_socket, buffer, sizeof(buffer) - 1, 0);
        if (valread > 0) {
            buffer[valread] = '\0';
            printf("Received: %s\n", buffer);

            // HTTP GET isteğini çözümle
            if (strncmp(buffer, "GET /right", 10) == 0) {
                press_key(VK_RIGHT);  // Sağ yön tuşuna bas
            } else if (strncmp(buffer, "GET /left", 9) == 0) {
                press_key(VK_LEFT);   // Sol yön tuşuna bas
            }

            // HTTP 200 OK yanıtını gönder
            char *http_response = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\nOK";
            send(new_socket, http_response, strlen(http_response), 0);
        }

        closesocket(new_socket); // Bağlantıyı kapat
    }

    closesocket(server_fd);
    WSACleanup();
    return 0;
}
