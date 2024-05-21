#include <stdio.h>      /* printf, sprintf */
#include <iostream>
#include <stdlib.h>     /* exit, atoi, malloc, free */
#include <unistd.h>     /* read, write, close */
#include <string.h>     /* memcpy, memset */
#include <sys/socket.h> /* socket, connect */
#include <netinet/in.h> /* struct sockaddr_in, struct sockaddr */
#include <netdb.h>      /* struct hostent, gethostbyname */
#include <arpa/inet.h>
#include <string>
#include <cstring>
#include "nlohmann/json.hpp"
#include "helpers.h"
#include "requests.h"

using namespace std;
using json = nlohmann::json;

#define SERVER_IP "34.246.184.49"
#define SERVER_PORT 8080
#define SERVER_API "/api/v1/tema"
#define CONTENT_TYPE "application/json"
#define MAX_LEN 1000


void sign_up(int sockfd) {
    char user[MAX_LEN], pass[MAX_LEN];

    cout << "username=";
    cin.getline(user, MAX_LEN);
    cout << "password=";
    cin.getline(pass, MAX_LEN);

    if(strchr(user, ' ')) {
        cout << "EROARE: Nu folositi spatii in acest camp" << endl;
        return;
    }

    if(strchr(pass, ' ')) {
        cout << "EROARE: Nu folositi spatii in acest camp" << endl;
        return;
    }


    json new_user = {{"username", user}, {"password", pass}};
    string json_str = new_user.dump();

    char* usr = (char*) malloc(json_str.length() + 1);
    if (!usr) {
        perror("malloc failed");
        exit(EXIT_FAILURE);
    }
    strcpy(usr, json_str.c_str());

    char *message = compute_post_request(SERVER_IP, SERVER_API "/auth/register", CONTENT_TYPE, &usr, 1, NULL, 0, NULL);

    send_to_server(sockfd, message);
    char *response = receive_from_server(sockfd);

    //cout << response << endl;

    if(strstr(response, "{"))
        cout << "EROARE: Utilizatorul exista deja" << endl;
    else
        cout << "SUCCES: " << user << " a fost inregistrat" << endl;

    free(usr);
    free(message);
    free(response);
}

void sign_in(int sockfd, char **token) {
    char user[MAX_LEN], pass[MAX_LEN];

    cout << "username=";
    cin.getline(user, MAX_LEN);
    cout << "password=";
    cin.getline(pass, MAX_LEN);

    if(strchr(user, ' ')) {
        cout << "EROARE: Nu folositi spatii in acest camp" << endl;
        return;
    }

    if(strchr(pass, ' ')) {
        cout << "EROARE: Nu folositi spatii in acest camp" << endl;
        return;
    }

    json logged_user = {{"username", user}, {"password", pass}};

    char* usr = (char*) malloc(logged_user.dump().length() + 1);
    if (!usr) {
        perror("malloc failed");
        exit(EXIT_FAILURE);
    }
    strcpy(usr, logged_user.dump().c_str());

    char *message = compute_post_request(SERVER_IP, SERVER_API "/auth/login", CONTENT_TYPE, &usr, 1, NULL, 0, NULL);

    send_to_server(sockfd, message);
    char *response = receive_from_server(sockfd);

    //cout << response << endl;

    if(strstr(response, "{"))
        cout << "EROARE: Credentiale gresite" << endl;
    
    else {
        cout << "SUCCES: " << user << " a fost logat" << endl;

        if(strstr(response, "connect.sid")) {
            char *new_token = strtok(strstr(response, "connect.sid"), ";");

            if(*token != NULL)
                free(*token);

            *token = (char *) malloc(strlen(new_token) + 1);
            strcpy(*token, new_token);
        }
    }
    

    free(usr);
    free(message);
    free(response);
}

void get_access(int sockfd, char **account_token, char **lib_token) {
    char *message = compute_get_request(SERVER_IP, SERVER_API "/library/access", NULL, account_token, 1, NULL);

    send_to_server(sockfd, message);
    char *response = receive_from_server(sockfd);

    //cout << response << endl;

    char *token_response = strstr(response, "{");

    json tok = json::parse(token_response);

    if(tok.contains("error")) {
        cout << "EROARE: Nu s-a putut face acces in biblioteca" << endl;
    }
    else {

        if(*lib_token != NULL)
            free(*lib_token);
        
        *lib_token = NULL;
        *lib_token = (char *) malloc(((string)tok.at("token")).length() + 1);
        strcpy(*lib_token, ((string)tok.at("token")).c_str());

        cout << "SUCCES: Ati intrat in biblioteca" << endl;
    }

    free(message);
    free(response);
}

void get_books(int sockfd, char **account_token, char *lib_token) {
    char **msg = (char**)malloc(sizeof(char*));
    char *message = compute_get_request(SERVER_IP, SERVER_API "/library/books", NULL, account_token, 1, lib_token);

    send_to_server(sockfd, message);
    char *response = receive_from_server(sockfd);
    //cout << response << endl;

    char *books = strstr(response, "[");

    if(books)
        cout << json::parse(books).dump(4) << endl;
    else
        cout << "EROARE: Nu aveti acces la biblioteca" << endl;

    free(message);
    free(response);
}

void get_book(int sockfd, char **account_token, char *lib_token) {

    char id[MAX_LEN];
    cout << "id=";
    fgets(id, MAX_LEN, stdin);
    id[strlen(id) - 1] = 0;

    char *path = (char *) malloc (strlen(SERVER_API) + strlen("/library/books/") + strlen(id));
    strcpy(path, SERVER_API);
    strcat(path, "/library/books/");
    strcat(path, id);

    char **msg = (char**)malloc(sizeof(char*));
    char *message = compute_get_request(SERVER_IP, path, NULL, account_token, 1, lib_token);

    send_to_server(sockfd, message);
    char *response = receive_from_server(sockfd);
    //cout << response << endl;

    json body = json::parse(strchr(response, '{'));

    if(body.contains("error"))
        if(strstr(((string)body.at("error")).c_str(), "No book"))
            cout << "EROARE: Cartea nu a fost gasita" << endl;
        else
            cout << "EROARE: Nu aveti acces la biblioteca" << endl;

    else
        cout << body.dump(4) << endl;

    free(message);
    free(response);
    free(path);
}

bool check_number(char *s) {
    for(int i = 0; i < strlen(s); i++)
        if(s[i] < '0' || s[i] > '9')
            return false;
    
    return true;
}

void add_book(int sockfd, char **account_token, char *lib_token) {
    char title[MAX_LEN], author[MAX_LEN], genre[MAX_LEN], count[MAX_LEN], publisher[MAX_LEN];

    cout << "title=";
    cin.getline(title, MAX_LEN);
    cout << "author=";
    cin.getline(author, MAX_LEN);
    cout << "genre=";
    cin.getline(genre, MAX_LEN);
    cout<< "page_count=";
    cin.getline(count, MAX_LEN);
    cout << "publisher=";
    cin.getline(publisher, MAX_LEN);

    if(!strlen(title) || !strlen(author) || !strlen(genre) 
            || !strlen(count) || !strlen(publisher)) {
        cout << "EROARE: Formatul este gresit" << endl;
        return;
    }

    if(!check_number(count)) {
        cout << "EROARE: Formatul este gresit" << endl;
        return;
    }

    json book = {{"title", title}, {"author", author},
                {"genre", genre}, {"page_count", count},
                {"publisher", publisher}};
    
    char *book_string = (char *)malloc(book.dump().length() + 1);

    strcpy(book_string, book.dump().c_str());

    char *message = compute_post_request(SERVER_IP, SERVER_API "/library/books",
                                        CONTENT_TYPE, &book_string, 1, account_token, 1, lib_token);

    send_to_server(sockfd, message);
    char *response = receive_from_server(sockfd);

    if(strchr(response, '{'))
        cout << "EROARE: NU aveti acces la biblioteca" << endl;
    else
        cout << "SUCCES: Ati adaugat cartea cu succes" << endl;

}

void delete_book(int sockfd, char **account_token, char *lib_token) {

    char id[MAX_LEN];
    cout << "id=";
    fgets(id, MAX_LEN, stdin);
    id[strlen(id) - 1] = 0;

    char *path = (char *) malloc (strlen(SERVER_API) + strlen("/library/books/") + strlen(id));
    strcpy(path, SERVER_API);
    strcat(path, "/library/books/");
    strcat(path, id);

    char **msg = (char**)malloc(sizeof(char*));
    char *message = compute_delete_request(SERVER_IP, path, NULL, account_token, 1, lib_token);

    send_to_server(sockfd, message);
    char *response = receive_from_server(sockfd);
    char *response_token = strchr(response, '{');
    //cout << response << endl;

    if(response_token != NULL)
        if(strstr(response_token, "No book"))
            cout << "EROARE: Cartea nu a fost gasita" << endl;
        else
            cout << "EROARE: Nu aveti acces la biblioteca" << endl;
    else
        cout << "SUCCES: Cartea a fost stearsa cu succes" << endl;

    free(message);
    free(response);
    free(path);
}

void log_out(int sockfd, char **account_token, char **lib_token) {

    if(*account_token == NULL) {
        cout << "EROARE: Trebuie sa fiti logat pentru a va deloga" << endl;
        return;
    }
    char **msg = (char**)malloc(sizeof(char*));
    char *message = compute_get_request(SERVER_IP, SERVER_API "/auth/logout", NULL, account_token, 1, NULL);

    send_to_server(sockfd, message);
    char *response = receive_from_server(sockfd);
    //cout << response << endl;

    cout << "SUCCES: Ati fost delogat.." << endl;

    if(*account_token != NULL)
        free(*account_token);
    
    if(*lib_token != NULL)
        free(*lib_token);
    
    *account_token = NULL;
    *lib_token = NULL;

    free(message);
    free(response);
}

int main(int argc, char *argv[]) {
    char cmd[MAX_LEN];
    int sockfd;
    char *account_token = NULL;
    char *lib_token = NULL;

    while (true) {
        sockfd = open_connection(SERVER_IP, SERVER_PORT, AF_INET, SOCK_STREAM, 0);

        cin.getline(cmd, MAX_LEN);

        if (!strcmp(cmd, "register")) {
            sign_up(sockfd);
        } else if (!strcmp(cmd, "login")) {
            sign_in(sockfd, &account_token);
        } else if (!strcmp(cmd, "logout")) {
            log_out(sockfd, &account_token, &lib_token);
        } else if(!strcmp(cmd, "enter_library")) {
            get_access(sockfd, &account_token, &lib_token);
        } else if(!strcmp(cmd, "get_books")){
            get_books(sockfd, &account_token, lib_token);
        } else if(!strcmp(cmd, "get_book")){
            get_book(sockfd, &account_token, lib_token);
        } else if(!strcmp(cmd, "delete_book")){
            delete_book(sockfd, &account_token, lib_token);
        }  else if(!strcmp(cmd, "add_book")){
            add_book(sockfd, &account_token, lib_token);
        } else if(!strcmp(cmd, "exit")) {
            break;
        }

        close_connection(sockfd);
    }

    return 0;
}

